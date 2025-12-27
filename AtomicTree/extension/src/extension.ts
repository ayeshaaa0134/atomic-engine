import * as vscode from 'vscode';
import * as path from 'path';
import * as os from 'os';
import * as child_process from 'child_process';

// System metrics tracking
let previousCpuInfo: { idle: number, total: number } | null = null;

async function getDiskInfo(): Promise<{ model: string, type: string }> {
    return new Promise((resolve) => {
        if (os.platform() !== 'win32') {
            resolve({ model: 'Non-Windows', type: 'Generic' });
            return;
        }

        const cmd = 'powershell -Command "Get-PhysicalDisk | Select-Object FriendlyName, MediaType | ConvertTo-Json"';
        child_process.exec(cmd, (err, stdout) => {
            if (err) {
                resolve({ model: 'Unknown', type: 'Unknown' });
                return;
            }
            try {
                const data = JSON.parse(stdout);
                if (Array.isArray(data)) {
                    resolve({
                        model: data[0].FriendlyName,
                        type: data[0].MediaType === 4 ? 'SSD/NVM' : (data[0].MediaType === 3 ? 'HDD' : 'SSD')
                    });
                } else {
                    resolve({
                        model: data.FriendlyName,
                        type: data.MediaType === 4 ? 'SSD/NVM' : (data.MediaType === 3 ? 'HDD' : 'SSD')
                    });
                }
            } catch {
                resolve({ model: 'Drive Detected', type: 'SSD' });
            }
        });
    });
}

async function getSystemMetrics() {
    const cpus = os.cpus();
    const totalMemory = os.totalmem();
    const freeMemory = os.freemem();
    const platform = os.platform();
    const arch = os.arch();
    const diskInfo = await getDiskInfo();

    // Calculate CPU usage
    let cpuUsage = 0;
    if (cpus.length > 0) {
        let idle = 0;
        let total = 0;
        cpus.forEach(cpu => {
            for (const type in cpu.times) {
                total += (cpu.times as any)[type];
            }
            idle += cpu.times.idle;
        });

        if (previousCpuInfo) {
            const idleDiff = idle - previousCpuInfo.idle;
            const totalDiff = total - previousCpuInfo.total;
            cpuUsage = 100 - (100 * idleDiff / totalDiff);
        }

        previousCpuInfo = { idle, total };
    }

    return {
        cpuCores: cpus.length,
        cpuModel: cpus[0]?.model || 'Unknown',
        cpuUsage: Math.max(0, Math.min(100, cpuUsage)),
        totalMemoryGB: (totalMemory / (1024 ** 3)).toFixed(2),
        freeMemoryGB: (freeMemory / (1024 ** 3)).toFixed(2),
        usedMemoryGB: ((totalMemory - freeMemory) / (1024 ** 3)).toFixed(2),
        platform: platform,
        arch: arch,
        storageModel: diskInfo.model,
        storageType: diskInfo.type
    };
}

export function activate(context: vscode.ExtensionContext) {
    console.log('AtomicTree Extension Activated');

    const controlsProvider = new AtomicTreeWebviewProvider(context.extensionUri, 'controls');
    context.subscriptions.push(
        vscode.window.registerWebviewViewProvider('atomicTree.controls', controlsProvider)
    );

    const vizProvider = new AtomicTreeWebviewProvider(context.extensionUri, 'visualizer');
    context.subscriptions.push(
        vscode.window.registerWebviewViewProvider('atomicTree.visualizer', vizProvider)
    );

    // Broadcast system metrics periodically to both webviews
    const metricsInterval = setInterval(() => {
        const systemMetrics = getSystemMetrics();
        controlsProvider.sendMessage({ command: 'systemStats', payload: systemMetrics });
        vizProvider.sendMessage({ command: 'systemStats', payload: systemMetrics });
    }, 2000); // Update every 2 seconds

    context.subscriptions.push({ dispose: () => clearInterval(metricsInterval) });

    // Send initial metrics immediately
    setTimeout(() => {
        const systemMetrics = getSystemMetrics();
        controlsProvider.sendMessage({ command: 'systemStats', payload: systemMetrics });
        vizProvider.sendMessage({ command: 'systemStats', payload: systemMetrics });
    }, 500);

    // Register Task Provider
    context.subscriptions.push(
        vscode.tasks.registerTaskProvider('atomic-tree-build', new AtomicTreeTaskProvider(context))
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('atomicTree.startProfile', () => {
            // Auto-detect running AtomicTree program from terminal
            const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('AtomicTree');

            controlsProvider.sendMessage({ command: 'start' });
            vizProvider.sendMessage({ command: 'start' });
            vscode.window.showInformationMessage('AtomicTree Profile Started - Extension listening for metrics');

            // Start visualization
            vizProvider.sendMessage({ command: 'start' });
            vscode.window.showInformationMessage('AtomicTree Profile Started - Extension listening for metrics');
        }),
        vscode.commands.registerCommand('atomicTree.stopProfile', () => {
            controlsProvider.sendMessage({ command: 'stop' });
            vizProvider.sendMessage({ command: 'stop' });
            vscode.window.showInformationMessage('AtomicTree Profile Stopped');
        }),
        vscode.commands.registerCommand('atomicTree.runCurrentFile', async () => {
            const activeEditor = vscode.window.activeTextEditor;
            if (!activeEditor || !activeEditor.document.fileName.endsWith('.cpp')) {
                vscode.window.showErrorMessage('Please open a C++ file to run AtomicTree.');
                return;
            }

            // Build task
            const buildTask = await createBuildTask(context, activeEditor.document.uri);
            const execution = await vscode.tasks.executeTask(buildTask);

            // Wait for build to complete before starting viz
            vscode.tasks.onDidEndTaskProcess(e => {
                if (e.execution === execution && e.exitCode === 0) {
                    vizProvider.sendMessage({ command: 'start' });
                    vscode.window.showInformationMessage('Build successful. Running AtomicTree code with live visualization...');

                    // Run the compiled executable in terminal
                    const exePath = activeEditor.document.fileName.replace('.cpp', '.exe');
                    const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('AtomicTree');

                    // Hook into terminal data to parse JSON
                    // Note: onDidWriteTerminalData is the correct API for intercepting terminal output
                    const disposable = (vscode.window as any).onDidWriteTerminalData((e: any) => {
                        if (e.terminal === terminal) {
                            parseTerminalData(e.data, vizProvider);
                        }
                    });
                    context.subscriptions.push(disposable);

                    terminal.show();
                    terminal.sendText(`& "${exePath}"`);
                }
            });
        })
    );
}

async function createBuildTask(context: vscode.ExtensionContext, fileUri: vscode.Uri): Promise<vscode.Task> {
    const enginePath = vscode.Uri.joinPath(context.extensionUri, 'engine');
    const includePath = vscode.Uri.joinPath(enginePath, 'include').fsPath;
    const srcPath = vscode.Uri.joinPath(enginePath, 'src').fsPath;

    const definition: vscode.TaskDefinition = {
        type: 'atomic-tree-build',
        file: fileUri.fsPath
    };

    const commandLine = `g++ -fdiagnostics-color=always -g "${fileUri.fsPath}" "${srcPath}\\*.cpp" -I"${includePath}" -o "${fileUri.fsPath.replace('.cpp', '.exe')}"`;

    return new vscode.Task(
        definition,
        vscode.TaskScope.Workspace,
        'AtomicTree Build',
        'AtomicTree',
        new vscode.ShellExecution(commandLine),
        ['$gcc']
    );
}

class AtomicTreeTaskProvider implements vscode.TaskProvider {
    constructor(private context: vscode.ExtensionContext) { }

    provideTasks(): vscode.ProviderResult<vscode.Task[]> {
        return [];
    }

    async resolveTask(_task: vscode.Task): Promise<vscode.Task | undefined> {
        const definition = _task.definition;
        if (definition.type === 'atomic-tree-build' && definition.file) {
            return createBuildTask(this.context, vscode.Uri.file(definition.file));
        }
        return undefined;
    }
}

let terminalBuffer = '';
function parseTerminalData(data: string, vizProvider: AtomicTreeWebviewProvider) {
    terminalBuffer += data;
    const lines = terminalBuffer.split(/\r?\n/);
    terminalBuffer = lines.pop() || '';

    for (const line of lines) {
        const trimmed = line.trim();
        if (trimmed.includes('{"type":') && trimmed.includes('}')) {
            try {
                const start = trimmed.indexOf('{');
                const end = trimmed.lastIndexOf('}');
                if (start !== -1 && end !== -1) {
                    const jsonStr = trimmed.substring(start, end + 1);
                    const json = JSON.parse(jsonStr);
                    vizProvider.sendMessage({ command: 'data', payload: json });
                }
            } catch (err) { }
        }
    }
}


class AtomicTreeWebviewProvider implements vscode.WebviewViewProvider {
    private _view?: vscode.WebviewView;

    constructor(
        private readonly _extensionUri: vscode.Uri,
        private readonly _type: 'controls' | 'visualizer'
    ) { }

    public resolveWebviewView(
        webviewView: vscode.WebviewView,
        context: vscode.WebviewViewResolveContext,
        _token: vscode.CancellationToken,
    ) {
        this._view = webviewView;

        webviewView.webview.options = {
            enableScripts: true,
            localResourceRoots: [
                vscode.Uri.joinPath(this._extensionUri, 'media'),
                vscode.Uri.joinPath(this._extensionUri, 'dist')
            ]
        };

        webviewView.webview.html = this._getHtmlForWebview(webviewView.webview);

        webviewView.webview.onDidReceiveMessage(data => {
            switch (data.command) {
                case 'runBenchmark':
                    vscode.commands.executeCommand('atomicTree.runCurrentFile');
                    break;
                case 'reset':
                    vscode.window.showInformationMessage('AtomicTree: Resetting memory state...');
                    // Logic to clear data files could go here
                    break;
            }
        });
    }

    public sendMessage(data: any) {
        if (this._view) {
            this._view.webview.postMessage(data);
        }
    }

    private _getHtmlForWebview(webview: vscode.Webview) {
        const scriptUri = webview.asWebviewUri(vscode.Uri.joinPath(this._extensionUri, 'dist', 'webview.js'));
        const styleUri = webview.asWebviewUri(vscode.Uri.joinPath(this._extensionUri, 'media', 'style.css'));

        return `<!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
             <!-- CSP allowing scripts and styles -->
             <meta http-equiv="Content-Security-Policy" content="default-src 'none'; style-src ${webview.cspSource} 'unsafe-inline'; script-src ${webview.cspSource} 'unsafe-inline';">
            <link href="${styleUri}" rel="stylesheet">
            <title>AtomicTree</title>
        </head>
        <body style="padding: 0; margin: 0; background: transparent;">
            <div id="root" data-view-type="${this._type}" style="height: 100vh; display: flex; flex-direction: column;">
                <div style="padding: 20px; color: #888;">Initializing AtomicTree ${this._type}...</div>
            </div>
            <script src="${scriptUri}"></script>
        </body>
        </html>`;
    }
}

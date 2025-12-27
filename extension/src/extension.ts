import * as vscode from 'vscode';
import * as path from 'path';

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

    // Register Task Provider
    context.subscriptions.push(
        vscode.tasks.registerTaskProvider('atomic-tree-build', new AtomicTreeTaskProvider(context))
    );

    context.subscriptions.push(
        vscode.commands.registerCommand('atomicTree.startProfile', () => {
            controlsProvider.sendMessage({ command: 'start' });
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

                    // Hook into terminal data to parse JSON output
                    // Using type assertion as onDidWriteTerminalData may not be in older VSCode API types
                    const disposable = (vscode.window as any).onDidWriteTerminalData?.((e: any) => {
                        if (e.terminal === terminal) {
                            parseTerminalData(e.data, vizProvider);
                        }
                    });
                    if (disposable) {
                        context.subscriptions.push(disposable);
                    }

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

    // Get all cpp files in src directory
    const cppFiles = [
        path.join(srcPath, 'manager.cpp'),
        path.join(srcPath, 'B_tree.cpp'),
        path.join(srcPath, 'primitives.cpp'),
        path.join(srcPath, 'garbage_collector.cpp')
    ].map(f => `"${f}"`).join(' ');

    const commandLine = `g++ -fdiagnostics-color=always -g "${fileUri.fsPath}" ${cppFiles} -I"${includePath}" -o "${fileUri.fsPath.replace('.cpp', '.exe')}"`;

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

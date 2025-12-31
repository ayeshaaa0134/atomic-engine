import * as vscode from 'vscode';
import { spawn, ChildProcess } from 'child_process';
import * as path from 'path';

let backendProcess: ChildProcess | undefined;

export function activate(context: vscode.ExtensionContext) {
    console.log('Atomic Tree Engine is active!');

    // 1. Register Views
    const provider = new AtomicTreeWebviewProvider(context.extensionUri);
    context.subscriptions.push(
        vscode.window.registerWebviewViewProvider('atomic-tree-dashboard', provider),
        vscode.window.registerWebviewViewProvider('atomic-tree-explorer', provider),
        vscode.window.registerWebviewViewProvider('atomic-tree-memory', provider),
        vscode.window.registerWebviewViewProvider('atomic-tree-radar', provider),
        vscode.window.registerWebviewViewProvider('atomic-tree-crashes', provider)
    );

    // 2. Register Commands
    context.subscriptions.push(vscode.commands.registerCommand('atomic-tree.startWorkload', () => {
        startBackend(context, provider);
        sendCommand({ method: 'runWorkload', params: { type: 'insert', count: 100 } });
        vscode.window.showInformationMessage('Atomic Tree Workload Started');
    }));

    context.subscriptions.push(vscode.commands.registerCommand('atomic-tree.stopWorkload', () => {
        sendCommand({ method: 'stopWorkload' });
    }));

    context.subscriptions.push(vscode.commands.registerCommand('atomic-tree.simulateCrash', () => {
        sendCommand({ method: 'simulateCrash' });
        vscode.window.showWarningMessage('Backend Crash Simulated!');
    }));
}

function startBackend(context: vscode.ExtensionContext, provider: AtomicTreeWebviewProvider) {
    if (backendProcess) return;

    const workspaceFolders = vscode.workspace.workspaceFolders;

    // 1. Check for custom backend path in settings
    const config = vscode.workspace.getConfiguration('atomicTree');
    let backendPath = config.get<string>('backendPath');

    if (backendPath && backendPath.trim() !== '') {
        // Support ${workspaceFolder} variable
        if (workspaceFolders && workspaceFolders.length > 0) {
            backendPath = backendPath.replace('${workspaceFolder}', workspaceFolders[0].uri.fsPath);
        }
        console.log(`Using custom backend: ${backendPath}`);
    } else {
        // 2. Default to bundled binary
        backendPath = vscode.Uri.joinPath(context.extensionUri, 'bin', 'atomic-engine.exe').fsPath;
        console.log(`Using bundled backend: ${backendPath}`);
    }

    if (!workspaceFolders) {
        vscode.window.showErrorMessage("Open a folder to start Atomic Engine");
        return;
    }

    // IMPORTANT: In a real extension, we'd bundle the binary or download it.

    try {
        // Run in workspace root so pmem.dat is created there
        const cwd = workspaceFolders[0].uri.fsPath;
        backendProcess = spawn(backendPath, [], { cwd });

        backendProcess.stdout?.on('data', (data) => {
            const str = data.toString();
            console.log(`Backend Out: ${str}`);

            // Parse JSON lines and forward to Webview
            const lines = str.split('\n');
            for (const line of lines) {
                if (!line.trim()) continue;
                try {
                    const json = JSON.parse(line);
                    provider.postMessage(json);
                } catch (e) {
                    // Ignore non-JSON output (debug logs)
                }
            }
        });

        backendProcess.stderr?.on('data', (data) => {
            console.error(`Backend Err: ${data}`);
        });

        backendProcess.on('close', (code) => {
            console.log(`Backend exited with code ${code}`);
            backendProcess = undefined;
            vscode.window.showErrorMessage(`Atomic Engine died (Exit Code: ${code})`);
        });

    } catch (e) {
        vscode.window.showErrorMessage('Failed to start Atomic Engine backend');
    }
}

function sendCommand(cmd: any) {
    if (backendProcess && backendProcess.stdin) {
        backendProcess.stdin.write(JSON.stringify(cmd) + '\n');
    }
}

export function deactivate() {
    if (backendProcess) {
        backendProcess.kill();
    }
}

class AtomicTreeWebviewProvider implements vscode.WebviewViewProvider {
    private _views = new Set<vscode.WebviewView>();

    constructor(private readonly _extensionUri: vscode.Uri) { }

    resolveWebviewView(webviewView: vscode.WebviewView) {
        this._views.add(webviewView);

        webviewView.onDidDispose(() => {
            this._views.delete(webviewView);
        });

        webviewView.webview.options = {
            enableScripts: true,
            localResourceRoots: [this._extensionUri]
        };

        webviewView.webview.html = this._getHtmlForWebview(webviewView.webview);
    }

    public postMessage(message: any) {
        for (const view of this._views) {
            view.webview.postMessage(message);
        }
    }

    private _getHtmlForWebview(webview: vscode.Webview) {
        return `<!DOCTYPE html>
        <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>Atomic Tree Dashboard</title>
            <style>
                body { font-family: sans-serif; padding: 10px; }
                .card { background: var(--vscode-editor-background); border: 1px solid var(--vscode-widget-border); padding: 10px; margin-bottom: 10px; }
            </style>
        </head>
        <body>
            <div class="card">
                <h3>Throughput</h3>
                <div id="chart-throughput" style="height: 100px;">Waiting for data...</div>
            </div>
            <script>
                // Listen for messages from extension
                window.addEventListener('message', event => {
                    const message = event.data;
                    document.getElementById('chart-throughput').innerText = JSON.stringify(message, null, 2);
                });
            </script>
        </body>
        </html>`;
    }
}

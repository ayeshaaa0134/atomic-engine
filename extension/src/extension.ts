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

    context.subscriptions.push(
        vscode.commands.registerCommand('atomicTree.startProfile', () => {
            // Auto-detect running AtomicTree program from terminal
            const terminal = vscode.window.activeTerminal || vscode.window.createTerminal('AtomicTree');

            controlsProvider.sendMessage({ command: 'start' });
            vizProvider.sendMessage({ command: 'start' });
            vscode.window.showInformationMessage('AtomicTree Profile Started - Extension listening for metrics');

            // Listen to terminal output
            listenToTerminalOutput(vizProvider);
        }),
        vscode.commands.registerCommand('atomicTree.stopProfile', () => {
            controlsProvider.sendMessage({ command: 'stop' });
            vizProvider.sendMessage({ command: 'stop' });
            vscode.window.showInformationMessage('AtomicTree Profile Stopped');
        }),
        vscode.commands.registerCommand('atomicTree.runCurrentFile', async () => {
            // Build and run current C++ file with AtomicTree
            await vscode.commands.executeCommand('workbench.action.tasks.runTask',
                'Run AtomicTree with Extension');

            // Start visualization
            vizProvider.sendMessage({ command: 'start' });
            vscode.window.showInformationMessage('Running AtomicTree code with live visualization...');
        })
    );
}

function listenToTerminalOutput(vizProvider: AtomicTreeWebviewProvider) {
    // This would parse JSON-L output from terminal
    // For now, we'll simulate with the vizProvider
    // In production, use vscode.window.onDidWriteTerminalData
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
                    vscode.window.showInformationMessage(`Starting AtomicTree Benchmark (${this._type})...`);
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

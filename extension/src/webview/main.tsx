import React, { useState, useEffect, useRef } from 'react';
import ReactDOM from 'react-dom/client';
import {
    VSCodeButton,
    VSCodeDivider,
    VSCodeCheckbox
} from '@vscode/webview-ui-toolkit/react';

const vscode = (window as any).acquireVsCodeApi();

// --- ROOT APP ---
const App = () => {
    const viewType = document.getElementById('root')?.getAttribute('data-view-type');

    if (viewType === 'controls') return <ControlsPanel />;
    if (viewType === 'visualizer') return <DashboardPanel />;

    return <DashboardPanel />;
};

// --- LEFT PANEL: CONTROLS ---
const ControlsPanel = () => {
    const [isRunning, setIsRunning] = useState(false);

    const handleStart = () => {
        setIsRunning(true);
        vscode.postMessage({ command: 'runBenchmark' });
    };

    const handleReset = () => {
        setIsRunning(false);
        vscode.postMessage({ command: 'reset' });
    };

    return (
        <div className="flex col gap-4 p-4 h-full">
            <div className="flex row items-center gap-2">
                <div className={isRunning ? "spinner" : "status-dot"}></div>
                <h2 className="text-sm font-bold uppercase tracking-wide">
                    {isRunning ? "Engine Status: Active" : "Engine Status: Idle"}
                </h2>
            </div>
            <VSCodeDivider />

            <div className="control-group flex col gap-2">
                <label className="control-label">B-TREE PARAMETERS</label>
                <div className="text-xs opacity-70 mb-1">Fanout & Leaf Capacity are handled in code via BTreeConfig.</div>
                <div className="p-2 border rounded bg-vscode-editor-background opacity-80 text-xs font-mono">
                    max_keys: 16<br />
                    min_keys: 8<br />
                    leaf_cap: 32
                </div>
            </div>

            <div className="control-group flex col gap-2 border-l-2" style={{ borderLeftColor: 'var(--vscode-debugIcon-breakpointForeground)' }}>
                <label className="control-label">DURABILITY TESTING</label>
                <VSCodeCheckbox>Enable Shadow Paging</VSCodeCheckbox>
                <div className="text-xs opacity-50">Ensures consistency via WORT (Write-Optimal Radix Tree) principles.</div>
            </div>

            <div className="grow"></div>

            <VSCodeButton
                appearance="primary"
                className="w-full"
                disabled={isRunning}
                onClick={handleStart}
            >
                <span className="codicon codicon-play mr-2"></span> RUN PROGRAM
            </VSCodeButton>
            <VSCodeButton
                appearance="secondary"
                className="w-full"
                onClick={handleReset}
            >
                RESET NVM STATE
            </VSCodeButton>
        </div>
    );
};

// --- RIGHT PANEL: Dashboard ---
const DashboardPanel = () => {
    const [tab, setTab] = useState<'live' | 'mem'>('live');
    const [stats, setStats] = useState({ ops: 0, latency: 0, writeAmp: 1.0 });
    const [history, setHistory] = useState<number[]>(new Array(60).fill(0));
    const [logs, setLogs] = useState<any[]>([]);
    const [memoryState, setMemoryState] = useState<number[]>([]);

    useEffect(() => {
        const handleMessage = (event: MessageEvent) => {
            const message = event.data;
            if (message.command === 'data') {
                const payload = message.payload;

                if (payload.type === 'metric') {
                    if (payload.name === 'throughput') {
                        setStats(s => ({ ...s, ops: payload.value }));
                        setHistory(h => [...h.slice(1), payload.value]);
                    } else if (payload.name === 'latency') {
                        setStats(s => ({ ...s, latency: payload.value }));
                    } else if (payload.name === 'write_amp') {
                        setStats(s => ({ ...s, writeAmp: payload.value }));
                    }
                } else if (payload.type === 'log') {
                    setLogs(l => [{ time: new Date().toLocaleTimeString(), ...payload }, ...l.slice(0, 49)]);
                } else if (payload.type === 'init') {
                    setLogs(l => [{ time: new Date().toLocaleTimeString(), level: 'INFO', message: `Engine Initialized: ${payload.name}` }, ...l]);
                    setStats({ ops: 0, latency: 0, writeAmp: 1.0 });
                    setHistory(new Array(60).fill(0));
                }
            }
        };

        window.addEventListener('message', handleMessage);
        return () => window.removeEventListener('message', handleMessage);
    }, []);

    return (
        <div className="flex col h-full bg-bg-color">
            <div className="tab-bar shrink-0">
                <Tab id="live" label="LIVE TELEMETRY" active={tab} onClick={setTab} />
                <Tab id="mem" label="MEMORY MAP" active={tab} onClick={setTab} />
            </div>

            <div className="grow overflow-y-auto p-4 flex col gap-4">
                {tab === 'live' && <LiveTelemetry stats={stats} history={history} logs={logs} />}
                {tab === 'mem' && <MemoryHexView memory={memoryState} />}
            </div>
        </div>
    );
};

const Tab = ({ id, label, active, onClick }: any) => (
    <div className={`tab ${active === id ? 'active' : ''}`} onClick={() => onClick(id)}>
        {label}
    </div>
);

// --- VIEW 1: LIVE TELEMETRY ---
const LiveTelemetry = ({ stats, history, logs }: any) => (
    <>
        <div className="grid grid-cols-3 gap-2" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr' }}>
            <MetricCard label="THROUGHPUT" value={(stats.ops / 1000).toFixed(1) + 'k'} unit="ops/s" color="var(--chart-blue)" />
            <MetricCard label="LATENCY" value={stats.latency.toFixed(1)} unit="µs" color="var(--chart-green)" />
            <MetricCard label="WRITE AMP" value={stats.writeAmp.toFixed(2)} unit="x" color="var(--chart-purple)" />
        </div>

        <div className="chart-container h-48 p-2 flex col">
            <div className="text-xs font-bold opacity-50 mb-2 uppercase">Engine IOPS (Throughput)</div>
            <Sparkline data={history} color="var(--chart-blue)" />
        </div>

        <div className="flex col gap-2">
            <div className="text-xs font-bold opacity-50 uppercase">System Logs</div>
            <div className="border rounded bg-vscode-editor-background h-48 overflow-y-auto flex col text-xs font-mono">
                {logs.length === 0 ? (
                    <div className="p-4 opacity-30 italic">Waiting for backend stream...</div>
                ) : (
                    logs.map((log: any, i: number) => (
                        <LogRow key={i} time={log.time} type={log.level || 'INFO'} msg={log.message} />
                    ))
                )}
            </div>
        </div>
    </>
);

const MetricCard = ({ label, value, unit, color }: any) => (
    <div className="metric-card border-l-4" style={{ borderLeftColor: color }}>
        <div className="text-xs opacity-60 font-bold mb-1">{label}</div>
        <div className="text-xl font-mono font-bold leading-none">{value}</div>
        <div className="text-[10px] opacity-50 font-normal uppercase mt-1">{unit}</div>
    </div>
);

const LogRow = ({ time, type, msg }: any) => (
    <div className="log-entry flex row border-b border-vscode-widget-border py-1 px-2">
        <span className="opacity-40 w-16 shrink-0">{time}</span>
        <span className={`log-${type.toLowerCase()} w-10 shrink-0 font-bold`}>{type}</span>
        <span className="opacity-80 ml-2">{msg}</span>
    </div>
);

// --- VIEW 2: MEMORY MAP ---
const MemoryHexView = ({ memory }: any) => {
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        const ctx = canvasRef.current?.getContext('2d');
        if (!ctx) return;

        const w = ctx.canvas.width;
        const h = ctx.canvas.height;
        const cols = 40;
        const rows = 20;
        const cellW = w / cols;
        const cellH = h / rows;

        ctx.clearRect(0, 0, w, h);

        for (let i = 0; i < cols * rows; i++) {
            const x = (i % cols) * cellW;
            const y = Math.floor(i / cols) * cellH;

            let color = '#1a1a1a'; // Unmapped
            if (i < 5) color = '#f14c4c'; // Header
            else if (i < 45) color = '#3794ff'; // Bitmap
            else if (i % 7 === 0) color = '#4ec9b0'; // Allocated Block

            ctx.fillStyle = color;
            ctx.fillRect(x + 1, y + 1, cellW - 2, cellH - 2);
        }
    }, [memory]);

    return (
        <div className="flex col gap-2 h-full">
            <div className="flex row justify-between text-xs font-mono opacity-50">
                <span>NVM_START: 0x0000</span>
                <span>NVM_END: 0xFFFF</span>
            </div>
            <div className="grow border border-vscode-widget-border bg-black rounded overflow-hidden relative p-2">
                <canvas ref={canvasRef} className="w-full h-full" width={400} height={200} />
            </div>
            <div className="flex row gap-4 text-[10px] uppercase font-bold opacity-60">
                <div className="flex row items-center gap-1"><div className="w-2 h-2" style={{ background: '#f14c4c' }}></div> Header</div>
                <div className="flex row items-center gap-1"><div className="w-2 h-2" style={{ background: '#3794ff' }}></div> Bitmap</div>
                <div className="flex row items-center gap-1"><div className="w-2 h-2" style={{ background: '#4ec9b0' }}></div> Data</div>
                <div className="flex row items-center gap-1"><div className="w-2 h-2" style={{ background: '#1a1a1a' }}></div> Unmapped</div>
            </div>
        </div>
    );
};

// --- UTILS ---
const Sparkline = ({ data, color }: any) => {
    const max = Math.max(...data, 1);
    const min = Math.min(...data);
    const range = max - min || 1;

    const points = data.map((d: number, i: number) => {
        const x = (i / (data.length - 1)) * 100;
        const y = 100 - ((d - min) / range) * 100;
        return `${x},${y}`;
    }).join(' ');

    return (
        <div className="w-full h-full border-b border-l border-vscode-widget-border relative">
            <svg viewBox="0 0 100 100" preserveAspectRatio="none" className="w-full h-full overflow-visible">
                <polyline fill="none" stroke={color} strokeWidth="2" points={points} vectorEffect="non-scaling-stroke" />
            </svg>
        </div>
    );
};

const rootElement = document.getElementById('root') as HTMLElement;
const root = ReactDOM.createRoot(rootElement);
root.render(<App />);

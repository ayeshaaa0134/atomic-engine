import React, { useState, useEffect, useRef, useMemo } from 'react';
import ReactDOM from 'react-dom/client';
import {
    VSCodeButton,
    VSCodeDivider,
    VSCodeCheckbox,
    VSCodeBadge
} from '@vscode/webview-ui-toolkit/react';

const vscode = (window as any).acquireVsCodeApi();

// --- ROOT APP ---
const App = () => {
    const viewType = document.getElementById('root')?.getAttribute('data-view-type');
    return <DashboardPanel mode={viewType} />;
};

// --- Dashboard Panel (Unified for consistency) ---
const DashboardPanel = ({ mode }: any) => {
    const [tab, setTab] = useState<'live' | 'research' | 'mem'>(mode === 'controls' ? 'live' : 'live');
    const [isRunning, setIsRunning] = useState(false);
    const [stats, setStats] = useState({
        ops: 0,
        latency: 0,
        physical_writes: 0,
        logical_writes: 0,
        allocated_blocks: 0,
        treeType: 'B+ Tree',
        consistency: 'Shadow Paging',
        integrity: 'UNKNOWN',
        region_kb: 0,
        block_size: 0
    });
    const [systemMetrics, setSystemMetrics] = useState<any>({});
    const [history, setHistory] = useState<number[]>(new Array(60).fill(0));
    const [logs, setLogs] = useState<any[]>([]);
    const [bitmapHex, setBitmapHex] = useState('');

    useEffect(() => {
        const handleMessage = (event: MessageEvent) => {
            const message = event.data;
            if (message.command === 'data') {
                const payload = message.payload;

                if (payload.type === 'metric') {
                    setStats(s => ({
                        ...s,
                        ops: payload.ops,
                        latency: payload.latency,
                        physical_writes: payload.physical_writes,
                        logical_writes: payload.logical_writes || (payload.ops * 16),
                        allocated_blocks: payload.allocated_blocks,
                        integrity: payload.integrity || s.integrity,
                        region_kb: payload.region_kb || s.region_kb,
                        block_size: payload.block_size || s.block_size
                    }));
                    setHistory(h => [...h.slice(1), payload.ops]);
                } else if (payload.type === 'bitmap') {
                    setBitmapHex(payload.data);
                } else if (payload.type === 'log') {
                    setLogs(l => [{ time: new Date().toLocaleTimeString(), ...payload }, ...l.slice(0, 49)]);
                }
            } else if (message.command === 'systemStats') {
                setSystemMetrics(message.payload);
            }
        };

        window.addEventListener('message', handleMessage);
        return () => window.removeEventListener('message', handleMessage);
    }, []);

    const writeAmp = useMemo(() => {
        if (stats.logical_writes === 0) return 1.0;
        return Math.max(1.0, stats.physical_writes / stats.logical_writes);
    }, [stats.physical_writes, stats.logical_writes]);

    if (mode === 'controls') return <ControlsPanel isRunning={isRunning} setIsRunning={setIsRunning} systemMetrics={systemMetrics} stats={stats} />;

    return (
        <div className="flex col h-full bg-bg-color">
            <div className="tab-bar shrink-0">
                <Tab id="live" label="DASHBOARD" active={tab} onClick={setTab} />
                <Tab id="research" label="RESEARCH" active={tab} onClick={setTab} />
                <Tab id="mem" label="MEMORY ARCH" active={tab} onClick={setTab} />
            </div>

            <div className="grow overflow-y-auto p-4 flex col gap-4">
                {tab === 'live' && (
                    <LiveTelemetry
                        stats={stats}
                        writeAmp={writeAmp}
                        history={history}
                        logs={logs}
                    />
                )}
                {tab === 'research' && <ResearchPanel stats={stats} writeAmp={writeAmp} />}
                {tab === 'mem' && <MemoryMapHex bitmapHex={bitmapHex} />}
            </div>
        </div>
    );
};

// --- CONTROLS PANEL ---
const ControlsPanel = ({ isRunning, setIsRunning, systemMetrics, stats }: any) => {
    const handleStart = () => {
        setIsRunning(true);
        vscode.postMessage({ command: 'runBenchmark' });
    };

    const handleReset = () => {
        setIsRunning(false);
        vscode.postMessage({ command: 'reset' });
    };

    return (
        <div className="flex col gap-4 p-4 h-full glass overflow-y-auto">
            <div className="flex row items-center gap-2">
                <div className={isRunning ? "spinner" : "status-dot"}></div>
                <h2 className="text-sm font-bold uppercase tracking-wide">
                    {isRunning ? "Engine: Active" : "Engine: Idle"}
                </h2>
            </div>

            <div className="system-specs flex col gap-3 p-3 bg-white/5 rounded border border-white/10">
                <div className="flex row justify-between items-center text-[10px] uppercase opacity-50 font-bold">
                    <span>Host Environment</span>
                    <VSCodeBadge>{systemMetrics.storageType || 'Detecting...'}</VSBadge>
                </div>

                <div className="flex col gap-2">
                    <div className="flex col">
                        <div className="flex row justify-between text-[11px] mb-1">
                            <span>CPU ({systemMetrics.cpuCores} Cores)</span>
                            <span className="font-mono">{systemMetrics.cpuUsage?.toFixed(1)}%</span>
                        </div>
                        <div className="usage-bar-bg h-1.5 w-full bg-black/20 rounded-full overflow-hidden">
                            <div className="h-full bg-chart-blue" style={{ width: `${systemMetrics.cpuUsage}%`, transition: 'width 0.5s ease' }}></div>
                        </div>
                    </div>

                    <div className="flex col">
                        <div className="flex row justify-between text-[11px] mb-1">
                            <span>Memory ({systemMetrics.totalMemoryGB} GB)</span>
                            <span className="font-mono">{((parseFloat(systemMetrics.usedMemoryGB) / parseFloat(systemMetrics.totalMemoryGB)) * 100).toFixed(1)}%</span>
                        </div>
                        <div className="usage-bar-bg h-1.5 w-full bg-black/20 rounded-full overflow-hidden">
                            <div className="h-full bg-chart-green" style={{ width: `${(parseFloat(systemMetrics.usedMemoryGB) / parseFloat(systemMetrics.totalMemoryGB)) * 100}%`, transition: 'width 0.5s ease' }}></div>
                        </div>
                    </div>
                </div>

                {systemMetrics.storageModel && (
                    <div className="text-[9px] opacity-40 truncate font-mono mt-1 bg-black/20 p-1 rounded">
                        {systemMetrics.storageModel}
                    </div>
                )}
            </div>

            <div className="control-group flex col gap-2 border-l-2" style={{ borderLeftColor: 'var(--chart-blue)' }}>
                <label className="control-label">NVM ENGINE CONFIG</label>
                <div className="flex col gap-1">
                    <div className="flex row justify-between text-[10px] opacity-60">
                        <span>Consistency</span>
                        <span style={{ color: 'var(--chart-green)' }}>{stats.consistency || 'Shadow Paging'}</span>
                    </div>
                    <div className="flex row justify-between text-[10px] opacity-60">
                        <span>Region Size</span>
                        <span>{stats.region_kb > 0 ? `${(stats.region_kb / 1024).toFixed(0)} MB` : '32 MB'}</span>
                    </div>
                    <div className="flex row justify-between text-[10px] opacity-60">
                        <span>Block Alignment</span>
                        <span>{stats.block_size > 0 ? `${stats.block_size} B` : '128 B'}</span>
                    </div>
                    <div className="flex row justify-between text-[10px] opacity-60">
                        <span>NVM Integrity</span>
                        <span style={{ color: stats.integrity === 'PASSED' ? 'var(--chart-green)' : 'var(--chart-red)', fontWeight: 'bold' }}>
                            {stats.integrity}
                        </span>
                    </div>
                </div>
            </div>

            <div className="mt-auto flex col gap-2">
                <VSCodeButton appearance="primary" className="w-full" onClick={handleStart} disabled={isRunning}>
                    RUN BENCHMARK
                </VSCodeButton>
                <VSCodeButton appearance="secondary" className="w-full" onClick={handleReset}>
                    RESET ENGINE
                </VSCodeButton>
            </div>
        </div>
    );
};

// --- LIVE TELEMETRY ---
const LiveTelemetry = ({ stats, writeAmp, history, logs }: any) => (
    <>
        <div className="grid grid-cols-3 gap-2" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr 1fr' }}>
            <MetricCard label="THROUGHPUT" value={(stats.ops / 1000).toFixed(1) + 'k'} unit="ops/s" color="var(--chart-blue)" />
            <MetricCard label="WRITE AMP" value={writeAmp.toFixed(2)} unit="x factor" color="var(--chart-purple)" />
            <MetricCard label="LATENCY" value={stats.latency.toFixed(1)} unit="µs (avg)" color="var(--chart-green)" />
        </div>

        <div className="chart-container h-48 p-2 flex col glass">
            <div className="text-xs font-bold opacity-50 mb-2 uppercase flex row justify-between">
                <span>Real-Time IOPS</span>
                <VSCodeBadge>{(stats.ops).toLocaleString()} OPS</VSCodeBadge>
            </div>
            <Sparkline data={history} color="var(--chart-blue)" />
        </div>

        <div className="flex col gap-2">
            <div className="text-xs font-bold opacity-50 uppercase">Engine Log Stream</div>
            <div className="border rounded bg-vscode-editor-background h-48 overflow-y-auto flex col text-xs font-mono">
                {logs.length === 0 ? (
                    <div className="p-4 opacity-30 italic">Connect to NVM backend...</div>
                ) : (
                    logs.map((log: any, i: number) => (
                        <LogRow key={i} time={log.time} type={log.level || 'INFO'} msg={log.message} />
                    ))
                )}
            </div>
        </div>
    </>
);

// --- RESEARCH PANEL ---
const ResearchPanel = ({ stats, writeAmp }: any) => (
    <div className="flex col gap-4 glass p-4 rounded">
        <h3 className="text-sm font-bold border-b pb-2">RESEARCH SPECIFICATIONS</h3>

        <div className="flex col gap-2">
            <div className="text-xs font-bold opacity-70">Architecture: {stats.treeType}</div>
            <p className="text-[11px] opacity-60">
                Implements <strong>NV-Tree (FAST '15)</strong> semantics using unsorted leaf nodes to reduce cache line flushes.
            </p>
        </div>

        <div className="flex col gap-2">
            <div className="text-xs font-bold opacity-70">Consistency: {stats.consistency}</div>
            <p className="text-[11px] opacity-60">
                Leverages <strong>WORT (FAST '17)</strong> principles for write-optimality. Uses 8-byte atomic pointer swaps to transition between consistent states without write-ahead logs.
            </p>
        </div>

        <div className="control-group mt-2">
            <div className="text-xs font-bold mb-1">Write Amplification Analysis</div>
            <div className="flex row items-end gap-1">
                <div className="text-2xl font-bold">{writeAmp.toFixed(2)}x</div>
                <div className="text-[10px] mb-1 opacity-50">Physical/Logical Ratio</div>
            </div>
            <div className="w-full bg-vscode-editor-background h-2 rounded mt-2 overflow-hidden">
                <div
                    className="h-full bg-chart-purple"
                    style={{
                        width: `${Math.min(100, (1 / writeAmp) * 100)}%`,
                        background: 'var(--chart-purple)'
                    }}
                ></div>
            </div>
            <div className="text-[9px] opacity-40 mt-1">Target: 1.0x (Direct persist)</div>
        </div>
    </div>
);

// --- MEMORY MAP HEX ---
const MemoryMapHex = ({ bitmapHex }: any) => {
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        const ctx = canvasRef.current?.getContext('2d');
        if (!ctx || !bitmapHex) return;

        const w = ctx.canvas.width;
        const h = ctx.canvas.height;
        const cellSize = 10;
        const cols = Math.floor(w / cellSize);

        ctx.clearRect(0, 0, w, h);

        // Decode hex to bits
        let bitIndex = 0;
        for (let i = 0; i < bitmapHex.length; i++) {
            const hex = parseInt(bitmapHex[i], 16);
            for (let bit = 3; bit >= 0; bit--) {
                const isSet = (hex & (1 << bit)) !== 0;

                const x = (bitIndex % cols) * cellSize;
                const y = Math.floor(bitIndex / cols) * cellSize;

                ctx.fillStyle = isSet ? '#4ec9b0' : '#2d2d2d';
                ctx.fillRect(x + 1, y + 1, cellSize - 2, cellSize - 2);

                bitIndex++;
                if (bitIndex > 1024) break;
            }
        }
    }, [bitmapHex]);

    return (
        <div className="flex col gap-4 h-full grow">
            <div className="text-xs font-bold opacity-50 uppercase">NVM Block Allocation (First 1k Blocks)</div>
            <div className="border rounded bg-black relative p-2 overflow-hidden flex items-center justify-center">
                <canvas ref={canvasRef} width={300} height={300} style={{ width: '100%', imageRendering: 'pixelated' }} />
            </div>
            <div className="flex row gap-4 text-[10px] uppercase font-bold opacity-60">
                <div className="flex row items-center gap-1"><div className="w-2 h-2" style={{ background: '#4ec9b0' }}></div> Allocated</div>
                <div className="flex row items-center gap-1"><div className="w-2 h-2" style={{ background: '#2d2d2d' }}></div> Free</div>
            </div>
        </div>
    );
};

// --- COMPONENTS ---
const Tab = ({ id, label, active, onClick }: any) => (
    <div className={`tab ${active === id ? 'active' : ''}`} onClick={() => onClick(id)}>
        {label}
    </div>
);

const MetricCard = ({ label, value, unit, color }: any) => (
    <div className="metric-card border-l-4 glass" style={{ borderLeftColor: color }}>
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
        <div className="w-full h-full border-b border-l border-vscode-widget-border relative overflow-hidden">
            <svg viewBox="0 0 100 100" preserveAspectRatio="none" className="w-full h-full overflow-visible">
                <polyline fill="none" stroke={color} strokeWidth="2" points={points} vectorEffect="non-scaling-stroke" />
            </svg>
        </div>
    );
};

const rootElement = document.getElementById('root') as HTMLElement;
const root = ReactDOM.createRoot(rootElement);
root.render(<App />);

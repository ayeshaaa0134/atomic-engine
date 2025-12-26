import React, { useState, useEffect, useRef, useMemo } from 'react';
import ReactDOM from 'react-dom/client';
import {
    VSCodeButton,
    VSCodeDropdown,
    VSCodeOption,
    VSCodeCheckbox,
    VSCodeDivider,
    VSCodeTag
} from '@vscode/webview-ui-toolkit/react';

// --- DATA SIMULATION ENGINE ---
// generates plausible B-Tree signals
const useDataEngine = (active: boolean) => {
    const [stats, setStats] = useState({ ops: 0, latency: 15, writeAmp: 1.1 });
    const [history, setHistory] = useState<number[]>(new Array(60).fill(0));

    useEffect(() => {
        if (!active) return;
        const interval = setInterval(() => {
            setStats(prev => ({
                ops: 120000 + Math.random() * 30000,
                latency: 10 + Math.random() * 8, // micro-seconds
                writeAmp: 1.0 + Math.random() * 0.2
            }));
            setHistory(h => [...h.slice(1), 120000 + Math.random() * 30000]);
        }, 500);
        return () => clearInterval(interval);
    }, [active]);

    return { stats, history };
};

// --- ROOT APP ---
const App = () => {
    const viewType = document.getElementById('root')?.getAttribute('data-view-type');

    // View Routing
    if (viewType === 'controls') return <ControlsPanel />;
    if (viewType === 'visualizer') return <DashboardPanel />;

    // FallbackDevMode
    return <DashboardPanel />;
};

// --- LEFT PANEL: CONTROLS ---
const ControlsPanel = () => {
    return (
        <div className="flex col gap-4 p-4 h-full">
            <div className="flex row items-center gap-2">
                <div className="spinner"></div>
                <h2 className="text-sm font-bold uppercase tracking-wide">Engine Controller</h2>
            </div>
            <VSCodeDivider />

            <div className="control-group flex col gap-2">
                <label className="control-label">WORKLOAD PRESET</label>
                <VSCodeDropdown style={{ width: '100%' }} value="oltp">
                    <VSCodeOption value="oltp">OLTP (Write-Heavy)</VSCodeOption>
                    <VSCodeOption value="analytics">Analytics (Read-Only)</VSCodeOption>
                    <VSCodeOption value="blob">Large Blobs</VSCodeOption>
                </VSCodeDropdown>
            </div>

            <div className="control-group flex col gap-2">
                <label className="control-label">KEY CONFIGURATION</label>
                <div className="flex row justify-between text-xs font-mono opacity-70">
                    <span>10K</span>
                    <span>100M</span>
                </div>
                <input type="range" className="w-full" />
                <div className="flex row gap-2 items-center mt-1">
                    <VSCodeCheckbox checked>Zipfian Dist</VSCodeCheckbox>
                </div>
            </div>

            <div className="control-group flex col gap-2 border-l-2" style={{ borderLeftColor: '#f14c4c' }}>
                <label className="control-label text-chart-red">CHAOS ENGINEERING</label>
                <VSCodeCheckbox>Inject Random Crashes</VSCodeCheckbox>
                <div className="text-xs opacity-50">Probability per split: 0.1%</div>
            </div>

            <div className="grow"></div>

            <VSCodeButton appearance="primary" className="w-full">
                <span className="codicon codicon-play mr-2"></span> START BENCHMARK
            </VSCodeButton>
            <VSCodeButton appearance="secondary" className="w-full">
                RESET ENGINE
            </VSCodeButton>
        </div>
    );
};

// --- RIGHT PANEL: VISUALIZER ---
const DashboardPanel = () => {
    const [tab, setTab] = useState<'live' | 'compare' | 'mem'>('live');
    const { stats, history } = useDataEngine(true);

    return (
        <div className="flex col h-full bg-bg-color">
            <div className="tab-bar shrink-0">
                <Tab id="live" label="LIVE TELEMETRY" active={tab} onClick={setTab} />
                <Tab id="compare" label="COMPARISON" active={tab} onClick={setTab} />
                <Tab id="mem" label="MEMORY MAP" active={tab} onClick={setTab} />
            </div>

            <div className="grow overflow-y-auto p-4 flex col gap-4">
                {tab === 'live' && <LiveTelemetry stats={stats} history={history} />}
                {tab === 'compare' && <ComparisonView />}
                {tab === 'mem' && <MemoryHexView />}
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
const LiveTelemetry = ({ stats, history }: any) => (
    <>
        <div className="grid grid-cols-2 gap-2" style={{ display: 'grid', gridTemplateColumns: '1fr 1fr' }}>
            <MetricCard label="THROUGHPUT" value={(stats.ops / 1000).toFixed(1) + 'k'} unit="ops/s" color="var(--chart-blue)" />
            <MetricCard label="LATENCY (p99)" value={stats.latency.toFixed(1)} unit="µs" color="var(--chart-green)" />
            <MetricCard label="WRITE AMP" value={stats.writeAmp.toFixed(2)} unit="x" color="var(--chart-purple)" />
            <MetricCard label="BUFFER MISS" value="0.01" unit="%" color="var(--chart-red)" />
        </div>

        <div className="chart-container h-48 p-2 flex col">
            <div className="text-xs font-bold opacity-50 mb-2 uppercase">Real-time IOPS</div>
            <Sparkline data={history} color="var(--chart-blue)" />
        </div>

        <div className="flex col gap-2">
            <div className="text-xs font-bold opacity-50 uppercase">Engine Events</div>
            <div className="border rounded bg-vscode-editor-background h-32 overflow-hidden flex col">
                <LogRow time="10:42:05" type="INFO" msg="B-Tree Split at Node 0x4F20" />
                <LogRow time="10:42:05" type="INFO" msg="WAL Skipped (Atomic Mode)" />
                <LogRow time="10:42:06" type="WARN" msg="NVM High Latency Block Detected" />
                <LogRow time="10:42:08" type="INFO" msg="GC Cycle: Reclaimed 12 blocks" />
            </div>
        </div>
    </>
);

const MetricCard = ({ label, value, unit, color }: any) => (
    <div className="metric-card border-l-4" style={{ borderLeftColor: color }}>
        <div className="text-xs opacity-60 font-bold mb-1">{label}</div>
        <div className="text-xl font-mono font-bold">{value} <span className="text-sm opacity-50 font-normal">{unit}</span></div>
    </div>
);

const LogRow = ({ time, type, msg }: any) => (
    <div className="log-entry flex row">
        <span className="log-time">{time}</span>
        <span className={`log-${type.toLowerCase()} w-10 shrink-0`}>{type}</span>
        <span className="opacity-80">{msg}</span>
    </div>
);

// --- VIEW 2: COMPARISON ---
const ComparisonView = () => (
    <div className="flex col gap-6">
        <Banner text="AtomicTree vs Traditional Engines (1M Keys, 50% Write)" />

        <CompareBar title="Throughput (Higher is Better)" items={[
            { label: 'AtomicTree', value: 100, display: '152k', color: 'var(--chart-blue)' },
            { label: 'PMDK (Intel)', value: 70, display: '105k', color: '#555' },
            { label: 'SQLite', value: 30, display: '45k', color: '#555' },
        ]} />

        <CompareBar title="Time to Recovery (Lower is Better)" items={[
            { label: 'AtomicTree', value: 5, display: '0.2ms', color: 'var(--chart-green)' },
            { label: 'PMDK', value: 15, display: '50ms', color: '#555' },
            { label: 'SQLite', value: 100, display: '1200ms', color: '#555' },
        ]} />
    </div>
);

const CompareBar = ({ title, items }: any) => (
    <div className="flex col gap-2">
        <div className="text-xs font-bold uppercase">{title}</div>
        {items.map((it: any, i: number) => (
            <div key={i} className="flex row items-center gap-2 text-xs">
                <div className="w-20 text-right opacity-70 font-bold">{it.label}</div>
                <div className="grow h-6 bg-vscode-widget-border rounded overflow-hidden relative">
                    <div style={{ width: `${it.value}%`, background: it.color }} className="h-full flex items-center px-2 text-bg-color font-bold">
                        {it.display}
                    </div>
                </div>
            </div>
        ))}
    </div>
);

// --- VIEW 3: MEMORY MAP (CANVAS) ---
const MemoryHexView = () => {
    const canvasRef = useRef<HTMLCanvasElement>(null);

    useEffect(() => {
        const ctx = canvasRef.current?.getContext('2d');
        if (!ctx) return;

        let frameId = 0;
        const draw = () => {
            const w = ctx.canvas.width;
            const h = ctx.canvas.height;
            const cols = 50;
            const cellW = w / cols;
            const cellH = 8;

            ctx.clearRect(0, 0, w, h);

            for (let i = 0; i < 800; i++) {
                const x = (i % cols) * cellW;
                const y = Math.floor(i / cols) * (cellH + 1);

                // Animated flickering effect
                const noise = Math.random();
                let color = '#333';
                if (noise > 0.98) color = '#f14c4c'; // Leak
                else if (noise > 0.8) color = '#3794ff'; // Alloc
                else if (noise > 0.6) color = '#2d2d2d'; // Free

                if (i === 402) color = '#fff'; // Cursor

                ctx.fillStyle = color;
                ctx.fillRect(x, y, cellW - 1, cellH);
            }
            frameId = requestAnimationFrame(() => setTimeout(draw, 100));
        };
        draw();
        return () => cancelAnimationFrame(frameId);
    }, []);

    return (
        <div className="flex col gap-2 h-full">
            <div className="flex row justify-between text-xs font-mono opacity-50">
                <span>HEAP_START: 0x1000</span>
                <span>HEAP_END: 0xFFFF</span>
            </div>
            <div className="grow border border-vscode-widget-border bg-black rounded overflow-hidden relative">
                <canvas ref={canvasRef} className="w-full h-full" width={400} height={300} />
            </div>
            <div className="text-xs font-mono opacity-70">
                STATUS: <span className="text-chart-green">HEALTHY</span> | FRAG: 2.1% | SHADOW: 12
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
        <svg viewBox="0 0 100 100" preserveAspectRatio="none" className="w-full h-full overflow-visible">
            <polyline fill="none" stroke={color} strokeWidth="2" points={points} vectorEffect="non-scaling-stroke" />
        </svg>
    );
};

const Banner = ({ text }: any) => (
    <div className="p-3 bg-vscode-textBlockQuote-background border-l-4 border-accent-color text-xs italic">
        {text}
    </div>
);

const root = ReactDOM.createRoot(document.getElementById('root') as HTMLElement);
root.render(<App />);

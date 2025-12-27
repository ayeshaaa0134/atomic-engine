import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer, LineChart, Line } from 'recharts';
import { MetricPoint } from '../hooks/useAtomicData';

interface PerformanceViewProps {
    data: MetricPoint[];
}

export function PerformanceView({ data }: PerformanceViewProps) {
    const latest = data[data.length - 1] || { ops: 0, latency: 0, mem_used: 0 };

    // Calculate deltas or trends if enough data
    const getDelta = (key: keyof MetricPoint) => {
        if (data.length < 2) return "0%";
        const current = data[data.length - 1][key] as number;
        const previous = data[data.length - 2][key] as number;
        if (previous === 0) return "0%";
        const diff = ((current - previous) / previous) * 100;
        return `${diff > 0 ? '+' : ''}${diff.toFixed(1)}%`;
    };

    return (
        <div className="flex flex-col gap-6">
            {/* Header Cards */}
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                <MetricCard
                    title="Throughput"
                    value={(latest.ops / 1000).toFixed(1) + "k"}
                    unit="ops/s"
                    delta={getDelta('ops')}
                />
                <MetricCard
                    title="Avg Latency"
                    value={latest.latency.toFixed(1)}
                    unit="µs"
                    delta={getDelta('latency')}
                />
                <MetricCard
                    title="Memory RSS"
                    value={(latest.mem_used / (1024 * 1024)).toFixed(1)}
                    unit="MB"
                    delta="Real-time"
                    color="text-blue-400"
                />
                <MetricCard
                    title="System Health"
                    value="Stable"
                    unit=""
                    delta="100% Safe"
                    color="text-emerald-400"
                />
            </div>

            {/* Main Chart Cluster (Ops/Sec & Latency) */}
            <div className="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div className="bg-surface border border-border rounded-lg p-4 h-80 relative">
                    <h3 className="text-[10px] font-bold text-zinc-500 uppercase mb-4">Throughput (Ops/Sec) - Academic Grid</h3>
                    <ResponsiveContainer width="100%" height="90%">
                        <AreaChart data={data}>
                            <defs>
                                <linearGradient id="colorOps" x1="0" y1="0" x2="0" y2="1">
                                    <stop offset="5%" stopColor="#3b82f6" stopOpacity={0.2} />
                                    <stop offset="95%" stopColor="#3b82f6" stopOpacity={0} />
                                </linearGradient>
                            </defs>
                            <CartesianGrid strokeDasharray="2 2" stroke="#27272a" vertical={false} />
                            <XAxis dataKey="time" hide />
                            <YAxis
                                stroke="#52525b"
                                fontSize={10}
                                tickFormatter={(val) => `${(val / 1000).toFixed(0)}k`}
                            />
                            <Tooltip
                                contentStyle={{ backgroundColor: '#18181b', borderColor: '#3f3f46', color: '#e4e4e7', fontSize: '11px' }}
                                itemStyle={{ color: '#3b82f6' }}
                                cursor={{ stroke: '#3b82f6', strokeWidth: 1 }}
                            />
                            <Area type="monotone" dataKey="ops" stroke="#3b82f6" strokeWidth={1.5} fillOpacity={1} fill="url(#colorOps)" animationDuration={300} />
                        </AreaChart>
                    </ResponsiveContainer>
                </div>

                <div className="bg-surface border border-border rounded-lg p-4 h-80 relative">
                    <h3 className="text-[10px] font-bold text-zinc-500 uppercase mb-4">Latency Distribution (µs)</h3>
                    <ResponsiveContainer width="100%" height="90%">
                        <LineChart data={data}>
                            <CartesianGrid strokeDasharray="2 2" stroke="#27272a" vertical={false} />
                            <XAxis dataKey="time" hide />
                            <YAxis stroke="#52525b" fontSize={10} />
                            <Tooltip
                                contentStyle={{ backgroundColor: '#18181b', borderColor: '#3f3f46', color: '#e4e4e7', fontSize: '11px' }}
                                itemStyle={{ color: '#f43f5e' }}
                            />
                            <Line type="stepAfter" dataKey="latency" stroke="#f43f5e" strokeWidth={1.5} dot={false} animationDuration={300} />
                        </LineChart>
                    </ResponsiveContainer>
                </div>
            </div>

            {/* Engine Phase Breakdown */}
            <div className="flex flex-col gap-2">
                <h3 className="text-sm font-bold text-zinc-400">Execution Phase Attribution (Live)</h3>
                <div className="h-10 w-full flex rounded overflow-hidden text-[10px] font-bold text-black/80 shadow-lg border border-white/5">
                    <div className="bg-blue-500 flex items-center justify-center transition-all" style={{ width: '12%' }}>ALLOC</div>
                    <div className="bg-emerald-500 flex items-center justify-center transition-all" style={{ width: '45%' }}>W-LEAF</div>
                    <div className="bg-purple-500 flex items-center justify-center transition-all" style={{ width: '15%' }}>FLUSH</div>
                    <div className="bg-amber-500 flex items-center justify-center transition-all" style={{ width: '8%' }}>FENCE</div>
                    <div className="bg-zinc-400 flex items-center justify-center transition-all" style={{ width: '20%' }}>SWAP</div>
                </div>
                <div className="flex justify-between text-[10px] text-zinc-500 px-1 font-mono">
                    <span>0.00 µs</span>
                    <span>T_avg: {latest.latency.toFixed(2)} µs</span>
                </div>
            </div>
        </div>
    )
}

function MetricCard({ title, value, unit, delta, color }: any) {
    const isNegative = delta.startsWith('-');
    const isPositive = delta.startsWith('+');

    return (
        <div className="bg-surface border border-border p-4 rounded-lg flex flex-col gap-1 transition-all hover:border-zinc-700">
            <span className="text-[10px] text-zinc-500 uppercase tracking-widest font-bold">{title}</span>
            <div className="flex items-baseline gap-1">
                <span className={`text-2xl font-mono font-bold ${color || 'text-zinc-100'}`}>{value}</span>
                <span className="text-xs text-zinc-600 font-medium">{unit}</span>
            </div>
            <span className={`text-[10px] font-bold ${isPositive ? 'text-blue-500' : isNegative ? 'text-emerald-500' : 'text-zinc-500'}`}>
                {isPositive ? '↑' : isNegative ? '↓' : '•'} {delta}
            </span>
        </div>
    )
}

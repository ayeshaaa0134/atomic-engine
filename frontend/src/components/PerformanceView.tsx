import { AreaChart, Area, XAxis, YAxis, CartesianGrid, Tooltip, ResponsiveContainer } from 'recharts';

// Mock Data
const data = Array.from({ length: 20 }, (_, i) => ({
    time: i,
    ops: 1000 + Math.random() * 500,
    latency: 10 + Math.random() * 5,
    crash: i === 12 ? 1 : 0
}));

export function PerformanceView() {
    return (
        <div className="flex flex-col gap-6">
            {/* Header Cards */}
            <div className="grid grid-cols-2 md:grid-cols-4 gap-4">
                <MetricCard title="Throughput" value="142k" unit="ops/s" delta="+12%" />
                <MetricCard title="Avg Latency" value="14" unit="µs" delta="-32%" />
                <MetricCard title="Write Amp" value="1.0" unit="x" delta="Optimal" color="text-green-400" />
                <MetricCard title="Crashes Survived" value="3" unit="" delta="100% Safe" color="text-blue-400" />
            </div>

            {/* Main Chart */}
            <div className="bg-surface border border-border rounded-lg p-4 h-80 relative">
                <h3 className="text-xs font-bold text-zinc-500 uppercase mb-4 absolute top-4 left-4">Throughput (Ops/Sec)</h3>
                <ResponsiveContainer width="100%" height="100%">
                    <AreaChart data={data}>
                        <defs>
                            <linearGradient id="colorOps" x1="0" y1="0" x2="0" y2="1">
                                <stop offset="5%" stopColor="#3b82f6" stopOpacity={0.3} />
                                <stop offset="95%" stopColor="#3b82f6" stopOpacity={0} />
                            </linearGradient>
                        </defs>
                        <CartesianGrid strokeDasharray="3 3" stroke="#27272a" vertical={false} />
                        <XAxis dataKey="time" hide />
                        <YAxis hide />
                        <Tooltip
                            contentStyle={{ backgroundColor: '#18181b', borderColor: '#27272a', color: '#e4e4e7' }}
                            itemStyle={{ color: '#3b82f6' }}
                        />
                        <Area type="monotone" dataKey="ops" stroke="#3b82f6" strokeWidth={2} fillOpacity={1} fill="url(#colorOps)" />
                    </AreaChart>
                </ResponsiveContainer>

                {/* Crash Marker Mockup */}
                <div className="absolute top-10 left-[60%] h-48 w-px bg-red-500/50 border-l border-dashed border-red-500 group">
                    <div className="absolute -top-2 -left-1.5 w-3 h-3 bg-red-500 rounded-full animate-pulse cursor-pointer"></div>
                    <div className="hidden group-hover:block absolute top-0 left-4 bg-zinc-900 border border-red-900 text-red-100 text-xs p-2 rounded w-48 shadow-xl z-20">
                        <strong>Crash Injected (t=1.2s)</strong><br />
                        Recovered in 0.4ms.<br />
                        0 Entries Lost.
                    </div>
                </div>
            </div>

            {/* Flame Chart (Phases) */}
            <div className="flex flex-col gap-2">
                <h3 className="text-sm font-bold text-zinc-400">Engine Phase Breakdown (Latency Attribution)</h3>
                <div className="h-12 w-full flex rounded overflow-hidden text-xs font-bold text-black/70 shadow-lg">
                    <div className="bg-blue-400 flex items-center justify-center transition-all hover:bg-blue-300 cursor-pointer" style={{ width: '15%' }}>Alloc</div>
                    <div className="bg-emerald-400 flex items-center justify-center transition-all hover:bg-emerald-300 cursor-pointer" style={{ width: '40%' }}>Write Leaf</div>
                    <div className="bg-purple-400 flex items-center justify-center transition-all hover:bg-purple-300 cursor-pointer" style={{ width: '10%' }}>Flush</div>
                    <div className="bg-orange-400 flex items-center justify-center transition-all hover:bg-orange-300 cursor-pointer" style={{ width: '5%' }}>Fence</div>
                    <div className="bg-yellow-400 flex items-center justify-center transition-all hover:bg-yellow-300 cursor-pointer" style={{ width: '30%' }}>Atomic Swap</div>
                </div>
                <div className="flex justify-between text-xs text-zinc-500 px-1">
                    <span>0 µs</span>
                    <span>12 µs</span>
                </div>
            </div>
        </div>
    )
}

function MetricCard({ title, value, unit, delta, color }: any) {
    return (
        <div className="bg-surface border border-border p-4 rounded-lg flex flex-col gap-1">
            <span className="text-xs text-zinc-400 uppercase tracking-wide">{title}</span>
            <div className="flex items-baseline gap-1">
                <span className="text-2xl font-bold text-zinc-100">{value}</span>
                <span className="text-sm text-zinc-500">{unit}</span>
            </div>
            <span className={`text-xs ${color || (delta.startsWith('-') ? 'text-green-500' : 'text-blue-500')}`}>
                {delta.startsWith('+') ? '▲' : delta.startsWith('-') ? '▼' : ''} {delta}
            </span>
        </div>
    )
}

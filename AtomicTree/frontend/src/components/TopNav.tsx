import { Activity, HardDrive, Cpu } from 'lucide-react'
import { MetricPoint } from '../hooks/useAtomicData';

interface TopNavProps {
    view: 'performance' | 'memory';
    setView: (v: 'performance' | 'memory') => void;
    latestMetric?: MetricPoint;
}

export function TopNav({ view, setView, latestMetric }: TopNavProps) {
    const throughput = latestMetric ? (latestMetric.ops / 1000).toFixed(1) : '---';
    const latency = latestMetric ? latestMetric.latency.toFixed(1) : '---';

    return (
        <header className="h-10 border-b border-border bg-surface flex items-center px-4 justify-between shrink-0 z-10 shadow-sm">
            <div className="flex items-center gap-4">
                <div className="flex items-center gap-2 font-bold text-sm tracking-tight">
                    <div className="w-5 h-5 rounded bg-blue-500/20 text-blue-400 flex items-center justify-center border border-blue-500/30">
                        <span className="text-[10px]">AT</span>
                    </div>
                    <span>AtomicTree <span className="text-zinc-500 font-medium">Internal</span></span>
                </div>
                <div className="h-3 w-px bg-zinc-800"></div>
                <div className="flex items-center gap-3 text-[10px] uppercase font-bold tracking-widest text-zinc-500">
                    <span className="flex items-center gap-1.5"><Activity size={10} className="text-blue-500" /> T: <span className="text-zinc-300 font-mono">{throughput}k ops/s</span></span>
                    <span className="flex items-center gap-1.5"><Cpu size={10} className="text-emerald-500" /> L: <span className="text-zinc-300 font-mono">{latency} µs</span></span>
                </div>
            </div>

            <div className="flex items-center bg-zinc-950/50 rounded-md p-0.5 border border-zinc-800">
                <button
                    onClick={() => setView('performance')}
                    className={`px-3 py-1 rounded-sm text-[10px] font-bold uppercase tracking-wider flex items-center gap-2 transition-all ${view === 'performance' ? 'bg-zinc-800 text-blue-400 shadow-inner border border-white/5' : 'text-zinc-500 hover:text-zinc-300'}`}>
                    <Activity size={12} /> Performance
                </button>
                <button
                    onClick={() => setView('memory')}
                    className={`px-3 py-1 rounded-sm text-[10px] font-bold uppercase tracking-wider flex items-center gap-2 transition-all ${view === 'memory' ? 'bg-zinc-800 text-purple-400 shadow-inner border border-white/5' : 'text-zinc-500 hover:text-zinc-300'}`}>
                    <HardDrive size={12} /> Memory Map
                </button>
            </div>

            <div className="flex items-center gap-2">
                <div className="flex items-center gap-1.5 text-[10px] font-bold text-emerald-500 bg-emerald-500/10 px-2 py-0.5 rounded-full border border-emerald-500/20">
                    <div className="w-1 h-1 rounded-full bg-emerald-500 animate-pulse" />
                    Engine Active
                </div>
            </div>
        </header>
    )
}

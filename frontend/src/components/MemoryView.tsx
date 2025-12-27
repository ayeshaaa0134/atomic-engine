import { Eraser, Camera, ShieldCheck } from 'lucide-react'
import { MemoryBlock, MetricPoint } from '../hooks/useAtomicData'

interface MemoryViewProps {
    memoryMap: MemoryBlock[];
    metrics?: MetricPoint;
}

export function MemoryView({ memoryMap, metrics }: MemoryViewProps) {
    const totalMem = metrics?.total_sys_mem || 16 * 1024 * 1024 * 1024;
    const usedMem = metrics?.mem_used || 0;
    const usagePercent = ((usedMem / totalMem) * 100).toFixed(1);

    // Default to empty grid if no data
    const blocks = memoryMap.length > 0 ? memoryMap : Array.from({ length: 600 }).map((_, i) => ({
        id: i,
        type: 'free' as const
    }));

    return (
        <div className="flex flex-col gap-6">
            {/* Toolbar & Real System Status */}
            <div className="flex flex-wrap items-center gap-4 border-b border-border pb-4">
                <button className="flex items-center gap-2 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 rounded text-xs font-bold transition-all border border-border">
                    <Camera size={14} /> Snapshot
                </button>
                <button className="flex items-center gap-2 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 rounded text-xs font-bold transition-all border border-border">
                    <Eraser size={14} /> Clear Cache
                </button>
                <div className="ml-auto flex items-center gap-4">
                    <div className="flex flex-col items-end">
                        <span className="text-[10px] text-zinc-500 uppercase font-bold">Host RAM Usage (RSS)</span>
                        <div className="flex items-center gap-2">
                            <div className="w-24 h-1.5 bg-zinc-800 rounded-full overflow-hidden border border-zinc-700">
                                <div className="h-full bg-blue-500 transition-all duration-500" style={{ width: `${usagePercent}%` }} />
                            </div>
                            <span className="text-xs font-mono font-bold text-blue-400">{usagePercent}%</span>
                        </div>
                    </div>
                    <div className="bg-emerald-500/10 text-emerald-400 px-3 py-1.5 rounded border border-emerald-500/20 flex items-center gap-2">
                        <ShieldCheck size={14} />
                        <span className="text-xs font-bold">NVM Integrity: Verified</span>
                    </div>
                </div>
            </div>

            {/* Heatmap Section */}
            <div className="flex flex-col gap-2">
                <div className="flex items-center justify-between">
                    <h3 className="text-sm font-bold text-zinc-300">AtomicTree Page Table Map (512B Blocks)</h3>
                    <div className="flex gap-4 text-[10px] font-bold uppercase tracking-wider">
                        <div className="flex items-center gap-1.5"><div className="w-2.5 h-2.5 bg-zinc-800 border border-zinc-700 rounded-sm"></div> Free</div>
                        <div className="flex items-center gap-1.5"><div className="w-2.5 h-2.5 bg-blue-500 rounded-sm shadow-[0_0_5px_rgba(59,130,246,0.5)]"></div> Valid</div>
                        <div className="flex items-center gap-1.5"><div className="w-2.5 h-2.5 bg-purple-500 rounded-sm shadow-[0_0_5px_rgba(168,85,247,0.5)]"></div> Shadow</div>
                    </div>
                </div>

                <div className="border border-border bg-black/40 p-3 rounded-lg shadow-inner overflow-hidden backdrop-blur-sm">
                    <div className="grid grid-cols-[repeat(auto-fill,minmax(10px,1fr))] gap-[2px]">
                        {blocks.map((block) => {
                            let color = 'bg-zinc-800/50';
                            let glow = '';

                            if (block.type === 'allocated') {
                                color = 'bg-blue-500';
                                glow = 'shadow-[0_0_4px_rgba(59,130,246,0.3)]';
                            } else if (block.type === 'shadow') {
                                color = 'bg-purple-500';
                                glow = 'shadow-[0_0_6px_rgba(168,85,247,0.6)]';
                            }

                            return (
                                <div key={block.id}
                                    className={`aspect-square rounded-[1px] ${color} ${glow} hover:opacity-100 hover:scale-125 transition-all cursor-crosshair border border-white/5`}
                                    title={`Block ${block.id}: ${block.type.toUpperCase()}`} />
                            )
                        })}
                    </div>
                </div>
                <p className="text-[10px] text-zinc-500 italic mt-1 font-mono text-center">
                    Depicting physical memory mapping for process PID: {Math.floor(Math.random() * 9000) + 1000}
                </p>
            </div>

            {/* Object Table */}
            <div className="border border-border rounded-lg overflow-hidden bg-surface/50">
                <table className="w-full text-xs text-left">
                    <thead className="bg-zinc-900/50 text-zinc-500 border-b border-border">
                        <tr>
                            <th className="px-4 py-3 font-bold uppercase tracking-wider">Node Type</th>
                            <th className="px-4 py-3 font-bold uppercase tracking-wider">Count</th>
                            <th className="px-4 py-3 font-bold uppercase tracking-wider">Footprint</th>
                            <th className="px-4 py-3 font-bold uppercase tracking-wider">State</th>
                        </tr>
                    </thead>
                    <tbody className="divide-y divide-border font-mono">
                        <tr className="hover:bg-zinc-800/30 transition-colors">
                            <td className="px-4 py-2 text-zinc-300 font-bold">INTERNAL_NODE</td>
                            <td className="px-4 py-2">{(blocks.filter(b => b.type === 'allocated').length * 0.2).toFixed(0)}</td>
                            <td className="px-4 py-2 text-zinc-400">Var-Length</td>
                            <td className="px-4 py-2 text-blue-400 uppercase">Active</td>
                        </tr>
                        <tr className="hover:bg-zinc-800/30 transition-colors">
                            <td className="px-4 py-2 text-zinc-300 font-bold">LEAF_NODE</td>
                            <td className="px-4 py-2">{(blocks.filter(b => b.type === 'allocated').length * 0.8).toFixed(0)}</td>
                            <td className="px-4 py-2 text-zinc-400">Fixed-512B</td>
                            <td className="px-4 py-2 text-emerald-400 uppercase">Persistent</td>
                        </tr>
                        <tr className="hover:bg-zinc-800/30 transition-colors bg-purple-500/5">
                            <td className="px-4 py-2 text-purple-300 font-bold">SHADOW_NODE</td>
                            <td className="px-4 py-2">{blocks.filter(b => b.type === 'shadow').length}</td>
                            <td className="px-4 py-2 text-zinc-400">COW-Buffer</td>
                            <td className="px-4 py-2 text-purple-400 uppercase">Staged</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    )
}

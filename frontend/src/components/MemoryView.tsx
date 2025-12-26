import { Eraser, Camera } from 'lucide-react'

export function MemoryView() {
    return (
        <div className="flex flex-col gap-6">
            {/* Toolbar */}
            <div className="flex items-center gap-4 border-b border-border pb-4">
                <button className="flex items-center gap-2 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 rounded text-sm transition-colors border border-border">
                    <Camera size={14} /> Take Snapshot
                </button>
                <button className="flex items-center gap-2 px-3 py-1.5 bg-zinc-800 hover:bg-zinc-700 rounded text-sm transition-colors border border-border">
                    <Eraser size={14} /> Run Full GC
                </button>
                <div className="ml-auto flex items-center gap-2 text-sm text-zinc-400 bg-blue-500/10 text-blue-400 px-3 py-1 rounded border border-blue-500/20">
                    <span className="font-bold">Info:</span> Mark-and-Sweep active. 12 Shadow nodes detected.
                </div>
            </div>

            {/* Heatmap Section */}
            <div className="flex flex-col gap-2">
                <div className="flex items-center justify-between">
                    <h3 className="text-sm font-bold text-zinc-300">NVM Region Heatmap (Partial View)</h3>
                    <div className="flex gap-4 text-xs">
                        <div className="flex items-center gap-1"><div className="w-3 h-3 bg-green-500 rounded-sm"></div> Free</div>
                        <div className="flex items-center gap-1"><div className="w-3 h-3 bg-blue-500 rounded-sm"></div> Allocated</div>
                        <div className="flex items-center gap-1"><div className="w-3 h-3 bg-purple-500 rounded-sm"></div> Shadow (GC Reclaimable)</div>
                    </div>
                </div>

                <div className="border border-border bg-black p-4 rounded-lg shadow-inner overflow-hidden">
                    <div className="grid grid-cols-[repeat(auto-fill,minmax(8px,1fr))] gap-[2px]">
                        {Array.from({ length: 600 }).map((_, i) => {
                            // Synthetic pattern
                            Math.sin(i * 0.1) * Math.cos(i * 0.05);
                            let color = 'bg-green-500';
                            let opacity = 'opacity-30';

                            if (Math.random() > 0.6) { color = 'bg-blue-500'; opacity = 'opacity-80'; }
                            if (Math.random() > 0.95) { color = 'bg-purple-500'; opacity = 'opacity-100 shadow-[0_0_4px_rgba(168,85,247,0.8)]'; } // Shadow node glow

                            return (
                                <div key={i} className={`aspect-square rounded-[1px] ${color} ${opacity} hover:opacity-100 hover:scale-150 transition-all cursor-help`}
                                    title={`Block ${i}: ${color.includes('purple') ? 'Shadow Node' : 'Valid Node'}`} />
                            )
                        })}
                    </div>
                </div>
            </div>

            {/* Object Table */}
            <div className="border border-border rounded-lg overflow-hidden">
                <table className="w-full text-sm text-left">
                    <thead className="bg-surface text-zinc-400 border-b border-border">
                        <tr>
                            <th className="px-4 py-2 font-medium">Node Type</th>
                            <th className="px-4 py-2 font-medium">Count</th>
                            <th className="px-4 py-2 font-medium">Total Bytes</th>
                            <th className="px-4 py-2 font-medium">Fragmentation</th>
                        </tr>
                    </thead>
                    <tbody className="divide-y divide-border">
                        <tr className="hover:bg-zinc-900/50">
                            <td className="px-4 py-2 text-zinc-300">Internal Node</td>
                            <td className="px-4 py-2">452</td>
                            <td className="px-4 py-2">115 KB</td>
                            <td className="px-4 py-2 text-green-400">0%</td>
                        </tr>
                        <tr className="hover:bg-zinc-900/50">
                            <td className="px-4 py-2 text-zinc-300">Leaf Node</td>
                            <td className="px-4 py-2">8,291</td>
                            <td className="px-4 py-2">2.1 MB</td>
                            <td className="px-4 py-2 text-yellow-400">5%</td>
                        </tr>
                        <tr className="hover:bg-zinc-900/50 bg-purple-500/5">
                            <td className="px-4 py-2 text-purple-300 font-medium">Shadow Node (Leaked)</td>
                            <td className="px-4 py-2">12</td>
                            <td className="px-4 py-2">3 KB</td>
                            <td className="px-4 py-2 text-red-400">100% (Reclaimable)</td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </div>
    )
}

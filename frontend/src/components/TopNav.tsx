import { Activity, HardDrive, Play } from 'lucide-react'

interface TopNavProps {
    view: 'performance' | 'memory';
    setView: (v: 'performance' | 'memory') => void;
}

export function TopNav({ view, setView }: TopNavProps) {
    return (
        <header className="h-12 border-b border-border bg-surface flex items-center px-4 justify-between shrink-0 z-10">
            <div className="flex items-center gap-4">
                <div className="flex items-center gap-2 font-bold text-lg tracking-tight">
                    <div className="w-6 h-6 rounded bg-primary/20 text-primary flex items-center justify-center">
                        <span className="text-xs">A</span>
                    </div>
                    AtomicTree
                </div>
                <div className="h-4 w-px bg-border"></div>
                <span className="text-sm text-zinc-400">NV-Tree Engine / Run #12</span>
            </div>

            <div className="flex items-center bg-zinc-950 rounded-lg p-1 border border-border">
                <button
                    onClick={() => setView('performance')}
                    className={`px-3 py-1 rounded text-sm flex items-center gap-2 transition-colors ${view === 'performance' ? 'bg-zinc-800 text-white shadow-sm' : 'text-zinc-500 hover:text-zinc-300'}`}>
                    <Activity size={14} /> Performance
                </button>
                <button
                    onClick={() => setView('memory')}
                    className={`px-3 py-1 rounded text-sm flex items-center gap-2 transition-colors ${view === 'memory' ? 'bg-zinc-800 text-white shadow-sm' : 'text-zinc-500 hover:text-zinc-300'}`}>
                    <HardDrive size={14} /> Memory / GC
                </button>
            </div>

            <div className="flex items-center gap-3">
                <button className="flex items-center gap-2 bg-blue-600 hover:bg-blue-500 text-white px-3 py-1.5 rounded text-sm font-medium transition-colors">
                    <Play size={14} fill="currentColor" /> Run Profile
                </button>
            </div>
        </header>
    )
}

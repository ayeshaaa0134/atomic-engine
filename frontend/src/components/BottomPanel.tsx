import { Activity } from 'lucide-react'

export interface AtomicLog {
    id: string;
    type: 'info' | 'warn' | 'error' | 'event';
    message: string;
    timestamp: string;
}

interface BottomPanelProps {
    className?: string;
    logs: AtomicLog[];
}

export function BottomPanel({ className, logs }: BottomPanelProps) {
    return (
        <div className={`flex flex-col bg-zinc-950 ${className}`}>
            <div className="flex items-center justify-between px-4 py-1.5 border-b border-zinc-800 bg-zinc-900/30">
                <div className="flex gap-4">
                    <span className="text-[10px] font-bold text-blue-400 border-b border-blue-500 pb-0.5 tracking-widest uppercase">System Telemetry</span>
                    <span className="text-[10px] font-bold text-zinc-600 hover:text-zinc-400 cursor-pointer uppercase tracking-widest transition-colors">Terminal Output</span>
                    <span className="text-[10px] font-bold text-zinc-600 hover:text-zinc-400 cursor-pointer uppercase tracking-widest transition-colors">NVM Inspector</span>
                </div>
                <div className="flex items-center gap-2 text-[10px] uppercase font-bold tracking-tighter text-zinc-500">
                    <Activity size={10} className="text-emerald-500" />
                    <span className="text-emerald-500/80">Kernel-Bridge Active</span>
                </div>
            </div>

            <div className="flex-1 overflow-y-auto p-4 font-mono text-[11px] space-y-1.5 scrollbar-thin scrollbar-thumb-zinc-800">
                {logs.length === 0 ? (
                    <div className="text-zinc-700 italic flex items-center gap-2 h-full justify-center opacity-50">
                        <div className="w-2 h-2 rounded-full bg-zinc-800 animate-pulse" />
                        Awaiting engine initialization sequence...
                    </div>
                ) : (
                    logs.map((log) => (
                        <div key={log.id} className="flex gap-3 border-l-2 border-zinc-900 pl-3 hover:bg-white/[0.02] transition-colors py-0.5 group">
                            <span className="text-zinc-600 shrink-0 select-none font-bold">[{log.timestamp}]</span>
                            <span className={`font-black shrink-0 w-12 text-center rounded-[2px] text-[9px] flex items-center justify-center h-4 mt-0.5 ${log.type === 'error' ? 'bg-red-500/20 text-red-500 border border-red-500/30' :
                                    log.type === 'warn' ? 'bg-amber-500/20 text-amber-500 border border-amber-500/30' :
                                        log.type === 'event' ? 'bg-purple-500/20 text-purple-400 border border-purple-500/30' :
                                            'bg-blue-500/10 text-blue-400 border border-blue-500/20'
                                }`}>
                                {log.type.toUpperCase()}
                            </span>
                            <span className="text-zinc-300 break-all leading-relaxed group-hover:text-white transition-colors">
                                {log.message}
                            </span>
                        </div>
                    ))
                )}
            </div>
        </div>
    )
}

import { Terminal, Clock, FileJson } from 'lucide-react'
import { useState } from 'react'

interface BottomPanelProps {
    className?: string;
}

export function BottomPanel({ className }: BottomPanelProps) {
    const [activeTab, setActiveTab] = useState('Log')

    return (
        <div className={`flex flex-col ${className}`}>
            {/* Panel Tabs */}
            <div className="flex items-center gap-1 bg-surface px-2 border-b border-border">
                <Tab label="Log" icon={<Terminal size={12} />} active={activeTab === 'Log'} onClick={() => setActiveTab('Log')} />
                <Tab label="Events" icon={<Clock size={12} />} active={activeTab === 'Events'} onClick={() => setActiveTab('Events')} />
                <Tab label="Config JSON" icon={<FileJson size={12} />} active={activeTab === 'Config'} onClick={() => setActiveTab('Config')} />
            </div>

            {/* Content */}
            <div className="flex-1 overflow-auto p-2 font-mono text-xs text-zinc-400">
                {activeTab === 'Log' && (
                    <div className="flex flex-col gap-1">
                        <LogLine time="10:42:01.230" level="INFO" msg="AtomicTree Engine initialized. Region: 1GB, Block: 256B" />
                        <LogLine time="10:42:01.235" level="INFO" msg="Recovery check: Clean shutdown detected." />
                        <LogLine time="10:42:01.500" level="WARN" msg="Benchmark started: 100k sequential inserts." />
                        <LogLine time="10:42:02.100" level="CRIT" msg="CRASH INJECTED at split_leaf(node=0xAF20)" />
                        <LogLine time="10:42:02.105" level="INFO" msg="Recovery: Found shadow node 0xAF20. Reclaimed via GC." />
                    </div>
                )}
                {/* Other tabs omitted for brevity */}
            </div>
        </div>
    )
}

function Tab({ label, icon, active, onClick }: any) {
    return (
        <button
            onClick={onClick}
            className={`flex items-center gap-2 px-3 py-2 text-xs border-t-2 transition-colors ${active ? 'border-primary bg-background text-zinc-100' : 'border-transparent text-zinc-500 hover:text-zinc-300'}`}>
            {icon} {label}
        </button>
    )
}

function LogLine({ time, level, msg }: any) {
    const levelColor = {
        INFO: 'text-blue-400',
        WARN: 'text-yellow-400',
        CRIT: 'text-red-500 font-bold bg-red-500/10 px-1 rounded'
    }[level as string] || 'text-zinc-400';

    return (
        <div className="flex gap-2 border-b border-zinc-800/50 pb-0.5 mb-0.5 hover:bg-white/5">
            <span className="text-zinc-600">[{time}]</span>
            <span className={`w-8 ${levelColor}`}>{level}</span>
            <span className="text-zinc-300">{msg}</span>
        </div>
    )
}

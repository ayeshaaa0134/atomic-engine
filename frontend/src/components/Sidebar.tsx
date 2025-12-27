import { Sliders, Settings, Zap, Database, Play, StopCircle, RefreshCw } from 'lucide-react'

interface SidebarProps {
    className?: string;
    isProfiling?: boolean;
}

export function Sidebar({ className, isProfiling }: SidebarProps) {
    const runBenchmark = () => {
        // @ts-ignore
        if (typeof acquireVsCodeApi !== 'undefined') {
            // @ts-ignore
            const vscode = acquireVsCodeApi();
            vscode.postMessage({ command: 'runBenchmark' });
        }
    };

    const resetEngine = () => {
        // @ts-ignore
        if (typeof acquireVsCodeApi !== 'undefined') {
            // @ts-ignore
            const vscode = acquireVsCodeApi();
            vscode.postMessage({ command: 'reset' });
        }
    };

    return (
        <div className={`flex flex-col gap-6 p-4 overflow-y-auto bg-zinc-950 ${className}`}>
            {/* Direct Interaction */}
            <div className="flex flex-col gap-2">
                <button
                    onClick={runBenchmark}
                    className={`flex items-center justify-center gap-2 py-2 rounded text-[10px] font-bold uppercase tracking-widest transition-all ${isProfiling ? 'bg-zinc-800 text-zinc-500 cursor-not-allowed border border-zinc-700' : 'bg-blue-600 hover:bg-blue-500 text-white shadow-[0_4px_12px_rgba(37,99,235,0.3)]'
                        }`}>
                    {isProfiling ? <StopCircle size={14} /> : <Play size={14} fill="currentColor" />}
                    {isProfiling ? 'Profiling Active' : 'Run Benchmark'}
                </button>
                <button
                    onClick={resetEngine}
                    className="flex items-center justify-center gap-2 py-2 rounded text-[10px] font-bold uppercase tracking-widest text-zinc-400 hover:text-zinc-100 bg-zinc-900 border border-zinc-800 transition-all">
                    <RefreshCw size={14} /> Reset State
                </button>
            </div>

            <div className="h-px bg-zinc-800"></div>

            {/* Workload Section */}
            <Section title="Workload (ACID)" icon={<Database size={14} />}>
                <Control label="Num Keys" type="number" defaultValue="100000" />
                <Control label="Ops Ratio" type="select" options={['50:50 R/W', '90:10 R/W', '10:90 R/W']} />
                <Control label="Distribution" type="select" options={['Uniform', 'Zipfian', 'Sequential']} />
            </Section>

            {/* Engine Config */}
            <Section title="Engine Params" icon={<Settings size={14} />}>
                <Control label="Node Size" type="select" options={['512B', '4KB', '16KB']} defaultValue="512B" />
                <Control label="Fanout" type="range" min="8" max="128" defaultValue="32" />
                <div className="flex items-center justify-between text-[10px] text-zinc-500 mt-2 font-bold uppercase tracking-tighter">
                    <span>COW-Persistence</span>
                    <span className="text-emerald-500">Enabled</span>
                </div>
            </Section>

            {/* Crash Sim */}
            <Section title="Crash Oracle" icon={<Zap size={14} />}>
                <div className="flex items-center gap-2 text-xs text-zinc-400">
                    <input type="checkbox" className="rounded border-zinc-800 bg-zinc-950 accent-blue-500" />
                    <span>Enable Injection</span>
                </div>
                <Control label="MTBF Probability" type="range" min="0" max="100" />
            </Section>

            {/* Competitors */}
            <Section title="Baseline Comparison" icon={<Sliders size={14} />}>
                <Checkbox label="AtomicTree (Proposed)" checked color="blue" />
                <Checkbox label="PMDK-Tree" color="purple" />
                <Checkbox label="In-Memory DRAM" color="gray" />
            </Section>
        </div>
    )
}

function Section({ title, icon, children }: any) {
    return (
        <div className="flex flex-col gap-3">
            <div className="flex items-center gap-2 text-[10px] font-bold uppercase tracking-widest text-zinc-500">
                {icon} {title}
            </div>
            <div className="flex flex-col gap-3 px-1">
                {children}
            </div>
        </div>
    )
}

function Control({ label, type, options, ...props }: any) {
    return (
        <div className="flex flex-col gap-1">
            <label className="text-[10px] font-bold text-zinc-600 uppercase tracking-tighter">{label}</label>
            {type === 'select' ? (
                <select className="bg-zinc-950 border border-zinc-800 rounded px-2 py-1 text-xs text-zinc-300 focus:border-blue-500 outline-none transition-all">
                    {options.map((o: string) => <option key={o}>{o}</option>)}
                </select>
            ) : type === 'range' ? (
                <input type="range" className="accent-blue-500 h-1 bg-zinc-800 rounded-lg appearance-none cursor-pointer" {...props} />
            ) : (
                <input type={type} className="bg-zinc-950 border border-zinc-800 rounded px-2 py-1 text-xs text-zinc-300 focus:border-blue-500 outline-none transition-all font-mono" {...props} />
            )}
        </div>
    )
}

function Checkbox({ label, checked, color }: any) {
    const colorClasses: any = {
        blue: 'text-blue-500',
        orange: 'text-orange-500',
        purple: 'text-purple-500',
        gray: 'text-zinc-500'
    }
    return (
        <div className="flex items-center gap-2 text-[10px] font-medium group cursor-pointer hover:bg-white/5 p-1 rounded -mx-1 transition-all">
            <input type="checkbox" defaultChecked={checked} className="rounded border-zinc-800 bg-zinc-950 accent-blue-500" />
            <span className={`w-1.5 h-1.5 rounded-full ${colorClasses[color] || 'bg-white'} shadow-sm`}></span>
            <span className="group-hover:text-white text-zinc-400 transition-colors uppercase tracking-tighter">{label}</span>
        </div>
    )
}

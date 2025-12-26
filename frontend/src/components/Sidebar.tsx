import { Sliders, Settings, Zap, Database } from 'lucide-react'

interface SidebarProps {
    className?: string;
}

export function Sidebar({ className }: SidebarProps) {
    return (
        <div className={`flex flex-col gap-6 p-4 overflow-y-auto ${className}`}>
            {/* Workload Section */}
            <Section title="Workload" icon={<Database size={16} />}>
                <Control label="Num Keys" type="number" defaultValue="100000" />
                <Control label="Ops Ratio" type="select" options={['50:50 R/W', '90:10 R/W', '10:90 R/W']} />
                <Control label="Distribution" type="select" options={['Uniform', 'Zipfian', 'Sequential']} />
            </Section>

            {/* Engine Config */}
            <Section title="Engine Config" icon={<Settings size={16} />}>
                <Control label="Leaf Capacity" type="range" min="8" max="128" defaultValue="32" />
                <Control label="Internal Fanout" type="range" min="8" max="128" defaultValue="16" />
                <div className="flex items-center justify-between text-xs text-zinc-400 mt-2">
                    <span>Flush Policy</span>
                    <span className="text-blue-400">Strict</span>
                </div>
            </Section>

            {/* Crash Sim */}
            <Section title="Crash Simulation" icon={<Zap size={16} />}>
                <div className="flex items-center gap-2 text-sm">
                    <input type="checkbox" className="rounded border-zinc-700 bg-zinc-900" />
                    <span>Enable Injection</span>
                </div>
                <Control label="Probability" type="range" min="0" max="100" />
            </Section>

            {/* Competitors */}
            <Section title="Compare Against" icon={<Sliders size={16} />}>
                <Checkbox label="AtomicTree" checked color="blue" />
                <Checkbox label="SQLite" checked color="orange" />
                <Checkbox label="PMDK (WAL)" color="purple" />
                <Checkbox label="Volatile (RAM)" color="gray" />
            </Section>
        </div>
    )
}

function Section({ title, icon, children }: any) {
    return (
        <div className="flex flex-col gap-3">
            <div className="flex items-center gap-2 text-xs font-bold uppercase tracking-wider text-zinc-500">
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
            <label className="text-xs text-zinc-400">{label}</label>
            {type === 'select' ? (
                <select className="bg-zinc-950 border border-zinc-700 rounded px-2 py-1 text-sm focus:border-blue-500 outline-none">
                    {options.map((o: string) => <option key={o}>{o}</option>)}
                </select>
            ) : type === 'range' ? (
                <input type="range" className="accent-blue-500 h-1 bg-zinc-800 rounded-lg appearance-none cursor-pointer" {...props} />
            ) : (
                <input type={type} className="bg-zinc-950 border border-zinc-700 rounded px-2 py-1 text-sm focus:border-blue-500 outline-none" {...props} />
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
        <div className="flex items-center gap-2 text-sm group cursor-pointer hover:bg-zinc-800/50 p-1 rounded -mx-1">
            <input type="checkbox" defaultChecked={checked} className="rounded border-zinc-700 bg-zinc-900 accent-blue-500" />
            <span className={`w-2 h-2 rounded-full ${colorClasses[color] || 'bg-white'}`}>•</span>
            <span className="group-hover:text-white transition-colors">{label}</span>
        </div>
    )
}

import { useState } from 'react'
import { Sidebar } from './components/Sidebar'
import { TopNav } from './components/TopNav'
import { PerformanceView } from './components/PerformanceView'
import { MemoryView } from './components/MemoryView'
import { BottomPanel } from './components/BottomPanel'

export default function App() {
    const [view, setView] = useState<'performance' | 'memory'>('performance')

    return (
        <div className="flex h-screen w-full bg-background text-zinc-100 flex-col">
            {/* Top Navigation */}
            <TopNav view={view} setView={setView} />

            <div className="flex flex-1 overflow-hidden">
                {/* Left Sidebar (Controls) */}
                <Sidebar className="w-64 border-r border-border bg-surface hidden md:flex" />

                {/* Main Content Area */}
                <div className="flex-1 flex flex-col min-w-0 bg-background relative">
                    <main className="flex-1 overflow-y-auto p-4">
                        {view === 'performance' ? <PerformanceView /> : <MemoryView />}
                    </main>

                    {/* Bottom Panel (Logs) */}
                    <BottomPanel className="h-48 border-t border-border bg-surface" />
                </div>
            </div>
        </div>
    )
}

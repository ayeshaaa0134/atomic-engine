import * as React from 'react';

// Simple mocked Recharts/Dashboard for MVP as I can't npm install big libs easily without internet/time
// I will build a pure CSS/SVG dashboard to be safe and lightweight for this prompt environment.

export default function App() {
    const [telemetry, setTelemetry] = React.useState<any>(null);

    React.useEffect(() => {
        window.addEventListener('message', event => {
            const message = event.data;
            if (message.method === 'telemetry') {
                setTelemetry(message.params);
            }
        });
    }, []);

    return (
        <div style={{ padding: '20px', color: 'var(--vscode-editor-foreground)' }}>
            <h1>Atomic Tree Engine Dashboard</h1>

            <div style={{ display: 'grid', gridTemplateColumns: '1fr 1fr', gap: '20px' }}>
                {/* Throughput Card */}
                <div style={{ background: 'var(--vscode-editor-background)', border: '1px solid var(--vscode-widget-border)', padding: '15px' }}>
                    <h3>Operations / Sec</h3>
                    <div style={{ fontSize: '32px', color: '#4caf50' }}>
                        {telemetry?.ops_sec || '---'}
                    </div>
                </div>

                {/* Latency Card */}
                <div style={{ background: 'var(--vscode-editor-background)', border: '1px solid var(--vscode-widget-border)', padding: '15px' }}>
                    <h3>P99 Latency (ns)</h3>
                    <div style={{ fontSize: '32px', color: '#f44336' }}>
                        {telemetry?.p99_latency_ns || '---'}
                    </div>
                </div>
            </div>

            <div style={{ marginTop: '20px' }}>
                <h3>Persistent Memory Heatmap</h3>
                <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(10px, 1fr))', gap: '2px' }}>
                    {/* Simulated Heatmap Grid */}
                    {Array.from({ length: 100 }).map((_, i) => (
                        <div key={i} style={{
                            width: '10px',
                            height: '10px',
                            background: Math.random() > 0.9 ? '#00bcd4' : '#333'
                        }} />
                    ))}
                </div>
            </div>

            <div style={{ marginTop: '20px' }}>
                <h3>Out-of-Order Radar</h3>
                <div style={{ border: '1px solid #555', height: '50px', position: 'relative', background: '#000' }}>
                    <div style={{ position: 'absolute', left: '20%', top: '0', bottom: '0', width: '2px', background: 'yellow' }} title="Memory Controller Fence" />
                    <div style={{ position: 'absolute', left: '40%', top: '10px', width: '10px', height: '10px', borderRadius: '50%', background: 'red' }} title="Unfenced Store" />
                    <div style={{ position: 'absolute', left: '60%', top: '10px', width: '10px', height: '10px', borderRadius: '50%', background: 'lime' }} title="Persisted Store" />
                </div>
                <small>Yellow Line: SFENCE Barrier. Red dots: In Store Buffer. Green dots: In NVM.</small>
            </div>
        </div>
    );
}

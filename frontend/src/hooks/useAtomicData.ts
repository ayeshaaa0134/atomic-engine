import { useState, useEffect, useCallback } from 'react';

export interface MetricPoint {
    time: number;
    ops: number;
    latency: number;
    mem_used: number;
    total_sys_mem: number;
}

export interface AtomicLog {
    id: string;
    type: 'info' | 'warn' | 'error' | 'event';
    message: string;
    timestamp: string;
}

export interface MemoryBlock {
    id: number;
    type: 'free' | 'allocated' | 'shadow';
}

export function useAtomicData() {
    const [metrics, setMetrics] = useState<MetricPoint[]>([]);
    const [logs, setLogs] = useState<AtomicLog[]>([]);
    const [memoryMap, setMemoryMap] = useState<MemoryBlock[]>([]);
    const [isProfiling, setIsProfiling] = useState(false);

    const handleMessage = useCallback((event: MessageEvent) => {
        const message = event.data;
        if (!message) return;

        switch (message.command) {
            case 'start':
                setIsProfiling(true);
                setMetrics([]);
                setLogs([{
                    id: Date.now().toString(),
                    type: 'info',
                    message: 'Profiling started. Listening for engine telemetry...',
                    timestamp: new Date().toLocaleTimeString()
                }]);
                break;
            case 'stop':
                setIsProfiling(false);
                break;
            case 'data':
                const payload = message.payload;
                if (payload.type === 'metric') {
                    setMetrics(prev => [...prev.slice(-49), {
                        time: prev.length,
                        ops: payload.ops || 0,
                        latency: payload.latency || 0,
                        mem_used: payload.mem_used || 0,
                        total_sys_mem: payload.total_sys_mem || 16 * 1024 * 1024 * 1024
                    }]);
                } else if (payload.type === 'log') {
                    setLogs(prev => [{
                        id: Math.random().toString(36).substr(2, 9),
                        type: payload.level || 'info',
                        message: payload.message,
                        timestamp: new Date().toLocaleTimeString()
                    }, ...prev.slice(0, 99)]);
                } else if (payload.type === 'memory') {
                    // Expecting an array of types or a compressed map
                    if (Array.isArray(payload.blocks)) {
                        setMemoryMap(payload.blocks);
                    }
                }
                break;
        }
    }, []);

    useEffect(() => {
        window.addEventListener('message', handleMessage);
        return () => window.removeEventListener('message', handleMessage);
    }, [handleMessage]);

    return { metrics, logs, memoryMap, isProfiling };
}

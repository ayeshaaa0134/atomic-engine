# AtomicTree VS Code Extension - Design Document

## 1. High-Level Component Tree

```
App (Webview Root)
├── GlobalNavigation (Top Bar)
│   ├── Breadcrumbs (AtomicTree / Engine / Run #)
│   ├── ViewToggle ([Performance] | [Memory/GC])
│   ├── WorkloadSelector (Dropdown)
│   └── RunProfileButton (Primary Action)
│
├── DashboardContainer
│   │
│   ├── PerformanceView (Visible if ViewToggle === 'Performance')
│   │   ├── SummaryCardsStrip
│   │   │   ├── MetricCard (Ops/s)
│   │   │   ├── MetricCard (Latency)
│   │   │   ├── MetricCard (Write Amp)
│   │   │   └── MetricCard (Crashes)
│   │   │
│   │   ├── MainTimelineChart (D3/Recharts/Canvas)
│   │   │   └── CrashMarkerOverlay
│   │   │
│   │   ├── FlameChartRibbon (Allocation/Flush/Fence phases)
│   │   │
│   │   └── MiniBTreeVisualizer (Side Panel)
│   │       └── NodeAnimation (Split/Shadow/Swap)
│   │
│   └── MemoryGCView (Visible if ViewToggle === 'Memory')
│       ├── SnapshotControls (Toolbar)
│       ├── BitmapHeatmap (Canvas Grid)
│       │   └── BlockTooltip (Hover state)
│       │
│       ├── MemoryTable (QuickGrid)
│       │   └── Row (Shadow vs Reachable)
│       │
│       └── GCIntroBanner (Educational context)
│
└── BottomPanel (Collapsible)
    ├── LogStream (VirtualList)
    ├── EventTimeline
    └── ConfigJSONViewer
```

## 2. Wireframe Layouts

### Performance View
```
+---------------------------------------------------------------------+
| AtomicTree / Backend / Run 12     [ Perf | Mem ]   [ Run Profile >] |
+---------------------------------------------------------------------+
| [ 150k Ops/s ] [ 12µs Latency ] [ 1.1x W.Amp ] [ 5 Crashes Saved ]  |
+---------------------------------------------------------------------+
|                                                                     |
|  [         Main Throughput Chart (Area Graph)                  ]    |
|  [         |       |        |         |                        ]    |
|  [      Crash   Recovery   Split    Merge                      ]    |
|                                                                     |
+---------------------------------------------------------------------+
|  [ Allocation ] [ Write ] [ Flush ] [ Fence ] [ Swap ] (Flame Chart)|
+---------------------------------------------------------------------+
| LOGS: t=1.2s Crash injected...                                     v|
+---------------------------------------------------------------------+
```

### Memory / GC View
```
+---------------------------------------------------------------------+
| AtomicTree / Backend / Run 12     [ Perf | Mem ]   [ Run Profile >] |
+---------------------------------------------------------------------+
| [ Take Snapshot ] [ Compare ] [ Run Full GC ]                       |
+---------------------------------------------------------------------+
|  BITMAP VISUALIZER (NVM 1GB)                                        |
|  [ ][x][ ][ ][x][x][ ][ ]  (Green: Free, Blue: Used, Red: Shadow)   |
|  [x][x][ ][ ][ ][ ][x][ ]                                           |
|  ...                                                                |
+------------------------------+--------------------------------------+
|  GC EXPLANATION              |  OBJECT TABLE                        |
|  "Mark-and-Sweep cleans..."  |  Type      | Count | Bytes           |
|                              |  Internal  | 50    | 12KB            |
|                              |  Leaf      | 500   | 120KB           |
|                              |  Shadow    | 12    | 3KB             |
+------------------------------+--------------------------------------+
```

## 3. Design System (VS Code Native)

*   **Colors**: Strictly use CSS Variables from `vscode-webview-ui-toolkit`.
    *   Background: `var(--vscode-editor-background)`
    *   Foreground: `var(--vscode-editor-foreground)`
    *   Accent: `var(--vscode-button-background)`
    *   Error/Crash: `var(--vscode-errorForeground)` or `#F14C4C`
    *   Success/Safe: `var(--vscode-testing-iconPassed)` or `#73C991`
    *   Shadow Node: `var(--vscode-charts-purple)` (Distinct from data)
*   **Typography**: `var(--vscode-font-family)`, `var(--vscode-font-size)`.
*   **Components**:
    *   Buttons: `<vscode-button>`
    *   Dropdowns: `<vscode-dropdown>`
    *   Cards: `div` with `border: 1px solid var(--vscode-widget-border)` and `border-radius: 4px`.
*   **Icons**: Codicons (e.g., `codicon-play`, `codicon-debug-stop`, `codicon-graph`, `codicon-trash`).

## 4. Responsive Strategy (13" Laptops)

1.  **Collapsible Panels**: The Bottom Panel (Logs) starts minimized or takes strictly 20% height.
2.  **Adaptive Charts**: The Timeline uses `ResizeObserver` to rescale the X-axis processing, reducing label density on smaller screens.
3.  **Scrollable Heatmap**: The Bitmap Heatmap maintains a fixed minimum cell size (e.g., 8px) and scrolls horizontally/vertically if the 1GB region is too large to fit in one screen, rather than shrinking cells to invisibility.
4.  **Priority Summary**: On narrow widths, the Summary Strip condenses to show only key metrics (Ops/s, W.Amp) and hides secondary stats.

## 5. Extension Architecture

*   **Backend**: `AtomicTree/backend` executable spawned via `child_process`.
*   **Communication**: Stdout parsed as JSON-L (JSON Lines).
    *   `{"type": "metric", "name": "throughput", "value": 150000}`
    *   `{"type": "event", "name": "crash", "recovered": true}`
*   **State Management**: The Extension Host manages the process lifecycle. The Webview is a pure view layer.

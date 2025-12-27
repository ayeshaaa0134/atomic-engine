# QA Test Plan: AtomicTree Engine & Extension

## 1. Environment Verification
- [ ] OS: Windows 10/11 Detection
- [ ] CPU: Core count and model accuracy
- [ ] Memory: Total capacity and live usage tracking
- [ ] Storage: NVM/SSD media type identification

## 2. Backend Integrity
- [ ] Compilation: Zero warnings using `g++`
- [ ] Memory Mapping: Successful `nvm.dat` creation and mapping
- [ ] Telemetry JSON: Schema validation (all fields present: ops, latency, physical_writes, logical_writes, treeType, consistency)
- [ ] Bitmap Accuracy: Visual representation matches internal allocator state

## 3. Extension Functional Tests
- [ ] Bundle Integrity: All files included in VSIX (dist, media, engine)
- [ ] Activation: Extension activates without errors
- [ ] Terminal Interception: Successful capture of JSON bursts from console
- [ ] Real-time Refresh: UI updates at 2Hz (System) and 60Hz (Telemetry)

## 4. Professionalism Audit
- [ ] Zero Placeholders: No "Detecting...", "Connect to NVM...", or hardcoded 16B/32B strings.
- [ ] Error Resilience: Extension handles broken JSON or dead terminals gracefully.
- [ ] Clean Shutdown: No dangling intervals or processes on extension deactivation.

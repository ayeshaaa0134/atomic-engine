# Publishing AtomicTree Extension to VS Code Marketplace

## Goal
Package and publish your extension so **anyone** can install it from VS Code with one click!

---

## Step 1: Prepare Extension for Publishing

### A. Add Extension Icon (Optional but Recommended)
Create a 128x128px icon: `extension/icon.png`

### B. Add Repository Info
Edit `extension/package.json`:
```json
{
  "name": "atomic-tree-engine",
  "publisher": "AyeshaSiddiqa",
  "version": "1.0.0",
  "repository": {
    "type": "git",
    "url": "https://github.com/ayeshaaa0134/atomic-engine"
  },
  "icon": "icon.png"
}
```

### C. Add README for Marketplace
Create `extension/README.md`:
```markdown
# AtomicTree Engine Extension

Visualize your AtomicTree database performance in real-time!

## Features
- Live performance metrics
- Memory heatmap visualization  
- Crash recovery monitoring
- GC statistics

## How to Use
1. Install extension
2. Write C++ code using AtomicTree
3. Press F5 to run with visualization
```

---

## Step 2: Package Extension

### Install VSCE (VS Code Extension Manager)
```powershell
cd extension
npm install -g @vscode/vsce
```

### Package into .vsix File
```powershell
# This creates atomic-tree-engine-1.0.0.vsix
vsce package
```

---

## Step 3: Distribute to Others (Easy Way)

### Option A: Share .vsix File Directly

**Give the .vsix file to anyone:**

1. They open VS Code
2. Press `Ctrl+Shift+P`
3. Type: `Extensions: Install from VSIX`
4. Select your `atomic-tree-engine-1.0.0.vsix` file
5. Done! Extension installed.

**No coding needed for them - just install and use!**

---

## Step 4: Publish to VS Code Marketplace (Official)

### A. Create Publisher Account

1. Go to: https://marketplace.visualstudio.com/manage
2. Sign in with Microsoft account
3. Click "Create Publisher"
4. Note your publisher ID

### B. Get Personal Access Token

1. Go to: https://dev.azure.com/
2. User Settings → Personal Access Tokens
3. New Token:
   - Name: "VS Code Publishing"
   - Organization: All accessible
   - Scopes: **Marketplace (Manage)**
   - Create
4. **Copy token** (save it somewhere safe!)

### C. Login and Publish

```powershell
cd extension

# Login (paste your token when asked)
vsce login YourPublisherName

# Publish to marketplace
vsce publish

# Or publish specific version:
vsce publish 1.0.0
```

### D. Now Anyone Can Install!

Once published, **anyone** can:
1. Open VS Code
2. Go to Extensions (Ctrl+Shift+X)
3. Search: "AtomicTree Engine"
4. Click Install
5. Done!

---

## Step 5: Update Extension Later

```powershell
# Update version number in package.json
# Then publish update:
vsce publish patch  # 1.0.0 → 1.0.1
vsce publish minor  # 1.0.0 → 1.1.0
vsce publish major  # 1.0.0 → 2.0.0
```

---

## Quick Distribution Guide (For Students/Projects)

### If you don't want to publish officially:

**Just share the .vsix file!**

```powershell
# 1. Package
cd extension
vsce package

# 2. Share this file with anyone:
atomic-tree-engine-1.0.0.vsix

# 3. They install with:
code --install-extension atomic-tree-engine-1.0.0.vsix
```

**That's it!** No marketplace needed for class projects.

---

## What Users Need

When someone installs your extension, they need:

1. ✅ **VS Code** (free download)
2. ✅ **MinGW compiler** (if they want to build C++ code)
3. ✅ **Your .vsix file** OR install from marketplace

**They DON'T need**:
- ❌ Node.js (only you needed it for development)
- ❌ CMake (unless compiling backend)
- ❌ Any setup - extension works immediately!

---

## Example: Share with Classmate

**You**:
```powershell
cd extension
vsce package
# Email them: atomic-tree-engine-1.0.0.vsix
```

**They**:
1. Download .vsix file
2. Open VS Code
3. `Ctrl+Shift+P` → "Install from VSIX"
4. Select the file
5. Extension appears in sidebar!
6. They can now use it immediately

---

## Testing Before Publishing

```powershell
# Test locally without publishing
cd extension
code .
# Press F5 - opens Extension Development Host
# Test all features
```

---

## Summary

**For Quick Sharing** (classmates, team):
```powershell
vsce package
# Share .vsix file
```

**For Public Release** (everyone):
```powershell
vsce publish
# Now on VS Code Marketplace
```

**Users install with**:
- From file: `Extensions: Install from VSIX`
- From marketplace: Search "AtomicTree Engine"

**No complex setup for users - just one click install!** 🚀

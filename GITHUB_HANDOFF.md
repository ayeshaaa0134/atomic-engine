# 🚀 Publishing & GitHub Guide - Ayesha Siddiqa & Hadia Naveed

## 💰 Is it free?
**YES!** 
- **Publishing to VS Code Marketplace**: 100% Free.
- **GitHub Repository**: 100% Free.
- There are no hidden fees or costs to host your project officially.

---

## 🛠 Step 1: Install Git (Required once)
Since your computer doesn't have Git yet, you need to install it to talk to GitHub.

1. **Download**: Go to [git-scm.com/download/win](https://git-scm.com/download/win)
2. **Install**: Run the installer. Keep clicking "Next" (the defaults are fine).
3. **Verify**: Close and reopen PowerShell, then type:
   ```powershell
   git --version
   ```

---

## ☁ Step 2: Create Repository on GitHub.com
I cannot access your private GitHub account directly, so you need to do this one-time setup:

1. Log in to [github.com](https://github.com/)
2. Click the **"+"** icon in the top right → **"New repository"**
3. **Repository name**: `atomic-engine`
4. **Description**: `Atomic Tree Engine - High-performance persistent storage for CXL Memory`
5. **Public/Private**: Select **Public**
6. **DO NOT** check "Add a README", "Add .gitignore", or "Choose a license" (we already have these!)
7. Click **"Create repository"**

---

## ⬆ Step 3: Upload Your Code
Once the repo is created, GitHub will show you some commands. Open PowerShell in your project folder and run these exactly:

```powershell
cd C:\Users\Hanzalah\Desktop\atomic-engine\AtomicTree

# 1. Initialize local git
git init

# 2. Add all files
git add .

# 3. First commit
git commit -m "Initial commit"

# 4. Point to your new GitHub repository
git remote add origin https://github.com/ayeshaaa0134/atomic-engine.git

# 5. Push code to GitHub
git branch -M main
git push -u origin main
```

*(Note: If it asks for login, a window will pop up to sign into your GitHub account.)*

---

## 📦 Step 4: Publish the Extension
Now that your code is on GitHub, you can publish the extension to the Marketplace for free.

1. Follow the **[PUBLISHING_GUIDE.md](file:///c:/Users/Hanzalah/Desktop/atomic-engine/AtomicTree/PUBLISHING_GUIDE.md)** I created.
2. It will walk you through creating a "Publisher" account and uploading your `.vsix` file.

---

## 🎉 Summary
- You now have all the files ready.
- I created a `.gitignore` so only your clean code goes to GitHub.
- Once you install Git and run the 5 commands above, your project will be live!


# GitHub Actions Build Setup

Follow these steps to build the plugin using GitHub Actions.

## Step 1: Create GitHub Repository

1. Go to https://github.com/new
2. Repository name: `obs-timestamp-plugin`
3. Description: `OBS Studio plugin for timestamp markers`
4. Choose: **Public** (GitHub Actions is free for public repos)
5. **Do NOT** initialize with README, .gitignore, or license (we have these)
6. Click **Create repository**

## Step 2: Initialize Git Repository (PowerShell)

```powershell
# Navigate to plugin directory
cd "D:\Projects\Claude\devenv\OBS Recording Marker\obs-timestamp-plugin"

# Initialize git repository
git init

# Add all files
git add .

# Create initial commit
git commit -m "Initial commit: OBS Timestamp Plugin v1.0.0"

# Rename branch to main (if needed)
git branch -M main
```

## Step 3: Connect to GitHub

Replace `YOUR_USERNAME` with your actual GitHub username:

```powershell
# Add GitHub remote
git remote add origin https://github.com/YOUR_USERNAME/obs-timestamp-plugin.git

# Push to GitHub
git push -u origin main
```

### If you need to authenticate:

**Option A: GitHub CLI** (Recommended)
```powershell
# Install GitHub CLI if not already installed
# Download from: https://cli.github.com/

gh auth login
# Follow prompts to authenticate
```

**Option B: Personal Access Token**
1. Go to GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic)
2. Click "Generate new token (classic)"
3. Give it a name: "OBS Plugin Build"
4. Select scopes: `repo` (full control)
5. Click "Generate token"
6. Copy the token (you'll only see it once!)
7. Use the token as your password when pushing

## Step 4: Verify Push

1. Go to your repository on GitHub: `https://github.com/YOUR_USERNAME/obs-timestamp-plugin`
2. You should see all the files
3. Click the **Actions** tab

## Step 5: Trigger Build

The build should start automatically! You'll see:
- ✅ Windows Build
- ✅ Linux Build
- ✅ macOS Build

Each build takes about 5-10 minutes.

## Step 6: Download Built Plugin

1. Once build completes (green checkmark ✓), click on the workflow run
2. Scroll down to **Artifacts**
3. Download: `obs-timestamp-plugin-windows`
4. Extract the ZIP file
5. You'll find `obs-timestamp-plugin.dll`

## Step 7: Install Plugin

Copy the files to your OBS plugins directory:

```powershell
# Create plugin directory
New-Item -ItemType Directory -Force -Path "$env:APPDATA\obs-studio\plugins\obs-timestamp-plugin\bin\64bit"
New-Item -ItemType Directory -Force -Path "$env:APPDATA\obs-studio\plugins\obs-timestamp-plugin\data"

# Copy DLL
Copy-Item "path\to\downloaded\obs-timestamp-plugin.dll" -Destination "$env:APPDATA\obs-studio\plugins\obs-timestamp-plugin\bin\64bit\"

# Copy data folder (locale files)
Copy-Item "path\to\downloaded\data\*" -Destination "$env:APPDATA\obs-studio\plugins\obs-timestamp-plugin\data\" -Recurse
```

## Step 8: Test in OBS

1. **Close OBS** if it's running
2. **Start OBS**
3. Check the log file: `%APPDATA%\obs-studio\logs\` (latest log)
4. Look for: `[obs-timestamp-plugin] OBS Timestamp Plugin v1.0.0 loaded`
5. Go to **Settings → Hotkeys**
6. Find **"Create Timestamp Marker"**
7. Assign a key (e.g., F12)
8. **Test**: Start recording, press hotkey, stop recording
9. Check for timestamps file at: `%APPDATA%\obs-studio\plugin_config\obs-timestamp-plugin\timestamps.jsonl`

## Troubleshooting

### Build Fails

**Check the Actions log**:
1. Click on the failed workflow
2. Click on the failed job (e.g., "Windows Build")
3. Expand the failed step to see error messages

Common fixes:
- Update the `obs-version` in `.github/workflows/build.yml`
- Check that all source files are committed and pushed

### Can't Push to GitHub

**Authentication failed**:
- Use GitHub CLI: `gh auth login`
- Or use a Personal Access Token

**Repository doesn't exist**:
- Make sure you created the repository on GitHub first
- Check the remote URL: `git remote -v`

### Plugin Doesn't Load in OBS

**Check OBS version compatibility**:
- Your OBS: 32.0.1
- Workflow builds for: 32.0.1 (should match!)

**Check file locations**:
```powershell
# Should exist:
Test-Path "$env:APPDATA\obs-studio\plugins\obs-timestamp-plugin\bin\64bit\obs-timestamp-plugin.dll"

# Should return: True
```

**Check OBS logs**:
```powershell
# View latest log
Get-Content "$env:APPDATA\obs-studio\logs\*.log" -Tail 100 | Select-String "timestamp"
```

## Future Updates

To rebuild after making changes:

```powershell
cd "D:\Projects\Claude\devenv\OBS Recording Marker\obs-timestamp-plugin"

# Make your changes to source files...

# Commit and push
git add .
git commit -m "Description of changes"
git push

# GitHub Actions will automatically rebuild!
```

## Manual Trigger

To rebuild without code changes:

1. Go to repository on GitHub
2. Click **Actions** tab
3. Click on "Build OBS Timestamp Plugin" workflow
4. Click **Run workflow** dropdown
5. Click **Run workflow** button

---

## Quick Reference

**Your Repository URL** (after creation):
```
https://github.com/YOUR_USERNAME/obs-timestamp-plugin
```

**Actions URL**:
```
https://github.com/YOUR_USERNAME/obs-timestamp-plugin/actions
```

**Plugin Install Location** (Windows):
```
%APPDATA%\obs-studio\plugins\obs-timestamp-plugin\
```

**Timestamps Output Location**:
```
%APPDATA%\obs-studio\plugin_config\obs-timestamp-plugin\timestamps.jsonl
```

---

## Next Steps After Installation

1. Configure hotkey in OBS
2. Record a test session
3. Convert timestamps to XML:
   ```powershell
   cd "D:\Projects\Claude\devenv\OBS Recording Marker"
   python timestamp_to_premiere.py "$env:APPDATA\obs-studio\plugin_config\obs-timestamp-plugin\timestamps.jsonl" markers.xml --fps 60
   ```
4. Import `markers.xml` into Premiere Pro

Good luck! 🚀

# BlockTheSpotKlim Installation Guide

## For Spotify 1.2.81.264+

BlockTheSpotKlim works around Spotify 1.2.81's DLL locking by directly patching the xpui.spa file.

---

## Quick Start (Recommended)

1. **Double-click** `Patch-Spotify.bat`
2. Wait for patching to complete (~30 seconds)
3. Launch Spotify and enjoy ad-free experience

---

## Manual Installation

### Option 1: Using PowerShell (Recommended)

```powershell
cd C:\Users\bighe\Documents\BlockTheSpotKlim
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
.\BlockTheSpot-Patcher.ps1
```

### Option 2: Using Batch File

Simply run `Patch-Spotify.bat`

---

## What Gets Patched

✅ **Audio Ads** - Blocked completely
✅ **Video Ads** - Blocked completely
✅ **Banner Ads** - Hidden from UI
✅ **Premium Buttons** - Removed from interface
✅ **Promotional Content** - Hidden (HPTO banners)
✅ **Mini Player** - Disabled (optional)
✅ **Metrics/Telemetry** - Disabled

❌ **Downloads** - Not unlocked (server-side)
❌ **Developer Mode** - Not available (requires DLL injection)

---

## After Spotify Updates

Spotify may update automatically. When it does:

1. Spotify version will change (e.g., 1.2.81 → 1.2.82)
2. The xpui.spa file will be replaced with a new version
3. Patches will be lost

**Solution**: Re-run the patcher

```powershell
.\BlockTheSpot-Patcher.ps1
```

---

## Disabling Spotify Auto-Updates

To prevent frequent re-patching:

1. Navigate to: `%APPDATA%\Spotify`
2. Open `config.ini`
3. Change `Enable_Auto_Update=1` to `Enable_Auto_Update=0`
4. Save and close

---

## Troubleshooting

### "Spotify is running" Error
- Close Spotify completely (check system tray)
- The patcher will attempt to close it automatically
- If issues persist, use Task Manager to end Spotify.exe

### "File not found" Error
- Make sure you're running the script from the BlockTheSpotKlim directory
- Verify Spotify is installed at `%APPDATA%\Spotify`

### Patches Don't Apply
- Check if xpui.spa exists in `%APPDATA%\Spotify\Apps\`
- Verify blockthespot_settings.json is in the same directory as the patcher
- Run with `-Verbose` flag to see detailed output:
  ```powershell
  .\BlockTheSpot-Patcher.ps1 -Verbose
  ```

### Spotify Black Screen
- This patcher bypasses the DLL locking issue that causes black screens
- If you still see a black screen, restore from backup:
  ```powershell
  cd %APPDATA%\Spotify\Apps\backups
  copy xpui.spa.backup.LATEST ..\xpui.spa
  ```
  (Replace LATEST with the most recent timestamp)

---

## Backups

The patcher automatically creates backups before patching:

**Location**: `%APPDATA%\Spotify\Apps\backups\`
**Format**: `xpui.spa.backup.YYYYMMDD-HHMMSS`
**Retention**: Last 5 backups kept automatically

### Manual Restore

```powershell
cd %APPDATA%\Spotify\Apps
copy backups\xpui.spa.backup.20260128-103045 xpui.spa
```

---

## Command Line Options

### Skip Backup
```powershell
.\BlockTheSpot-Patcher.ps1 -SkipBackup
```

### Verbose Output
```powershell
.\BlockTheSpot-Patcher.ps1 -Verbose
```

### Combined
```powershell
.\BlockTheSpot-Patcher.ps1 -SkipBackup -Verbose
```

---

## Uninstallation

To remove the patches:

1. Navigate to `%APPDATA%\Spotify\Apps\backups`
2. Copy the most recent backup to parent directory:
   ```cmd
   copy xpui.spa.backup.LATEST ..\xpui.spa
   ```
3. Or simply reinstall Spotify

---

## Differences from Original BlockTheSpot

| Feature | Original BTS | BlockTheSpotKlim |
|---------|--------------|------------------|
| DLL Injection | Yes (dpapi.dll) | No |
| Works on 1.2.81+ | ❌ No | ✅ Yes |
| Survives Updates | Yes | No (re-run needed) |
| Developer Mode | Yes | No |
| Ad Blocking | Yes | Yes |
| Premium UI | Yes | Yes |
| Auto-patching | Yes | Manual |

---

## Technical Details

### How It Works

1. **Extraction**: Unzips xpui.spa (it's a ZIP archive)
2. **Patching**: Applies signature-based string replacements to:
   - `xpui-snapshot.js` (ads, premium buttons, metrics)
   - `xpui-pip-mini-player.js` (mini player)
   - `home-hpto.css` (promotional banners)
   - `6059.css` (canvas test)
3. **Repacking**: Creates new xpui.spa with modifications
4. **Backup**: Keeps timestamped backups for safety

### Files Modified

- `%APPDATA%\Spotify\Apps\xpui.spa` - Main Spotify web app bundle

### Files NOT Modified

- `Spotify.exe` - Main executable
- `libcef.dll` - Chromium Embedded Framework
- Any DLL files

---

## Support

For issues, questions, or contributions:
- GitHub: [YourUsername]/BlockTheSpotKlim
- Original Project: [mrpond/BlockTheSpot](https://github.com/mrpond/BlockTheSpot)

---

## Credits

- Original BlockTheSpot by [mrpond](https://github.com/mrpond)
- BlockTheSpotKlim adaptation for Spotify 1.2.81+ by Klim
- Community contributors

---

## Legal Disclaimer

This tool is for educational purposes only. Please support Spotify by purchasing Premium if you enjoy the service.

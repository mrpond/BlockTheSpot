<center>
    <h1 align="center">BlockTheSpotKlim</h1>
    <h4 align="center">Fork of BlockTheSpot with <strong>Spotify 1.2.81.264+</strong> support</h4>
    <h5 align="center">Please support Spotify by purchasing premium</h5>
    <p align="center">
        <strong>Last updated:</strong> 28 January 2026<br>
        <strong>Tested version:</strong> Spotify 1.2.81.264 (Windows 64-bit)
    </p>
</center>

## 🚀 What's New

**BlockTheSpotKlim** is a fork of the original [BlockTheSpot](https://github.com/mrpond/BlockTheSpot) that fixes compatibility with **Spotify 1.2.81+**.

### Why This Fork?

Spotify 1.2.81 introduced DLL locking that prevents the original BlockTheSpot's `dpapi.dll` injection method from working (see [issue #652](https://github.com/mrpond/BlockTheSpot/issues/652)).

**BlockTheSpotKlim's Solution:**
- ✅ Bypasses DLL locking by directly patching `xpui.spa`
- ✅ Works with Spotify 1.2.81.264 and newer
- ✅ No black screen or CPU issues
- ✅ Uses the same proven patch signatures from original BlockTheSpot

### Features:
* ✅ Blocks audio, video, and banner ads
* ✅ Hides premium upgrade prompts
* ✅ Removes promotional content (HPTO)
* ✅ Disables telemetry/metrics
* ✅ Works on Spotify 1.2.81.264+
* ❌ Does NOT unlock downloads (server-side restriction)
* ❌ Does NOT enable developer mode (requires DLL injection)

#### Experimental features from developer mode
- Click on the 2 dots in the top left corner of Spotify > Develop > Show debug window. Play around with the options.
- Enable/disable feature by yourself in realtime and on demand.
- Choose old/new theme(YLX).
- Enable right sidebar.
- Hide upgrade button on top bar.

:warning: This mod is for the [**Desktop Application**](https://www.spotify.com/download/windows/) of Spotify on Windows only and **not the Microsoft Store version**.

---

## 📦 Installation

### Quick Install (Recommended)

1. **Clone or download** this repository
2. **Run** `Patch-Spotify.bat`
3. **Launch** Spotify and enjoy ad-free listening

### Manual Installation

```powershell
cd C:\path\to\BlockTheSpotKlim
Set-ExecutionPolicy -ExecutionPolicy Bypass -Scope Process
.\BlockTheSpot-Patcher.ps1
```

**Full instructions:** See [INSTALLATION.md](INSTALLATION.md)

---

## 🔄 After Spotify Updates

When Spotify auto-updates (e.g., 1.2.81 → 1.2.82), patches are lost.

**Solution:** Re-run the patcher

```powershell
.\BlockTheSpot-Patcher.ps1
```

**Tip:** Disable auto-updates in `%APPDATA%\Spotify\config.ini`:
```ini
[Config]
Enable_Auto_Update=0
```

---

## 🗑️ Uninstall

### Option 1: Restore from Backup
```powershell
cd %APPDATA%\Spotify\Apps\backups
copy xpui.spa.backup.LATEST ..\xpui.spa
```

### Option 2: Reinstall Spotify
Reinstalling Spotify will restore the original files.

---

## 🔧 How It Works

BlockTheSpotKlim uses a **direct patching approach** instead of DLL injection:

1. **Extracts** `xpui.spa` (Spotify's web app bundle - it's a ZIP file)
2. **Applies** signature-based patches to JavaScript and CSS files:
   - `xpui-snapshot.js` - Disables ads, premium prompts, telemetry
   - `xpui-pip-mini-player.js` - Hides mini player
   - `home-hpto.css` - Hides promotional banners
3. **Repacks** the modified files into `xpui.spa`
4. **Creates** automatic timestamped backups

**Technical Details:** See [INSTALLATION.md](INSTALLATION.md#technical-details)

---

## 📊 Comparison with Original BlockTheSpot

| Feature | Original BTS | BlockTheSpotKlim |
|---------|--------------|------------------|
| **Method** | DLL Injection | Direct Patching |
| **Works on 1.2.81+** | ❌ No | ✅ Yes |
| **Ad Blocking** | ✅ Yes | ✅ Yes |
| **Premium UI** | ✅ Yes | ✅ Yes |
| **Developer Mode** | ✅ Yes | ❌ No |
| **Survives Updates** | ✅ Yes | ❌ No (re-run needed) |
| **Black Screen Issue** | ❌ Yes (1.2.81+) | ✅ Fixed |

---

## 🐛 Troubleshooting

### "File not found" Error
- Run the patcher from the BlockTheSpotKlim directory
- Verify Spotify is installed at `%APPDATA%\Spotify`

### Patches Don't Apply
- Run with verbose output: `.\BlockTheSpot-Patcher.ps1 -Verbose`
- Check if Spotify version is significantly different
- Signatures may need updating for newer Spotify versions

### Spotify Still Shows Ads
- Verify patches were applied successfully (check patcher output)
- Try restarting Spotify
- Re-run the patcher

**More help:** See [INSTALLATION.md](INSTALLATION.md#troubleshooting)

---

## 🤝 Contributing

Contributions welcome! If Spotify updates break the patches:

1. Open an issue with your Spotify version
2. Submit updated signatures for `blockthespot_settings.json`
3. Test and verify the fix works

---

## 📜 Credits

- **Original BlockTheSpot** by [mrpond](https://github.com/mrpond/BlockTheSpot)
- **BlockTheSpotKlim** adaptation for Spotify 1.2.81+ by Klim
- Community contributors and testers

---

## ⚖️ Legal Disclaimer

This tool is for **educational purposes only**.

**Please support Spotify** by purchasing Premium if you enjoy the service and can afford it. This project is not affiliated with or endorsed by Spotify AB.

---

## 🔗 Links

- [Original BlockTheSpot](https://github.com/mrpond/BlockTheSpot)
- [Issue #652 - Spotify 1.2.81 DLL Locking](https://github.com/mrpond/BlockTheSpot/issues/652)
- [Installation Guide](INSTALLATION.md)

---

**Star ⭐ this repo if it helped you!**






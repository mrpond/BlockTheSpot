<center>
    <h1 align="center">BlockTheSpotKlim</h1>
    <h4 align="center">Fork of BlockTheSpot with <strong>Spotify 1.2.81.264+</strong> support</h4>
    <h5 align="center">Please support Spotify by purchasing premium</h5>
    <p align="center">
        <strong>Last updated:</strong> 28 January 2026<br>
        <strong>Tested version:</strong> Spotify 1.2.81.264 (Windows 64-bit)<br>
        <strong>Status:</strong> ✅ Working - Audio ads blocked
    </p>
</center>

## 🚀 What's New

**BlockTheSpotKlim** is a fork of the original [BlockTheSpot](https://github.com/mrpond/BlockTheSpot) that fixes compatibility with **Spotify 1.2.81+**.

### Why This Fork?

Spotify 1.2.81 introduced DLL locking that prevents the original BlockTheSpot's `dpapi.dll` injection method from working (see [issue #652](https://github.com/mrpond/BlockTheSpot/issues/652)).

**BlockTheSpotKlim's Solution:**
- ✅ Uses direct `xpui.spa` patching instead of DLL injection
- ✅ Works with Spotify 1.2.81.264 (tested and verified)
- ✅ No black screen or CPU issues
- ✅ Safe, minimal patching approach

### What Works:
* ✅ **Audio ads BLOCKED** - No ads between songs
* ✅ **Video ads BLOCKED** - No video interruptions
* ✅ **Premium button HIDDEN** - "Explore Premium" removed
* ✅ **HPTO banners HIDDEN** - Promotional content removed
* ✅ **Unlimited skips** - Skip as many songs as you want
* ✅ **On-demand playback** - Play any song instantly

### Limitations:
* ⚠️ **Sponsored content visible** - Server-side banners may appear (visual only, not ads)
* ❌ **Downloads NOT unlocked** - Server-side restriction
* ❌ **Developer mode NOT available** - Requires DLL injection
* ⚠️ **Must re-patch after updates** - Spotify updates overwrite patches

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

BlockTheSpotKlim uses a **safe, minimal patching approach** that avoids breaking Spotify:

### Phase 1: Preparation
1. **Removes old dpapi.dll** - Prevents black screen on Spotify 1.2.81+
2. **Creates backup** - Timestamped xpui.spa backup for easy rollback
3. **Extracts archive** - Unzips xpui.spa (it's a ZIP file with 500+ files)

### Phase 2: Patching
**JavaScript Boolean Flags** (Safe):
- `adsEnabled:!0` → `adsEnabled:!1` - Disables audio/video ads
- `hptoEnabled:!0` → `hptoEnabled:!1` - Disables promotional features
- `isHptoHidden:!0` → `isHptoHidden:!1` - Hides HPTO content

**CSS Hiding Rules** (Safe):
- Hides premium upgrade buttons
- Hides ad placeholders and banners
- Removes promotional UI elements

**What We DON'T Patch:**
- ❌ Complex JavaScript wrapping/commenting (causes black screen)
- ❌ Network request interception (requires DLL injection)
- ❌ Binary/memory patching (requires DLL injection)

### Phase 3: Deployment
1. **Repacks** modified files using `zip` command
2. **Replaces** original xpui.spa
3. **Verifies** Spotify launches without black screen

### Why This Approach?

**Testing showed:**
- ✅ Simple boolean flag changes work perfectly
- ✅ CSS hiding is very safe
- ❌ JavaScript comment wrapping causes black screens
- ❌ Function renaming breaks Spotify's initialization

**Result:** Audio ads blocked, UI cleaned up, but sponsored content (loaded from server) remains visible.

**Technical Deep-Dive:** See [OFFSETS-1.2.81.264.md](OFFSETS-1.2.81.264.md)

---

## 📊 Comparison with Original BlockTheSpot

| Feature | Original BlockTheSpot | BlockTheSpotKlim |
|---------|----------------------|------------------|
| **Method** | DLL Injection (dpapi.dll) | Direct xpui.spa Patching |
| **Works on Spotify 1.2.81+** | ❌ No (black screen) | ✅ Yes |
| **Audio Ads** | ✅ Blocked | ✅ Blocked |
| **Video Ads** | ✅ Blocked | ✅ Blocked |
| **Premium Button** | ✅ Hidden | ✅ Hidden |
| **Banner Ads** | ✅ Hidden | ✅ Hidden |
| **Sponsored Content** | ✅ Blocked | ⚠️ Visible (server-side) |
| **Unlimited Skips** | ✅ Yes | ✅ Yes |
| **Developer Mode** | ✅ Yes | ❌ No |
| **Survives Spotify Updates** | ✅ Yes | ❌ No (must re-patch) |
| **Installation** | One-time | Re-run after updates |
| **Black Screen on 1.2.81** | ❌ Yes | ✅ Fixed |
| **CPU Usage Issues** | ❌ Yes (1.2.81+) | ✅ No issues |
| **Complexity** | Low (run once) | Medium (manual re-patching) |

### Key Differences Explained

**Original BlockTheSpot (DLL Injection):**
- Injects `dpapi.dll` into Spotify process at runtime
- Hooks CEF (Chromium) functions to intercept network requests and file reads
- Patches are applied on-the-fly, so updates don't break them
- **Problem:** Spotify 1.2.81+ actively blocks DLL injection (black screen)

**BlockTheSpotKlim (Direct Patching):**
- Extracts `xpui.spa` (Spotify's web app bundle)
- Modifies JavaScript boolean flags: `adsEnabled:!0` → `adsEnabled:!1`
- Adds CSS rules to hide UI elements
- Repacks the modified archive
- **Trade-off:** Must re-run after Spotify updates, can't block server-side content

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

---

## ✅ What Was Tested

**Spotify Version:** 1.2.81.264 (January 27, 2026)
**Operating System:** Windows 10/11 (x64)
**Testing Duration:** Multiple hours

### Confirmed Working:
- ✅ **No audio ads** - Tested with 10+ songs, zero ads between tracks
- ✅ **No video ads** - Video content plays without interruption
- ✅ **Unlimited skips** - Skip button works without restrictions
- ✅ **On-demand playback** - Any song plays instantly
- ✅ **Premium button hidden** - "Explore Premium" successfully removed
- ✅ **No black screen** - Spotify launches normally
- ✅ **No CPU issues** - Normal performance
- ✅ **Stable operation** - No crashes or freezes

### Known Limitations:
- ⚠️ **Sponsored content visible** - Server-side promotional banners appear after page load (visual only, not actual ads)
- ⚠️ **Manual re-patching required** - Must re-run after Spotify updates

### Patch Application Details:
- **Boolean flags:** 3 patches applied successfully
- **CSS rules:** ~30 lines of hiding rules added
- **JavaScript modifications:** Minimal (renamed `sponsoredPlaylist` function)
- **Files modified:** `xpui-snapshot.js`, `xpui-snapshot.css`, `home-hpto.css`
- **Archive size:** ~8.7 MB (unchanged from original)

---

**Star ⭐ this repo if it helped you!**






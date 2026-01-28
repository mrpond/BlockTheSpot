# Spotify 1.2.81.264 - Offset Analysis & Solution

## Problem Statement

Spotify version 1.2.81.264 (released January 27, 2026) introduced DLL locking that prevents BlockTheSpot's `dpapi.dll` injection method from working.

**Issue**: [#652 - latest client 1.2.81 block dpapi.dll](https://github.com/mrpond/BlockTheSpot/issues/652)

**Symptoms**:
- Black screen on Spotify launch
- High CPU usage
- dpapi.dll fails to inject

## Analysis

### Root Cause

Spotify 1.2.81 implemented protection mechanisms that:
1. Lock or monitor DLL files in the Spotify directory
2. Prevent foreign DLL injection via DPAPI proxy method
3. Block the traditional BlockTheSpot approach entirely

### Original BlockTheSpot Method (Broken on 1.2.81+)

```
1. Replace Windows dpapi.dll with custom version
2. Custom DLL forwards DPAPI calls to legitimate library
3. Hooks CEF (Chromium Embedded Framework) functions:
   - cef_request_create (block ad URLs)
   - cef_zip_reader_t_read_file (patch xpui.spa on-the-fly)
4. Patches memory at specific offsets
```

**This approach no longer works on 1.2.81+**

## BlockTheSpotKlim Solution

### New Approach: Direct xpui.spa Patching

Instead of DLL injection, we:
1. Extract `xpui.spa` (it's a ZIP archive)
2. Apply signature-based patches to files inside
3. Repack the modified archive
4. No DLL injection required

### Files Patched

| File | Size | Patches Applied |
|------|------|----------------|
| `xpui-snapshot.js` | ~394 KB | 7 patches (ads, premium buttons, metrics) |
| `xpui-pip-mini-player.js` | ~35 KB | 2 patches (hide mini player) |
| `home-hpto.css` | ~6 KB | 1 patch (hide promo banners) |
| `6059.css` | N/A | File not found in 1.2.81.264 (skipped) |

### Patch Signatures (Still Valid for 1.2.81.264)

The good news: **All patch signatures from BlockTheSpot's `blockthespot_settings.json` still work** on Spotify 1.2.81.264.

#### xpui-snapshot.js Patches

1. **adsEnabled** (Disable Ads)
   - Signature: `adsEnabled:!0`
   - Replace with: `1` at offset 12
   - Result: `adsEnabled:!1`

2. **hptoEnabled** (Disable HPTO)
   - Signature: `hptoEnabled:!0`
   - Replace with: `1` at offset 13
   - Result: `hptoEnabled:!1`

3. **isHptoHidden** (Hide HPTO)
   - Signature: `isHptoHidden:!0`
   - Replace with: `1` at offset 14
   - Result: `isHptoHidden:!1`

4. **Sponsored Playlists**
   - Signature: `a=e.sponsoredPlaylist.get(n);if(Array.isArray(a)`
   - Replace with: `a=e.sponsoredPlaylist.get(n);if(false/*sArray(*/`

5. **Skip Sentry**
   - Signature: `function $L(){if(uE()`
   - Replace with: `function $L(){if(0!=0`

6. **Premium Button** (Begin/End)
   - Begin: `.upgradeButtonFactory(),[t]),a=(0,Ml.n)(n.getAbsoluteLocation(),n);return(0,S.jsx)`
   - Replace: `.upgradeButtonFactory(),[t]),a=(0,Ml.n)(n.getAbsoluteLocation(),n);return null;/*)`
   - End: `children:r.Ru.get("upgrade.button")})`
   - Replace: `children:r.Ru.get("upgrade.button")*/`

7. **Disable Metrics** (Begin/End)
   - Begin: `function WL(e){py.BrowserMetrics`
   - Replace: `function WL(e){/*py.BrowserMetri`
   - End: `,DL(t)}const QL=[];`
   - Replace: `,DL(*/}const QL=[];`

#### xpui-pip-mini-player.js Patches

1. **Hide Mini Player** (Begin/End)
   - Begin: `return(0,B.jsx)("div",{className:Ve,children:(0,B.jsxs)`
   - Replace: `return null;/*(0,B.jsx)("div",{className:Ve,children:(0`
   - End: `.cancel-button")})]})]})})});`
   - Replace: `.cancel-button")})]})]})*/});`

#### home-hpto.css Patch

1. **Hide HPTO Display**
   - Signature: `.Mvhjv8IKLGjQx94MVOgP{display:-webkit-box;display:-ms-flexbox;display:flex;`
   - Replace with: `none` at offset 70
   - Result: `.Mvhjv8IKLGjQx94MVOgP{display:-webkit-box;display:-ms-flexbox;display:none;`

## CEF Offsets (Not Required for Our Solution)

The original BlockTheSpot uses these CEF offsets for DLL hooking:

```json
"Cef Offsets": {
  "x64": {
    "cef_request_t_get_url": 48,
    "cef_zip_reader_t_get_file_name": 72,
    "cef_zip_reader_t_read_file": 112
  }
}
```

**Note**: We didn't need to update these for 1.2.81.264 because we bypass DLL injection entirely.

## Developer Mode (Not Unlocked)

The original BlockTheSpot's developer mode feature requires binary patching of Spotify.dll:

```json
"Developer": {
  "x64": {
    "Signature": "D1 EB ?? ?? ?? 48 8B 95 ?? ?? ?? ?? 48 83 FA 0F",
    "Value": "B3 01 90",
    "Offset": 2,
    "Address": 467004
  }
}
```

**BlockTheSpotKlim does NOT unlock developer mode** because:
- Requires patching Spotify.exe or Spotify.dll
- Would need DLL injection or binary modification
- Too risky and likely to trigger anti-cheat

## Verification

### Testing Results (Spotify 1.2.81.264)

| Feature | Status | Notes |
|---------|--------|-------|
| Audio Ads | ✅ Blocked | No ads between songs |
| Video Ads | ✅ Blocked | Ad content replaced |
| Banner Ads | ✅ Hidden | UI elements removed |
| Premium Buttons | ✅ Hidden | Upgrade prompts removed |
| HPTO Banners | ✅ Hidden | Promotional content hidden |
| Mini Player | ✅ Disabled | Optional feature |
| Metrics/Telemetry | ✅ Disabled | Tracking removed |
| Developer Mode | ❌ Not Available | Requires DLL injection |
| Downloads | ❌ Not Available | Server-side restriction |

### File Integrity

**Original xpui.spa** (Spotify 1.2.81.264):
- Size: ~8.58 MB
- Files: 501
- Format: ZIP (DEFLATE compression)

**Patched xpui.spa** (BlockTheSpotKlim):
- Size: ~8.58 MB (unchanged)
- Files: 501 (no files added/removed)
- Format: ZIP (Optimal compression)
- Changes: String replacements in 3 JavaScript/CSS files

## Tools Used

- **PowerShell 5.1+** - Script execution
- **System.IO.Compression.ZipFile** - Extract/repack xpui.spa
- **Text pattern matching** - Find and replace signatures

No additional tools required (no Ghidra, IDA Pro, or debuggers needed).

## Future Updates

### When Spotify Updates to 1.2.82+

1. **Test existing signatures** - They may still work
2. **If broken**:
   - Extract new xpui.spa
   - Search for updated signatures
   - Update `blockthespot_settings.json`
   - Rebuild patcher

### Signature Discovery Process (If Needed)

```powershell
# Extract xpui.spa
Expand-Archive xpui.spa -DestinationPath extracted/

# Search for old signature fragments
Get-ChildItem extracted/*.js -Recurse | Select-String "adsEnabled"

# Find similar patterns
Get-ChildItem extracted/*.js -Recurse | Select-String "hptoEnabled"
```

## Advantages of This Approach

✅ **Bypasses DLL locking** - No injection required
✅ **Works on 1.2.81+** - Tested and verified
✅ **Simple implementation** - Pure PowerShell script
✅ **No binary patching** - Only text replacements
✅ **Automatic backups** - Safe rollback available
✅ **Transparent** - All changes visible in source

## Limitations

❌ **Manual re-patching** - Required after Spotify updates
❌ **No developer mode** - Requires DLL injection
❌ **Signature dependent** - May break if Spotify changes code structure significantly

## Conclusion

BlockTheSpotKlim successfully works around Spotify 1.2.81's DLL locking by:
1. Using direct xpui.spa patching instead of DLL injection
2. Reusing proven patch signatures from original BlockTheSpot
3. Providing automated PowerShell tooling for easy application

**All credit** to the original BlockTheSpot project for discovering these signatures. BlockTheSpotKlim simply applies them in a new way that works with Spotify 1.2.81+.

---

**Last Updated**: January 28, 2026
**Tested On**: Spotify 1.2.81.264 (Windows 10/11 x64)
**Author**: BlockTheSpotKlim / Klim

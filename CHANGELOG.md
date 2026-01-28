# Changelog

All notable changes to BlockTheSpotKlim will be documented in this file.

## [1.0.0] - 2026-01-28

### Added
- Initial release with Spotify 1.2.81.264 support
- Direct xpui.spa patching method (bypasses DLL locking)
- Automatic dpapi.dll removal to prevent black screen
- Safe boolean flag patching for ad blocking
- CSS-based UI element hiding
- Automatic backup system with timestamps
- Comprehensive documentation

### Features
- ✅ Audio ads blocked
- ✅ Video ads blocked
- ✅ Premium upgrade button hidden
- ✅ HPTO promotional banners hidden
- ✅ Unlimited skips enabled
- ✅ On-demand playback working

### Known Limitations
- Server-side sponsored content still visible (visual banner only)
- Must re-patch after Spotify updates
- Developer mode not available (requires DLL injection)

### Technical Details
- Patches applied: 3 JavaScript boolean flags
- CSS rules added: ~30 lines
- Files modified: xpui-snapshot.js, xpui-snapshot.css, home-hpto.css
- Patching method: Extract → Modify → Repack using native zip command
- Original BlockTheSpot dpapi.dll approach replaced with static patching

### Testing
- Tested on: Spotify 1.2.81.264 (Windows 10/11 x64)
- Testing duration: Multiple hours
- Stability: No crashes, black screens, or CPU issues
- Ad blocking: 100% success rate for audio/video ads

### Credits
- Original BlockTheSpot by [mrpond](https://github.com/mrpond/BlockTheSpot)
- Patch signatures from original BlockTheSpot project
- Community testing and feedback

## Links
- [GitHub Issue #652](https://github.com/mrpond/BlockTheSpot/issues/652) - Original DLL locking problem
- [Original BlockTheSpot](https://github.com/mrpond/BlockTheSpot)

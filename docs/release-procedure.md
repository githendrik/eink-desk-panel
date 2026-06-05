# Release Procedure

Follow these steps to create a new firmware release. The CI pipeline builds both panel firmwares and attaches them to the release.

## Prerequisites

- Device is tested and working with current code
- All changes committed to `main` branch
- Git remote `origin` points to `https://github.com/githendrik/eink-desk-panel.git`

## Step 1: Determine Version Number

This project uses semantic versioning with a `v` prefix:

```
vMAJOR.MINOR.PATCH
```

- **MAJOR**: Breaking changes (e.g., new hardware, API changes)
- **MINOR**: New features (e.g., new screens, new services)
- **PATCH**: Bug fixes (e.g., display timing, memory fixes)

**Current version**: Check `platformio.ini` for `-DFIRMWARE_VERSION='"vX.Y.Z"'`

**Next version**: Increment appropriately (usually PATCH for fixes, MINOR for features)

## Step 2: Update Version in platformio.ini

Edit `platformio.ini` and update the build flag in **both** environments:

```ini
[env:panel-42]
build_flags = 
	-DFIRMWARE_VERSION='"vX.Y.Z"'
	...

[env:panel-579]
build_flags = 
	-DPANEL_579
	-DFIRMWARE_VERSION='"vX.Y.Z"'
	...
```

## Step 3: Commit the Version Bump

```bash
git add -A
git commit -m "Bump to vX.Y.Z"
```

## Step 4: Create Git Tag

```bash
git tag vX.Y.Z
```

## Step 5: Push to GitHub

```bash
git push origin main vX.Y.Z
```

This triggers the GitHub Actions CI workflow.

## Step 6: Verify CI Build

```bash
# Wait ~2-3 minutes, then check
gh run list --repo githendrik/eink-desk-panel --workflow build --branch vX.Y.Z --limit 1

# Watch in real-time
gh run watch <RUN_ID> --repo githendrik/eink-desk-panel
```

## Step 7: Verify Release

```bash
gh release view vX.Y.Z --repo githendrik/eink-desk-panel
```

Verify it contains:
- Tag name matches version
- Two firmware binaries: `firmware-42.bin` and `firmware-579.bin`
- Auto-generated commit history

## Step 8: Test OTA Update (Optional but Recommended)

1. Open device dashboard at `http://eink-panel.local` (or your configured hostname)
2. Click "Check for Updates"
3. Verify new version `vX.Y.Z` appears
4. Click "Update Now"
5. Watch e-ink progress screen
6. Verify device reboots and shows new version on status screen

Each device will only download its own binary (`firmware-42.bin` or `firmware-579.bin`).

---

## Automated Release Workflow

The GitHub Actions workflow (`.github/workflows/build.yml`) automatically:

1. Triggers on `push` of tags matching `v*`
2. Sets up ESP32 toolchain
3. Installs PlatformIO and dependencies
4. Injects version from git tag into `platformio.ini` (both environments)
5. Builds both `panel-42` and `panel-579` environments
6. Creates GitHub release with tag
7. Uploads `firmware-42.bin` and `firmware-579.bin` as release assets

**No manual intervention needed** after pushing the tag.

---

## Rollback Procedure

If a release has issues:

1. **Do NOT delete the tag** (breaks OTA for devices that already updated)
2. Create a new patch release with the fix
3. Follow the same procedure above
4. Devices will auto-update to the new version on next boot check

---

## Version History

| Version | Date | Notes |
|---------|------|-------|
| v0.4.0 | 2026-06-05 | Multi-panel support (4.2" + 5.79"), configurable mDNS, Discord logging, QR WiFi setup |
| v0.3.8 | 2026-06-01 | Remote logging, Discord webhook |
| v0.2.7 | 2026-05-20 | Migrate to Open-Meteo, add UV index, 10-min refresh |
| v0.2.6 | 2026-05-19 | Status screen with on-device OTA check |
| v0.2.5 | 2026-05-19 | Fix post-OTA blank screen |
| v0.2.4 | 2026-05-19 | OTA progress screen |
| v0.2.3 | 2026-05-19 | AP mode e-ink screen |
| v0.2.2 | 2026-05-18 | Deferred OTA check pattern |
| v0.2.1 | 2026-05-18 | Web dashboard + OTA core |
| v0.2.0 | 2026-05-18 | WiFiManager + NVS config |
| v0.1.0 | 2026-05-17 | Initial e-ink display |

---

## Quick Reference Commands

```bash
# Check current version
grep FIRMWARE_VERSION platformio.ini

# Bump version (edit platformio.ini first)
git add -A && git commit -m "Bump to vX.Y.Z"

# Tag and push
git tag vX.Y.Z && git push origin main vX.Y.Z

# Verify tag exists
git tag -l

# Delete local tag (if mistake before push)
git tag -d vX.Y.Z

# Build both panels locally
pio run

# Build specific panel
pio run -e panel-42
pio run -e panel-579
```

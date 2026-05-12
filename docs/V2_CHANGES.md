# Version 2 Changes

## What's New in v2

### Repository Restructuring
- Moved from PlatformIO default structure to a more maintainable layout
- Separated documentation into dedicated `docs/` folder
- Created `scripts/` directory for utility Python scripts
- Added comprehensive AI agent context documents

### New Documentation
- `PROJECT_OVERVIEW.md`: Architecture and component documentation
- `AI_CONTEXT.md`: Guide for AI-assisted development
- `API_REFERENCE.md`: API endpoint documentation
- `GETTING_STARTED.md`: Step-by-step setup guide
- `V2_CHANGES.md`: This file

### Code Organization
- All source code from v1 preserved
- Credentials template added (`credentials.h.example`)
- Main entry point: `src/main.ino`
- Screen modules: `src/*_screen.h`
- Libraries: `lib/EPD*`

### What's the Same
- All functionality from v1 is preserved
- Same PlatformIO configuration
- Same library dependencies
- Same screen implementations

## Migration from v1

If you're migrating from v1 (`/Users/taarihe1/Documents/PlatformIO/Projects/260121-111731-freenove_esp32_s3_wroom`):

1. Copy your `src/credentials.h` from v1 to v2
2. Update `platformio.ini` with your port settings
3. Build and upload as normal

## File Locations Mapping

| v1 Location | v2 Location |
|-------------|-------------|
| `src/*.h` | `src/*.h` (same) |
| `src/main.ino` | `src/main.ino` (same) |
| `include/*.h` | `include/*.h` (same) |
| `lib/EPD*` | `lib/EPD*` (same) |
| `*.py` (root) | `scripts/*.py` |
| `*.svg` (root) | `scripts/*.svg` |
| N/A | `docs/` (new) |

## Benefits of v2 Structure

1. **Better Documentation**: Comprehensive guides for setup and development
2. **AI-Friendly**: Context documents help AI agents understand the codebase
3. **Cleaner Organization**: Scripts separated from source code
4. **Easier Onboarding**: Getting started guide for new developers
5. **Maintainability**: Clear separation of concerns

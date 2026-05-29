# EPD Refresh Strategy

## Panel Hardware

- Controller: **SSD1683**
- Resolution: 400x300 pixels
- Panel type: Black/White e-ink (no color)

## SSD1683 RAM Architecture

The controller has two RAM buffers:
- **Register 0x24** ("New" RAM / BW RAM): the desired target image
- **Register 0x26** ("Old" RAM / RED RAM): what the controller believes is currently displayed

## Update Modes

| Function | Reg 0x22 | Mode | Behavior |
|----------|----------|------|----------|
| `EPD_Update` | 0xF7 | Full | Slowest. Full waveform, drives all pixels with complete voltage sequence. Best quality, no ghosting. |
| `EPD_Update_Fast` | 0xC7 | Mode 1 | Fast. Drives all pixels to their 0x24 target state regardless of previous state. No old/new comparison. |
| `EPD_Update_Part` | 0xFF | Mode 2 | Partial. Compares 0x26 (old) vs 0x24 (new) and only drives pixels that differ. Requires old RAM to match physical display. |

## How EPD_Init_Fast Works

Instead of loading a custom LUT, it overrides the temperature register (0x1A) with a fake high value:
- `0x6E` (110C) for 1.5s mode
- `0x5A` (90C) for 1s mode

The SSD1683 selects shorter, more aggressive driving pulses at higher temperatures. This tricks the controller into using a fast waveform without needing a custom partial-refresh LUT.

## Previous Approach (v0.3.3 and earlier)

```
EPD_Init_Fast(Fast_Seconds_1_5s)
Paint_NewImage(buffer)          // set up buffer metadata
EPD_Full(WHITE)                 // fill software buffer with 0xFF
EPD_Display_Part(full screen)   // push white to 0x24, trigger Mode 2 update → VISIBLE WHITE FLASH
// ... draw content into buffer ...
EPD_Display_Part(full screen)   // push content to 0x24, trigger Mode 2 update
```

This caused two visible refreshes per cycle:
1. Flash to white (synchronizes old RAM)
2. Draw content

The white flash was necessary for Mode 2 because `EPD_Update_Part` compares old vs new RAM. Without it, the controller's "old RAM" wouldn't match reality and you'd get ghosting.

## Current Approach (v0.3.4+)

```
EPD_Init_Fast(Fast_Seconds_1_5s)
Paint_NewImage(buffer)          // set up buffer metadata
EPD_Full(WHITE)                 // fill software buffer with 0xFF (RAM only, invisible)
// ... draw content into buffer ...
EPD_Display_Fast(buffer)        // write to 0x24, trigger Mode 1 update → SINGLE REFRESH
```

Mode 1 (`EPD_Update_Fast`, command 0xC7) drives every pixel to its target state without comparing old/new RAM. This means:
- No need to synchronize old RAM first
- No white flash
- Single visible transition from old content to new content
- Slightly more "flicker" than true partial (Mode 2) but no ghosting

## DC Bias Prevention

E-ink panels accumulate DC bias when pixels are driven in the same direction repeatedly without a full refresh cycle. Over time this causes:
- Permanent ghosting / burn-in
- Reduced contrast
- Pixel degradation

To prevent this, we force a full clear every 10th refresh:
```cpp
if (forceFullRefresh || refreshCount >= 10) {
    EPD_Init();
    EPD_Clear();    // writes 0xFF to both 0x24 and 0x26, then EPD_Update (full)
    refreshCount = 0;
}
```

`EPD_Clear()` does a proper full refresh (0xF7) which applies the complete voltage waveform and resets DC bias.

## Fallback Plan

If `EPD_Display_Fast` (Mode 1) causes visible ghosting or artifacts on this specific panel:

### Option A: Revert to previous approach
Restore the white flash by reverting to:
```cpp
EPD_Full(WHITE);
EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);  // white flash
// draw...
EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);  // content
```

### Option B: Use Mode 2 with explicit old RAM sync
Write white to register 0x26 directly (without displaying it), then use `EPD_Display_Part`:
```cpp
// Tell controller "old state is white" without visible refresh
EPD_Address_Set(0, 0, EPD_W - 1, EPD_H - 1);
EPD_SetCursor(0, 0);
EPD_WR_REG(0x26);
for (int i = 0; i < 15000; i++) EPD_WR_DATA8(0xFF);

// Now partial update works correctly
EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
```
This would give the smoothest partial update (only changed pixels driven) but may ghost if the physical display doesn't actually match the "white" we told the controller about.

### Option C: True partial refresh with EPD_Init_Part
Implement `EPD_Init_Part()` with a proper partial-refresh LUT for the SSD1683. This gives the best of both worlds (no flash, minimal flicker, only changed pixels driven) but requires finding/calibrating the correct LUT values for this specific panel.

## Key Files

- `lib/EPD/EPD.cpp` — hardware register sequences
- `lib/EPD/EPD.h` — function declarations
- `lib/EPD_GUI/EPD_GUI.cpp` — `EPD_Full()` is software-only buffer fill (line 111)
- `src/main_screen.h` — `display_main_screen()` uses the refresh strategy

## Border Waveform (Register 0x3C)

| Value | Context | Effect |
|-------|---------|--------|
| 0x05 | `EPD_Init` / `EPD_Init_Fast` | Border follows LUT (normal) |
| 0x80 | `EPD_Display_Part` | Border HiZ (floating, prevents border flash during partial) |

Note: `EPD_Display_Fast` does NOT set 0x3C to 0x80. If border flashing is observed during fast updates, adding `EPD_WR_REG(0x3C); EPD_WR_DATA8(0x80);` before `EPD_Display_Fast` may help.

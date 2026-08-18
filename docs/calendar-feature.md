# Calendar Events & Fun Fact Feature

*Added in v0.5.0. 5.79" panel only. Layout refined in v0.5.1–v0.5.8.*

## Overview

The right half of the 5.79" panel (previously blank) displays:
- **Calendar events** for today and tomorrow, fetched from an iCloud calendar API
- **A useless fact of the day** (in German), shown below the calendar events when space remains, or as the sole content when there are no events

## Data Sources

### Calendar API

A self-hosted API that proxies iCloud calendar events.

```
GET http://calendar-api:3000/events
Authorization: Bearer <token>
```

Response:
```json
{
  "today": {
    "label": "Today",
    "date": "2026-06-09",
    "events": [
      {
        "summary": "Fiete KiTa Waldtag",
        "start": "2026-06-09T08:00:00",
        "end": "2026-06-09T16:45:00",
        "allDay": false
      }
    ]
  },
  "tomorrow": {
    "label": "Tomorrow",
    "date": "2026-06-10",
    "events": []
  },
  "fetched_at": "2026-06-09T04:06:45.224Z"
}
```

- Typically 0-1 events, max 3 per day
- Each event has `summary`, `start` (ISO 8601), `end`, and `allDay` (boolean)

### Useless Facts API

```
GET https://uselessfacts.jsph.pl/api/v2/facts/today?language=de
```

Returns a different fact each day. On the 5.79" panel, the fact is always fetched (independent of calendar state) to fill remaining space below events.

## Display Layout

The right half starts at x=410 (after the 8px IC gap) and extends to x=784, giving ~374px of usable width. Full 272px height. Content starts at y=20, top-aligned with the large temperature numbers on the left half.

### With Events

Days with events are rendered top-down. Empty days are skipped entirely (no header shown). If only tomorrow has events, it renders at the top -- the "Tomorrow" header makes the day clear.

```
+--------------------------------------+
|  Today                      (32px)   |  <- 18pt Helvetica, bold/larger
|                              +6px    |  <- padding after header
|  08:00  Fiete KiTa Waldtag  (24px)  |  <- 14pt Helvetica
|                                      |
|  Tomorrow                   (32px)   |
|                              +6px    |
|  14:30  Team Standup         (24px)  |
|  All day  Oma Geburtstag     (24px)  |
|                                      |
|                                      |
|  Das Chupa Chups-Logo wurde   (16px) |  <- smaller filler font
|  von Salvador Dali entworfen. (16px) |  <- bottom-aligned, 25px margin
+--------------------------------------+
```

- Day headers: font size 32 (18pt Helvetica), visually distinct from event text
- Header padding: 6px gap between header and first event
- Timed events: `HH:MM  Summary` in font size 24 (14pt Helvetica), time extracted from ISO start field
- All-day events: `All day  Summary` in font size 24 (no time prefix)
- Long summaries are truncated to fit the available width
- Spacing: 30px between event lines, 14px gap between day sections
- Useless fact renders bottom-aligned in remaining space (font size 16, 9x15 pixel font)

### Without Events (Fun Fact Only)

```
+--------------------------------------+
|                                      |
|  Das Chupa Chups-Logo wurde          |  <- 20px font (14pt Helvetica)
|  von Salvador Dali entworfen.        |  <- vertically centered
|                                      |
+--------------------------------------+
```

- Font size 20 (14pt Helvetica) when the fact is the sole content — more prominent
- Word-wrapped to fit the right half width
- Vertically centered on the right half

### Font Size Logic

The fact uses two different font sizes depending on context:
- **Filler** (calendar events present): font size 16, line height 20px — subtle, blends in
- **Sole content** (no events): font size 20, line height 26px — more prominent

## Configuration

Set via the web dashboard (`http://eink-panel.local`):

| Field | Dashboard ID | NVS Namespace | NVS Key |
|-------|-------------|---------------|---------|
| API URL | `cal_url` | `calendar` | `api_url` |
| Bearer Token | `cal_token` | `calendar` | `bearer` |

If the API URL is empty, calendar fetching is skipped entirely (no network calls).

## Fetch Intervals

### Calendar
- Normal: every 10 minutes (`CALENDAR_INTERVAL`)
- On failure: every 1 minute (`RETRY_INTERVAL`)
- Staleness tracking: `calendarFailCount` incremented on each failure, reset to 0 on success
- Also fetched once at boot in `setup()`

### Useless Fact (5.79" panel only)
- Normal: every 4 hours (`FACT_INTERVAL`)
- Fetched once at boot in `setup()`
- Independent of calendar state — always available to fill remaining space
- On the 4.2" panel, the fact is only fetched when there are zero calendar events (legacy behavior)

## Files Modified

| File | Changes |
|------|---------|
| `src/config_manager.h` | Added `calendarApiUrl`, `calendarBearerToken` with NVS load/save |
| `src/main_screen.h` | Added `CalendarEvent`/`CalendarDay` structs, `fetch_calendar_data()`, `fetch_useless_fact()`, right-half rendering in `display_main_screen()` |
| `src/main.ino` | Added calendar fetch to boot sequence and loop with interval/retry. Separate `FACT_INTERVAL` (4h) for 5.79" panel. |
| `src/web_dashboard.h` | Added Calendar config section (API URL + Bearer Token) |

## Panel Scope

- **5.79" panel**: Full feature (fetch + render). Rendering is guarded by `#ifdef PANEL_579`. Fact always fetched on its own 4h cycle.
- **4.2" panel**: Since v0.7.0 the calendar is rendered in the top half by the
  dedicated German layout in `src/screen_42.h` (see [panel-42-layout.md](panel-42-layout.md)).
  The useless fact is no longer fetched or shown on this panel.

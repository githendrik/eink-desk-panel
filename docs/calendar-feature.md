# Calendar Events & Fun Fact Feature

*Added in v0.5.0. 5.79" panel only.*

## Overview

The right half of the 5.79" panel (previously blank) displays either:
- **Calendar events** for today and tomorrow, fetched from an iCloud calendar API
- **A useless fact of the day** (in German), shown when there are no events for either day

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

Returns a different fact each day. Only fetched when the calendar has zero events for both today and tomorrow.

## Display Layout

The right half starts at x=410 (after the 8px IC gap) and extends to x=784, giving ~374px of usable width. Full 272px height.

### With Events

Days with events are rendered top-down. Empty days are skipped entirely (no header shown). If only tomorrow has events, it renders at the top -- the "Tomorrow" header makes the day clear.

```
+--------------------------------------+
|  Today                      (12px)   |
|  08:00  Fiete KiTa Waldtag  (16px)  |
|                                      |
|  Tomorrow                   (12px)   |
|  14:30  Team Standup         (16px)  |
|  All day  Oma Geburtstag     (16px)  |
+--------------------------------------+
```

- Day headers: 12px font, only shown if that day has events
- Timed events: `HH:MM  Summary` in 16px, time extracted from ISO start field
- All-day events: `All day  Summary` in 16px (no time prefix)
- Long summaries are truncated to fit the available width
- Spacing: 24px between event lines, 8px gap between day sections

### Without Events (Fun Fact)

```
+--------------------------------------+
|                                      |
|  Das Chupa Chups-Logo wurde          |
|  von Salvador Dali entworfen.        |
|                                      |
+--------------------------------------+
```

- 16px font, word-wrapped to fit the right half width
- Vertically offset 20px from the top

## Configuration

Set via the web dashboard (`http://eink-panel.local`):

| Field | Dashboard ID | NVS Namespace | NVS Key |
|-------|-------------|---------------|---------|
| API URL | `cal_url` | `calendar` | `api_url` |
| Bearer Token | `cal_token` | `calendar` | `bearer` |

If the API URL is empty, calendar fetching is skipped entirely (no network calls).

## Fetch Interval

- Normal: every 10 minutes (`CALENDAR_INTERVAL`)
- On failure: every 1 minute (`RETRY_INTERVAL`)
- Staleness tracking: `calendarFailCount` incremented on each failure, reset to 0 on success
- Also fetched once at boot in `setup()`

## Files Modified (v0.5.0)

| File | Changes |
|------|---------|
| `src/config_manager.h` | Added `calendarApiUrl`, `calendarBearerToken` with NVS load/save |
| `src/main_screen.h` | Added `CalendarEvent`/`CalendarDay` structs, `fetch_calendar_data()`, `fetch_useless_fact()`, right-half rendering in `display_main_screen()` |
| `src/main.ino` | Added calendar fetch to boot sequence and loop with interval/retry |
| `src/web_dashboard.h` | Added Calendar config section (API URL + Bearer Token) |

## Panel Scope

- **5.79" panel**: Full feature (fetch + render). Rendering is guarded by `#ifdef PANEL_579`.
- **4.2" panel**: Dashboard config fields are visible but data is not rendered. Fetching only occurs if a calendar URL is configured (which it wouldn't be for the 4.2" panel).

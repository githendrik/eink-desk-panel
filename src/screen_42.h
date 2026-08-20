#ifndef SCREEN_42_H
#define SCREEN_42_H

// ---------------------------------------------------------------------------
// 4.2" panel layout (400 x 300) -- German weather forecast + family calendar.
//
// This screen is deliberately different from the 5.79" one: it drops the Aare
// temperature, pollen, UV and weight widgets in favour of a readable multi-hour
// and multi-day forecast, with the shared calendar filling the bottom band.
//
//   y   0..160   calendar: full date header, then today's and tomorrow's events
//   y 166        divider
//   y 183..295   weather: big temperature, condition icon, detail column,
//                then two forecast entries (in 2h, tomorrow)
// ---------------------------------------------------------------------------

#include "EPD_GUI.h"
#include "weather_icons.h"

// --- German labels ---------------------------------------------------------

static const char* WD_LONG[7] = {
  "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag", "Samstag", "Sonntag"
};
static const char* WD_SHORT[7] = { "Mo", "Di", "Mi", "Do", "Fr", "Sa", "So" };
static const char* MONTH_DE[12] = {
  "Januar", "Februar", "M\u00e4rz", "April", "Mai", "Juni",
  "Juli", "August", "September", "Oktober", "November", "Dezember"
};

// Short German description for a WMO weather code.
static const char* wmoTextDe(int code) {
  switch (code) {
    case 0:  return "klar";
    case 1:  return "sonnig";
    case 2:  return "teils bew\u00f6lkt";
    case 3:  return "bedeckt";
    case 45: case 48: return "Nebel";
    case 51: case 53: case 55: return "Nieselregen";
    case 56: case 57: return "gefrierender Niesel";
    case 61: return "leichter Regen";
    case 63: return "Regen";
    case 65: return "starker Regen";
    case 66: case 67: return "gefrierender Regen";
    case 71: return "leichter Schnee";
    case 73: return "Schnee";
    case 75: return "starker Schnee";
    case 77: return "Schneegriesel";
    case 80: return "leichte Schauer";
    case 81: return "Schauer";
    case 82: return "starke Schauer";
    case 85: case 86: return "Schneeschauer";
    case 95: return "Gewitter";
    case 96: case 99: return "Gewitter mit Hagel";
    default: return "";
  }
}

// --- small drawing helpers -------------------------------------------------

static void s42_text(int x, int y, const char* s, int size) {
  EPD_ShowStringUTF8(x, y, s, size, BLACK);
}

static void s42_textRight(int xRight, int y, const char* s, int size) {
  EPD_ShowStringUTF8(xRight - EPD_GetUTF8TextWidth(s, size), y, s, size, BLACK);
}

static void s42_textCenter(int xCenter, int y, const char* s, int size) {
  EPD_ShowStringUTF8(xCenter - EPD_GetUTF8TextWidth(s, size) / 2, y, s, size, BLACK);
}

// Truncate in place until the string fits maxWidth, appending an ellipsis.
static void s42_fit(char* s, int maxWidth, int size) {
  if (EPD_GetUTF8TextWidth(s, size) <= maxWidth) return;
  while (strlen(s) > 1 && EPD_GetUTF8TextWidth(s, size) > maxWidth - 6) {
    s[strlen(s) - 1] = '\0';
  }
  strcat(s, ".");
}

// Horizontal precipitation-probability bar: outline plus proportional fill.
static void s42_popBar(int x, int y, int w, int h, int pop) {
  EPD_DrawRectangle(x, y, x + w, y + h, BLACK, 0);  // outline
  if (pop <= 0) return;
  int fill = (w - 2) * pop / 100;
  if (fill < 1) fill = 1;
  EPD_DrawRectangle(x + 1, y + 1, x + 1 + fill, y + h - 1, BLACK, 1);
}

static void s42_icon(int x, int y, int size, int code, bool isDay) {
  const uint8_t* bmp = (size == 24)
      ? weatherIcons24[weatherIconForCode(code, isDay)]
      : weatherIcons48[weatherIconForCode(code, isDay)];
  // EPD_ShowPicture draws at (x+1, y+1) and treats a 0 bit as ink.
  EPD_ShowPicture(x - 1, y - 1, size, size, bmp, BLACK);
}

// ---------------------------------------------------------------------------

// Render one calendar row. `label` is drawn in the left gutter (may be "").
static void s42_calRow(int y, const char* label, const CalendarEvent& ev,
                       int gutterX, int timeX, int sumX, int sumMaxW) {
  char buf[96];
  if (label && label[0]) s42_text(gutterX, y + 4, label, 12);
  s42_text(timeX, y, ev.allDay ? "ganzt." : ev.startTime.c_str(), 16);
  snprintf(buf, sizeof(buf), "%s", ev.summary.c_str());
  s42_fit(buf, sumMaxW, 16);
  s42_text(sumX, y, buf, 16);
}

void draw_layout_42() {
  char buf[96];

  const int L = 10;    // left margin
  const int R = 390;   // right margin

  // ===== Calendar (top half) ========================================
  // Worst case is 3 events today + 3 tomorrow = 6 rows, which is what the
  // geometry below is sized for. Day labels live in a left gutter so adding
  // the "Morgen" section costs no extra rows.
  {
    const int gutterX = L;        // "Heute" / "Morgen"
    const int timeX   = L + 62;   // "08:00" / "ganzt."
    const int sumX    = L + 120;  // event title
    const int sumMaxW = R - sumX;
    const int pitch   = 20;
    const int perDay  = 3;        // hard cap so the band cannot overflow
    int y = 40;

    // Header: full date, e.g. "Montag, 17. August 2026"
    if (wx.valid) {
      snprintf(buf, sizeof(buf), "%s, %d. %s %d",
               WD_LONG[wx.todayWday % 7], wx.todayDay,
               MONTH_DE[(wx.todayMonth - 1) % 12], wx.todayYear);
      s42_text(L, 2, buf, 32);
    }
    if (getStalenessMessage().length() > 0) {
      s42_textRight(R, 14, "veraltet", 12);
    }

    // Today, skipping events that have already finished
    int shown = 0, hidden = 0;
    for (int i = 0; i < calendarDays[0].eventCount && shown < perDay; i++) {
      const CalendarEvent& ev = calendarDays[0].events[i];
      if (calendarEventIsPast(ev)) { hidden++; continue; }
      s42_calRow(y, shown == 0 ? "Heute" : "", ev, gutterX, timeX, sumX, sumMaxW);
      y += pitch;
      shown++;
    }
    if (shown == 0) {
      s42_text(gutterX, y + 4, "Heute", 12);
      s42_text(timeX, y, hidden > 0 ? "keine Termine mehr" : "keine Termine", 16);
      y += pitch;
    }

    // Tomorrow (only when there is something to show)
    int nTomorrow = calendarDays[1].eventCount;
    if (nTomorrow > perDay) nTomorrow = perDay;
    for (int i = 0; i < nTomorrow; i++) {
      s42_calRow(y, i == 0 ? "Morgen" : "", calendarDays[1].events[i],
                 gutterX, timeX, sumX, sumMaxW);
      y += pitch;
    }
  }

  EPD_DrawLine(L, 166, R, 166, BLACK);

  // ===== Weather (bottom half) ======================================
  if (wx.valid) {
    // Big current temperature. u8g2 anchors on the baseline (y + size), and
    // logisoso62 is 62px tall for a nominal size of 78, so y sits ~16px above
    // the visible top of the digits.
    snprintf(buf, sizeof(buf), "%d", wx.now.temp);
    int tw = EPD_GetUTF8TextWidth(buf, 78);
    s42_text(L + 8, 167, buf, 78);
    s42_text(L + 8 + tw + 4, 175, "o", 24);

    // Condition icon
    s42_icon(152, 187, 48, wx.now.code, wx.now.isDay);

    // Detail column
    const int dx = 228;
    int dy = 181;
    const int pitch = 22;

    snprintf(buf, sizeof(buf), "%d\u00b0/%d\u00b0", wx.days[0].tmin, wx.days[0].tmax);
    s42_text(dx, dy, "Heute", 16);  s42_textRight(R, dy, buf, 16);  dy += pitch;

    snprintf(buf, sizeof(buf), "%d km/h", wx.now.wind);
    s42_text(dx, dy, "Wind", 16);   s42_textRight(R, dy, buf, 16);  dy += pitch;

    snprintf(buf, sizeof(buf), "%d%%", wx.days[0].pop);
    s42_text(dx, dy, "Regen", 16);  s42_textRight(R, dy, buf, 16);

    // ===== Two forecast entries: in 2h, and tomorrow ================
    const int halfX = 206;

    if (wx.hourCount > 2) {
      const WxHour& h = wx.hours[2];
      snprintf(buf, sizeof(buf), "In 2h:  %d\u00b0", h.temp);
      s42_text(L + 4, 249, buf, 16);
      s42_icon(L + 4, 271, 24, h.code, h.hour >= 7 && h.hour < 21);
      s42_text(L + 36, 275, wmoTextDe(h.code), 16);
    }

    if (wx.dayCount > 1) {
      const WxDay& d = wx.days[1];
      snprintf(buf, sizeof(buf), "Morgen:  %d\u00b0", d.tmax);
      s42_text(halfX, 249, buf, 16);
      s42_icon(halfX, 271, 24, d.code, true);
      s42_text(halfX + 32, 275, wmoTextDe(d.code), 16);
    }
  }
}

#endif  // SCREEN_42_H

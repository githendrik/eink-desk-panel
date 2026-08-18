#ifndef SIM_DATA_STUB_H
#define SIM_DATA_STUB_H
// Sample data for the host-side preview (sim/sim42.cpp). Mirrors the shape of
// the real globals in main_screen.h. Not compiled into the firmware.

#define WX_HOURS 12
#define WX_DAYS  3

struct WxNow  { int temp, feels, wind, humidity, code; bool isDay; };
struct WxHour { int hour, temp, pop, code; };
struct WxDay  { int tmin, tmax, pop, code, wday; };
struct WeatherData {
  WxNow now; WxHour hours[WX_HOURS]; int hourCount;
  WxDay days[WX_DAYS]; int dayCount;
  String sunrise, sunset;
  int todayYear, todayDay, todayMonth, todayWday;
  bool valid;
};

struct CalendarEvent { String summary, startTime; bool allDay; };
struct CalendarDay   { String label; CalendarEvent events[5]; int eventCount; };

static WeatherData wx = {
  /* now      */ { 26, 27, 7, 47, 3, true },
  /* hours    */ {{21,25,18,3},{22,23,30,61},{23,22,20,61},{0,22,10,3},
                  {1,21,3,3},{2,20,5,2},{3,19,0,2},{4,19,0,0},
                  {5,18,0,0},{6,18,8,1},{7,19,28,3},{8,20,45,61}},
  /* hourCount*/ 12,
  /* days     */ {{18,31,30,3,6},{18,23,100,63,0},{17,27,3,1,1}},
  /* dayCount */ 3,
  "06:28", "20:40",
  2026, 17, 8, 0,
  true
};

static CalendarDay calendarDays[2] = {
  { "Today",    {{"Termin ganzer Tag","",true},
                 {"Sp\u00fclung Abwasserleitung","08:00",false},
                 {"Elterncaf\u00e9 Plus!","18:30",false}}, 3 },
  { "Tomorrow", {{"Turnen Kinder","10:00",false}}, 1 }
};
static int calendarTotalEvents = 4;

static String getStalenessMessage() { return String(""); }

#endif

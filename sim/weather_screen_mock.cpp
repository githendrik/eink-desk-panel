#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <map>
#include <vector>

// Mock Arduino types
typedef bool boolean;
typedef uint8_t byte;

#define PROGMEM
#define _BV(bit) (1 << (bit))

// Mock String class
class String {
public:
    std::string str;
    
    String() : str("") {}
    String(const char* s) : str(s ? s : "") {}
    String(std::string s) : str(s) {}
    String(int v) : str(std::to_string(v)) {}
    String(double v) : str(std::to_string(v)) {}
    
    const char* c_str() const { return str.c_str(); }
    int length() const { return str.length(); }
    String substring(int start, int end) const { return str.substr(start, end - start); }
    String toLowerCase() const { 
        std::string result = str;
        for (char& c : result) c = tolower(c);
        return result;
    }
    
    String& operator=(const String& other) { str = other.str; return *this; }
    String& operator+=(const String& other) { str += other.str; return *this; }
    String operator+(const String& other) const { return String(str + other.str); }
    bool operator==(const String& other) const { return str == other.str; }
    bool operator!=(const String& other) const { return str != other.str; }
    
    int toInt() const { return std::atoi(str.c_str()); }
    double toDouble() const { return std::atof(str.c_str()); }
    
    void toCharArray(char* buf, int size) const {
        strncpy(buf, str.c_str(), size - 1);
        buf[size - 1] = '\0';
    }
};

String operator+(const char* a, const String& b) { return String(a) + b; }
String operator+(const String& a, const char* b) { return a + String(b); }

// Mock WiFi
class WiFiClass {
public:
    bool connected = true;
    int status() { return connected ? 3 : 0; }
    void begin(const char* ssid, const char* pass) { connected = true; }
    String localIP() { return "192.168.1.100"; }
};

WiFiClass WiFi;

// Mock HTTP
class HTTPClient {
public:
    int responseCode = 200;
    String payload;
    
    void begin(String url) {}
    int GET() { return responseCode; }
    String getString() { return payload; }
    void end() {}
};

// Mock Serial
class SerialClass {
public:
    void begin(int baud) {}
    void println(const String& s) { std::cout << s.c_str() << std::endl; }
    void println(int v) { std::cout << v << std::endl; }
    void print(const String& s) { std::cout << s.c_str(); }
    void print(int v) { std::cout << v; }
};

SerialClass Serial;

// Mock millis
unsigned long mock_millis_base = 0;
unsigned long millis() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (unsigned long)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000) - mock_millis_base;
}

// Mock WiFiClient
class WiFiClient {
public:
    bool connected() { return true; }
};

// Include the mock EPD
#include "epd_mock.h"

// Global mock instance
EPDMock* g_epd = new EPDMock();

// Mock JSON
namespace Arduino_JSON {
    class JSONVar {
    public:
        std::map<std::string, JSONVar> children;
        std::string value;
        bool isUndefined = false;
        
        JSONVar() : isUndefined(false) {}
        
        JSONVar operator[](const std::string& key) {
            if (children.find(key) == children.end()) {
                JSONVar undef;
                undef.isUndefined = true;
                return undef;
            }
            return children[key];
        }
        
        JSONVar operator[](int index) {
            std::string key = std::to_string(index);
            if (children.find(key) == children.end()) {
                JSONVar undef;
                undef.isUndefined = true;
                return undef;
            }
            return children[key];
        }
        
        operator int() const { return std::atoi(value.c_str()); }
        operator double() const { return std::atof(value.c_str()); }
        operator String() const { return String(value); }
        
        static String typeof(const JSONVar& v) {
            if (v.isUndefined) return "undefined";
            return "object";
        }
        
        static JSONVar parse(const String& json) {
            // Very simple mock parser - just for testing
            JSONVar result;
            result.isUndefined = false;
            
            result.children["main"] = JSONVar();
            result.children["main"].children["temp"] = JSONVar();
            result.children["main"].children["temp"].value = "23.5";
            
            result.children["list"] = JSONVar();
            result.children["list"].children["0"] = JSONVar();
            result.children["list"].children["0"].children["components"] = JSONVar();
            result.children["list"].children["0"].children["components"].children["pm2_5"] = JSONVar();
            result.children["list"].children["0"].children["components"].children["pm2_5"].value = "25";
            
            return result;
        }
    };
    
    String stringify(const JSONVar& v) {
        return String(v.value);
    }
}

using namespace Arduino_JSON;

// Mock TimeLib
namespace TimeLib {
    int year(time_t t) { return 2024; }
    int month(time_t t) { return 1; }
    int day(time_t t) { return 15; }
    int hour(time_t t) { return 14; }
    int minute(time_t t) { return 30; }
    int second(time_t t) { return 0; }
}

using namespace TimeLib;

// Mock credentials
#define WIFI_SSID "test"
#define WIFI_PASSWORD "test"
#define OPENWEATHER_API_KEY "test"

// Weather icon arrays (mock)
const unsigned char* Weather_Num[6] = {nullptr};

// Include weather screen logic (we'll inline it here)
String weather = "Clear";
String temperature = "23";
String temperatureMin = "18";
String temperatureMax = "26";
String city_js = "Bern";
String aareTemp = "15";
String aareText = "swimming recommended";
String aareTime = "2024-01-15 14:30:00";
String pollenLevel = "moderate";

int getWeatherIcon(String weatherCondition) {
    String cond = weatherCondition.toLowerCase();
    if (cond.str.find("mist") != std::string::npos || cond.str.find("fog") != std::string::npos) {
        return 0;
    } else if (cond.str.find("cloud") != std::string::npos) {
        return 1;
    } else if (cond.str.find("thunder") != std::string::npos) {
        return 2;
    } else if (cond.str.find("clear") != std::string::npos) {
        return 3;
    } else if (cond.str.find("snow") != std::string::npos) {
        return 4;
    } else if (cond.str.find("rain") != std::string::npos || cond.str.find("drizzle") != std::string::npos) {
        return 5;
    }
    return 3;
}

void fetch_weather_data(int& httpResponseCode) {
    httpResponseCode = 200;
    temperature = "23";
    aareTemp = "15";
    pollenLevel = "moderate";
}

void display_weather_screen(uint8_t* ImageBW, bool& forceFullRefresh) {
    static char buffer[64];

    if (forceFullRefresh) {
        EPD_Init();
        EPD_Clear();
        forceFullRefresh = false;
    }

    EPD_Init_Fast(1);
    
    Paint_NewImage(ImageBW, EPD_W, EPD_H, 0, WHITE);
    EPD_Full(WHITE);
    EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "%s°", temperature.c_str());
    EPD_ShowStringUTF8(120, 80, buffer, 72, BLACK);

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Pollen %s", pollenLevel.c_str());
    EPD_ShowStringUTF8(120, 170, buffer, 24, BLACK);

    memset(buffer, 0, sizeof(buffer));
    snprintf(buffer, sizeof(buffer), "Aare %s°", aareTemp.c_str());
    EPD_ShowStringUTF8(120, 220, buffer, 24, BLACK);

    EPD_Display_Part(0, 0, EPD_W, EPD_H, ImageBW);
}

// Export display data to JSON for Python renderer
void exportDisplayData(const char* filename) {
    std::ofstream out(filename);
    
    out << "{\n";
    out << "  \"textElements\": [\n";
    
    bool first = true;
    for (const auto& elem : g_epd->textElements) {
        if (!first) out << ",\n";
        first = false;
        out << "    {\"x\": " << elem.x 
            << ", \"y\": " << elem.y
            << ", \"text\": \"" << elem.text << "\""
            << ", \"fontSize\": " << elem.fontSize
            << ", \"color\": " << (elem.color == BLACK ? 1 : 0) << "}";
    }
    
    out << "\n  ],\n";
    out << "  \"lineElements\": [\n";
    
    first = true;
    for (const auto& elem : g_epd->lineElements) {
        if (!first) out << ",\n";
        first = false;
        out << "    {\"x1\": " << elem.x1 
            << ", \"y1\": " << elem.y1
            << ", \"x2\": " << elem.x2
            << ", \"y2\": " << elem.y2
            << ", \"color\": " << (elem.color == BLACK ? 1 : 0) << "}";
    }
    
    out << "\n  ],\n";
    out << "  \"pictureElements\": [\n";
    
    first = true;
    for (const auto& elem : g_epd->pictureElements) {
        if (!first) out << ",\n";
        first = false;
        out << "    {\"x\": " << elem.x 
            << ", \"y\": " << elem.y
            << ", \"w\": " << elem.w
            << ", \"h\": " << elem.h
            << ", \"iconIndex\": " << elem.iconIndex << "}";
    }
    
    out << "\n  ]\n";
    out << "}\n";
    
    out.close();
    std::cout << "Exported display data to: " << filename << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "E-Ink Desk Panel Simulator" << std::endl;
    std::cout << "==========================" << std::endl;
    
    uint8_t ImageBW[15000];
    bool forceFullRefresh = true;
    int httpResponseCode;
    
    std::cout << "Fetching weather data..." << std::endl;
    fetch_weather_data(httpResponseCode);
    
    std::cout << "Rendering display..." << std::endl;
    display_weather_screen(ImageBW, forceFullRefresh);
    
    const char* outputFile = "display_data.json";
    if (argc > 1) {
        outputFile = argv[1];
    }
    
    exportDisplayData(outputFile);
    
    std::cout << "\nNow run: python3 sim/render.py " << outputFile << " output.png" << std::endl;
    std::cout << "Then open output.png to see your display preview" << std::endl;
    
    return 0;
}

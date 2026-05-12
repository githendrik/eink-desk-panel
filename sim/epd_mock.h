#ifndef EPD_MOCK_H
#define EPD_MOCK_H

#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

#define EPD_W 400
#define EPD_H 300

enum Color {
    WHITE = 0,
    BLACK = 1
};

enum FontSize {
    FONT_8 = 8,
    FONT_12 = 12,
    FONT_16 = 16,
    FONT_24 = 24,
    FONT_32 = 32,
    FONT_48 = 48
};

class EPDMock {
public:
    uint8_t* buffer;
    int width;
    int height;
    
    struct TextElement {
        int x, y;
        std::string text;
        int fontSize;
        Color color;
    };
    
    struct LineElement {
        int x1, y1, x2, y2;
        Color color;
    };
    
    struct RectElement {
        int x, y, w, h;
        Color color;
        bool filled;
    };
    
    struct PictureElement {
        int x, y, w, h;
        int iconIndex;
        std::string label;
    };
    
    std::vector<TextElement> textElements;
    std::vector<LineElement> lineElements;
    std::vector<RectElement> rectElements;
    std::vector<PictureElement> pictureElements;
    
    EPDMock() : width(EPD_W), height(EPD_H) {
        buffer = new uint8_t[width * height / 8];
        memset(buffer, 0xFF, width * height / 8);
    }
    
    ~EPDMock() {
        delete[] buffer;
    }
    
    void clear() {
        memset(buffer, 0xFF, width * height / 8);
        textElements.clear();
        lineElements.clear();
        rectElements.clear();
        pictureElements.clear();
    }
    
    void showStringUTF8(int x, int y, const char* text, int fontSize, Color color) {
        TextElement elem;
        elem.x = x;
        elem.y = y;
        elem.text = std::string(text);
        elem.fontSize = fontSize;
        elem.color = color;
        textElements.push_back(elem);
    }
    
    void drawLine(int x1, int y1, int x2, int y2, Color color) {
        LineElement elem;
        elem.x1 = x1;
        elem.y1 = y1;
        elem.x2 = x2;
        elem.y2 = y2;
        elem.color = color;
        lineElements.push_back(elem);
    }
    
    void drawRect(int x, int y, int w, int h, Color color, bool filled = false) {
        RectElement elem;
        elem.x = x;
        elem.y = y;
        elem.w = w;
        elem.h = h;
        elem.color = color;
        elem.filled = filled;
        rectElements.push_back(elem);
    }
    
    void showPicture(int x, int y, int w, int h, int iconIndex, Color color) {
        PictureElement elem;
        elem.x = x;
        elem.y = y;
        elem.w = w;
        elem.h = h;
        elem.iconIndex = iconIndex;
        elem.label = "";
        pictureElements.push_back(elem);
    }
    
    int getTextWidth(const char* text, int fontSize) {
        // Approximate text width based on font size
        std::string str(text);
        return str.length() * fontSize * 0.6;
    }
};

// Global mock instance
extern EPDMock* g_epd;

void EPD_GPIOInit() {}
void EPD_Init() { if (g_epd) g_epd->clear(); }
void EPD_Init_Fast(int mode) { if (g_epd) g_epd->clear(); }
void EPD_Clear() { if (g_epd) g_epd->clear(); }
void EPD_Full(Color color) {}
void EPD_Display_Part(int x, int y, int w, int h, uint8_t* image) {}

void EPD_ShowStringUTF8(int x, int y, const char* text, int fontSize, Color color) {
    if (g_epd) g_epd->showStringUTF8(x, y, text, fontSize, color);
}

void EPD_DrawLine(int x1, int y1, int x2, int y2, Color color) {
    if (g_epd) g_epd->drawLine(x1, y1, x2, y2, color);
}

void EPD_DrawRect(int x, int y, int w, int h, Color color, bool filled = false) {
    if (g_epd) g_epd->drawRect(x, y, w, h, color, filled);
}

void EPD_ShowPicture(int x, int y, int w, int h, int iconIndex, Color color) {
    if (g_epd) g_epd->showPicture(x, y, w, h, iconIndex, color);
}

int EPD_GetUTF8TextWidth(const char* text, int fontSize) {
    if (g_epd) return g_epd->getTextWidth(text, fontSize);
    return 0;
}

void Paint_NewImage(uint8_t* image, int w, int h, int rotate, Color color) {
    // Mock: just clear the image buffer
    memset(image, 0xFF, w * h / 8);
}

#endif

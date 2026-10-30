#pragma once

#include <cstdint>
#include <string>

struct FontHandle {
    void *ptr{nullptr};
};

struct RendererDisplayRect {
    int16_t x;
    int16_t y;
    int16_t width;
    int16_t height;
};

class RendererDisplay {
public:
    virtual ~RendererDisplay() = default;

    // Dimensions and colors
    virtual int16_t width() const = 0;
    virtual int16_t height() const = 0;

    // Drawing primitives
    virtual void fillScreen(uint16_t color) = 0;
    virtual void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) = 0;
    virtual void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h,
                             uint16_t color) = 0;

    // Text operations
    virtual uint16_t lineHeight(FontHandle font) = 0;
    virtual uint16_t getTextWidth(const char *cstr, FontHandle font) = 0;
    virtual uint16_t getTextWidthQuick(const char *cstr, FontHandle font) {
        return getTextWidth(cstr, font);
    }
    virtual void getTextSize(const char *cstr, uint16_t *outWidth, uint16_t *outHeight,
                             FontHandle font) = 0;

    // Takes a single char and returns a null-terminated const char * with a replacement sequence
    // Note that the returned const char * should be long-lived
    virtual auto getSuperscriptCharacter(const char inputChar, FontHandle font) const -> const
        char * {
        return nullptr;
    };
    virtual auto getSubscriptCharacter(const char inputChar, FontHandle font) const -> const
        char * {
        return nullptr;
    };

    // For fonts that can set their size
    virtual inline void setSupSubFontSize(FontHandle font) {}
    virtual inline void setNormalFontSize(FontHandle font) {}
    virtual inline void setItalicFont(FontHandle font, bool italic) {}

    // Returns the x position after drawing the text (advance)
    virtual int16_t drawSingleLineOfText(const char *cstr, int16_t x, int16_t y, bool invert,
                                         FontHandle font) = 0;

    virtual inline uint32_t getForegroundColor() const { return 0x000000; }
    virtual inline uint32_t getBackgroundColor() const { return 0xFFFFFF; }
};

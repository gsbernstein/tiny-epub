#ifndef RENDERER_H
#define RENDERER_H

#include "../Displays/DisplaySystem.hpp"
#include "../Optics/OpticsSystem.hpp"

class Renderer {
public:
    Renderer(OpticsSystem *opticInput, DisplaySystem *displayInput);

    void renderMaxBox();
    void renderMaxBoxWithText(const String &text);

    void renderMaxBoxWithWord(const String &word);

    void renderBox(int xStart, int yStart, int xEnd, int yEnd);

    auto getPreviousPlace() -> unsigned long;

    auto getDisplay() -> DisplaySystem * { return display_; }

    auto getPixelWidth() -> int { return maxPixelsX_; }
    auto getPixelHeight() -> int { return maxPixelsY_; }

    int localXOffset = 0;
    int localYOffset = 0;

protected:
    OpticsSystem *optic_;
    DisplaySystem *display_;

    void computeMinMax();
    void computeMaxLines();

    int xMin_ = 0;
    int xMax_ = 0;
    int yMin_ = 0;
    int yMax_ = 0;
    int maxPixelsX_ = 0;
    int maxPixelsY_ = 0;

    int maxLines_ = 0;
};

#endif
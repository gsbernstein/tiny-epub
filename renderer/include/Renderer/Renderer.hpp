#ifndef RENDERER_H
#define RENDERER_H

#include "Displays/DisplaySystem.hpp"

#define MAX_CHARS_PER_PAGE 1500
#define MAX_CHARS_PER_LINE 200

typedef std::array<char, MAX_CHARS_PER_PAGE> PageBuffer;
typedef std::array<char, MAX_CHARS_PER_LINE> LineBuffer;

class Renderer {
public:
    Renderer(DisplaySystem &display);

    // TODO: Could be replaced with a stream, or a stream-like class
    virtual auto available() -> int = 0;
    virtual auto read() -> int = 0;
    virtual auto seek(uint32_t pos) -> bool = 0;
    virtual auto position() -> size_t = 0;
    virtual auto size() -> size_t = 0;

    auto getPreviousPlace() -> unsigned long { return previousPlace_; }
    auto getCurrentPlace() -> unsigned long { return currentPlace_; }
    auto getDisplay() -> DisplaySystem * { return &display_; }

    // Render a page of text with the current place
    // returns the next place for reading so the calling function can
    // track place.
    auto renderPage(unsigned long placeInFile) -> unsigned long;

protected:
    DisplaySystem &display_;

    // This should be called by subclasses after content is loaded
    void loaded();

    unsigned long currentPlace_ = 0;
    unsigned long previousPlace_ = 0;
    unsigned long nextPlace_ = 0;

    auto loadPrevIntoBuffer() -> unsigned long;
    auto loadCurrIntoBuffer() -> unsigned long;
    auto loadNextIntoBuffer() -> unsigned long;
    auto goBackwardsOnePage(unsigned long place) -> unsigned long;

    PageBuffer prevBuffer_{};
    PageBuffer currBuffer_{};
    PageBuffer nextBuffer_{};

    // This does the work of lineify from the old system. Loads one page of
    // text into that buffer given the current offset in the file
    auto loadIntoBuffer(PageBuffer &buffer, unsigned long place) -> unsigned long;
};

#endif

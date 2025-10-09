#pragma once

#include <Spiram/String.hpp>
#include <Spiram/Vector.hpp>
#include <string>

struct EPubTocChapter {
    SpiramString title;
    SpiramString href;
    uint32_t epubOffset{};
    uint32_t endEpubOffset{};
    SpiramString inSubsectionHref;
};

using EPubTocChapterVector = SpiramVector<EPubTocChapter>;

#include <EPubFile/EPubFile.hpp>
#include <ImageTest/ImageTest.hpp>
#include <Renderer/Renderer.hpp>
#include <Renderer/RendererDisplay.hpp>
#include <SDL.h>
#include <SDL_ttf.h>
#include <filesystem>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "Catch2/catch_amalgamated.hpp"

class MockDisplay : public RendererDisplay {
public:
    MockDisplay(int16_t w, int16_t h, const char *fontPath, int ptSize) : w_(w), h_(h) {
        // Headless-friendly init
        if (SDL_WasInit(SDL_INIT_VIDEO) == 0) {
            SDL_SetHint(SDL_HINT_RENDER_DRIVER, "software");
            SDL_SetHint(SDL_HINT_VIDEODRIVER, "dummy");
            SDL_Init(SDL_INIT_VIDEO);
        }
        if (TTF_WasInit() == 0) TTF_Init();

        surface_ = SDL_CreateRGBSurfaceWithFormat(0, w_, h_, 32, SDL_PIXELFORMAT_RGBA32);
        REQUIRE(surface_ != nullptr);
        renderer_ = SDL_CreateSoftwareRenderer(surface_);
        REQUIRE(renderer_ != nullptr);
        ttfFont_ = TTF_OpenFont(fontPath, ptSize);
        REQUIRE(ttfFont_ != nullptr);
    }

    // Use RGB565 for foreground/background colors in tests
    inline uint32_t getForegroundColor() const override { return 0x0000; } // black in 565
    inline uint32_t getBackgroundColor() const override { return 0xFFFF; } // white in 565

    static inline void rgb565ToRgb888(uint16_t c, uint8_t &r, uint8_t &g, uint8_t &b) {
        uint8_t r5 = static_cast<uint8_t>((c >> 11) & 0x1F);
        uint8_t g6 = static_cast<uint8_t>((c >> 5) & 0x3F);
        uint8_t b5 = static_cast<uint8_t>(c & 0x1F);
        r = static_cast<uint8_t>((r5 << 3) | (r5 >> 2));
        g = static_cast<uint8_t>((g6 << 2) | (g6 >> 4));
        b = static_cast<uint8_t>((b5 << 3) | (b5 >> 2));
    }

    ~MockDisplay() override {
        if (ttfFont_) {
            TTF_CloseFont(ttfFont_);
        }
        if (renderer_) {
            SDL_DestroyRenderer(renderer_);
        }
        if (surface_) {
            SDL_FreeSurface(surface_);
        }
        if (TTF_WasInit()) {
            TTF_Quit();
        }
        if (SDL_WasInit(SDL_INIT_VIDEO)) {
            SDL_QuitSubSystem(SDL_INIT_VIDEO);
        }
    }

    int16_t width() const override { return w_; }
    int16_t height() const override { return h_; }
    void *defaultFont() const { return ttfFont_; }

    void fillScreen(uint16_t color) override {
        uint8_t r, g, b;
        rgb565ToRgb888(color, r, g, b);
        SDL_SetRenderDrawColor(renderer_, r, g, b, 0xFF);
        SDL_RenderClear(renderer_);
    }

    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) override {
        uint8_t r, g, b;
        rgb565ToRgb888(color, r, g, b);
        SDL_SetRenderDrawColor(renderer_, r, g, b, 0xFF);
        SDL_RenderDrawLine(renderer_, x0, y0, x1, y1);
    }

    void drawXBitmap(int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h,
                     uint16_t color) override {
        // Interpret input as packed 1bpp XBM-style (LSB-first) mask
        SDL_Surface *bm = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
        REQUIRE(bm != nullptr);
        uint8_t r, g, b;
        rgb565ToRgb888(color, r, g, b);
        const int bytesPerLine = (w + 7) / 8;
        for (int yy = 0; yy < h; ++yy) {
            uint8_t *row = static_cast<uint8_t *>(bm->pixels) + yy * bm->pitch;
            const uint8_t *src = bitmap + yy * bytesPerLine;
            for (int xx = 0; xx < w; ++xx) {
                const int byteIdx = xx / 8;
                const int bitIdx = xx % 8; // LSB-first
                const uint8_t bit = (src[byteIdx] >> bitIdx) & 0x01;
                uint8_t *px = row + xx * 4; // RGBA
                if (bit) {
                    px[0] = r;
                    px[1] = g;
                    px[2] = b;
                    px[3] = 0xFF;
                } else {
                    px[0] = 0x00;
                    px[1] = 0x00;
                    px[2] = 0x00;
                    px[3] = 0x00;
                }
            }
        }
        SDL_Texture *tx = SDL_CreateTextureFromSurface(renderer_, bm);
        SDL_FreeSurface(bm);
        SDL_Rect dst{x, y, (int)w, (int)h};
        SDL_RenderCopy(renderer_, tx, nullptr, &dst);
        SDL_DestroyTexture(tx);
    }

    uint16_t lineHeight(FontHandle font) override {
        TTF_Font *ft = static_cast<TTF_Font *>(font.ptr);
        return static_cast<uint16_t>(TTF_FontHeight(ft));
    }
    uint16_t getTextWidth(const char *cstr, FontHandle font) override {
        TTF_Font *ft = static_cast<TTF_Font *>(font.ptr);
        int w = 0, h = 0;
        TTF_SizeUTF8(ft, cstr, &w, &h);
        return static_cast<uint16_t>(w);
    }
    uint16_t getTextWidthQuick(const char *cstr, FontHandle font) override {
        return getTextWidth(cstr, font);
    }
    void getTextSize(const char *cstr, uint16_t *outWidth, uint16_t *outHeight,
                     FontHandle font) override {
        TTF_Font *ft = static_cast<TTF_Font *>(font.ptr);
        int w = 0, h = 0;
        TTF_SizeUTF8(ft, cstr, &w, &h);
        *outWidth = static_cast<uint16_t>(w);
        *outHeight = static_cast<uint16_t>(h);
    }

    int16_t drawSingleLineOfText(const char *cstr, int16_t x, int16_t y, bool,
                                 FontHandle font) override {
        TTF_Font *ft = static_cast<TTF_Font *>(font.ptr);
        uint8_t r, g, b;
        rgb565ToRgb888(static_cast<uint16_t>(getForegroundColor() & 0xFFFF), r, g, b);
        SDL_Color fgColor{(Uint8)r, (Uint8)g, (Uint8)b, 0xFF};
        SDL_Surface *text = TTF_RenderUTF8_Blended(ft, cstr, fgColor);
        REQUIRE(text != nullptr);
        SDL_Texture *tx = SDL_CreateTextureFromSurface(renderer_, text);
        SDL_Rect dst{x, y, text->w, text->h};
        SDL_FreeSurface(text);
        SDL_RenderCopy(renderer_, tx, nullptr, &dst);
        SDL_DestroyTexture(tx);
        return static_cast<int16_t>(x + dst.w);
    }

    void present() { SDL_RenderPresent(renderer_); }
    bool savePNG(const char *path) {
        // Convert surface to raw RGBA
        SDL_Surface *conv = SDL_ConvertSurfaceFormat(surface_, SDL_PIXELFORMAT_RGBA32, 0);
        if (!conv) return false;
        // Composite any transparency onto white
        for (int y = 0; y < conv->h; ++y) {
            uint8_t *row = static_cast<uint8_t *>(conv->pixels) + y * conv->pitch;
            for (int x = 0; x < conv->w; ++x) {
                uint8_t *px = row + x * 4; // RGBA byte order
                uint8_t r = px[0], g = px[1], b = px[2], a = px[3];
                if (a != 255) {
                    // out = a*src + (1-a)*white
                    px[0] = static_cast<uint8_t>(255 - ((255 - r) * a) / 255);
                    px[1] = static_cast<uint8_t>(255 - ((255 - g) * a) / 255);
                    px[2] = static_cast<uint8_t>(255 - ((255 - b) * a) / 255);
                } else {
                    // no-op
                }
                px[3] = 255;
            }
        }
        // Convert to 8bpp grayscale and save
        std::vector<uint8_t> gray;
        RGBAtoGray(static_cast<const uint8_t *>(conv->pixels), conv->w, conv->h, conv->pitch, gray);
        bool ok = Save8bpp(path, conv->w, conv->h, gray.data());
        SDL_FreeSurface(conv);
        return ok;
    }

private:
    int16_t w_;
    int16_t h_;
    SDL_Surface *surface_{};
    SDL_Renderer *renderer_{};
    TTF_Font *ttfFont_{};
};

TEST_CASE("Renderer renders an EPUB first page", "[renderer][epub]") {
    const std::string fixturesDir = std::string(TESTS_DIR) + "/Fixtures/epub";
    const std::string sampleEpub = fixturesDir + "/LastQuestion.epub";

    auto epub =
        std::make_shared<EPubFile>(std::make_shared<PosixUnzipperFileStream>(sampleEpub.c_str()));
    REQUIRE(epub->isOpen());
    auto stream = std::make_shared<EPubFileRendererStream>(epub);

    // Use a system font or a test font path; adjust path as needed
    const std::string fontPath = std::string(TESTS_DIR) + "/Fixtures/NotoSans-Light.ttf";
    MockDisplay display(600, 800, fontPath.c_str(), 16);
    RendererDisplayRect rect{0, 0, 600, 800};
    FontHandle fontHandle{display.defaultFont()};
    Renderer renderer(display, rect, fontHandle);
    RendererDisplayRect inset{30, 30, static_cast<int16_t>(rect.width - 60),
                              static_cast<int16_t>(rect.height - 60)};
    renderer.setRect(inset);
    renderer.setStream(stream);

    const std::string imagesDir = std::string(TESTS_DIR) + "/Images";
    std::error_code ec;
    std::filesystem::create_directories(imagesDir, ec);

    const char *accept = std::getenv("ACCEPT_IMAGE_TESTS");

    auto [nextPlace, darkPercent] = renderer.renderFirstPage();
    (void)darkPercent;
    display.present();
    std::string name0 = std::string("last_question_") + std::to_string(0) + ".png";
    REQUIRE(display.savePNG(name0.c_str()));

    // Compare or seed golden
    {
        int curW = 0, curH = 0;
        std::vector<uint8_t> grayCur;
        REQUIRE(Load8bpp(name0.c_str(), curW, curH, grayCur));
        std::string goldenPath = imagesDir + "/" + name0;
        int gw = 0, gh = 0;
        std::vector<uint8_t> grayGold;
        if (accept || !Load8bpp(goldenPath.c_str(), gw, gh, grayGold)) {
            REQUIRE(Save8bpp(goldenPath.c_str(), curW, curH, grayCur.data()));
        } else {
            REQUIRE(gw == curW);
            REQUIRE(gh == curH);
            REQUIRE(Compare8bpp(grayCur.data(), grayGold.data(), grayGold.size()));
        }
    }
    display.fillScreen(display.getBackgroundColor());

    // Render all remaining pages until EOF
    for (int i = 1; !renderer.atEndOfFile() && i < 10000; ++i) {
        std::tie(nextPlace, std::ignore) = renderer.renderPage(nextPlace);
        display.present();
        std::string name = std::string("last_question_") + std::to_string(i) + ".png";
        REQUIRE(display.savePNG(name.c_str()));

        int curW = 0, curH = 0;
        std::vector<uint8_t> grayCur;
        REQUIRE(Load8bpp(name.c_str(), curW, curH, grayCur));
        std::string goldenPath = imagesDir + "/" + name;
        int gw = 0, gh = 0;
        std::vector<uint8_t> grayGold;
        if (accept || !Load8bpp(goldenPath.c_str(), gw, gh, grayGold)) {
            REQUIRE(Save8bpp(goldenPath.c_str(), curW, curH, grayCur.data()));
        } else {
            REQUIRE(gw == curW);
            REQUIRE(gh == curH);
            REQUIRE(Compare8bpp(grayCur.data(), grayGold.data(), grayGold.size()));
        }

        // Clear between frames so subsequent pages don't overlay
        display.fillScreen(display.getBackgroundColor());
    }

    REQUIRE(renderer.getNumRenderedLines() > 0);
}

TEST_CASE("Renderer renders from TXT file", "[renderer][txt][file]") {
    const std::string fixturesDir = std::string(TESTS_DIR) + "/Fixtures/txt";
    const std::string sampleTxt = fixturesDir + "/lorem.txt";

    // Load entire file into a string
    std::ifstream ifs(sampleTxt);
    REQUIRE(ifs.good());
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());

    const std::string fontPath = std::string(TESTS_DIR) + "/Fixtures/NotoSans-Light.ttf";
    MockDisplay display(600, 800, fontPath.c_str(), 16);
    RendererDisplayRect rect{0, 0, 600, 800};
    FontHandle fontHandle{display.defaultFont()};
    Renderer renderer(display, rect, fontHandle);
    RendererDisplayRect inset{30, 30, static_cast<int16_t>(rect.width - 60),
                              static_cast<int16_t>(rect.height - 60)};
    renderer.setRect(inset);

    // Create a TXT stream
    auto stream = std::make_shared<StdStringStream>(content, DocType::TXT);
    renderer.setStream(stream);

    const std::string imagesDir = std::string(TESTS_DIR) + "/Images";
    std::error_code ec;
    std::filesystem::create_directories(imagesDir, ec);
    const char *accept = std::getenv("ACCEPT_IMAGE_TESTS");

    auto [nextPlace, darkPercent] = renderer.renderFirstPage();
    (void)darkPercent;
    display.present();
    std::string name0 = std::string("txtfile_") + std::to_string(0) + ".png";
    REQUIRE(display.savePNG(name0.c_str()));
    {
        int curW = 0, curH = 0;
        std::vector<uint8_t> grayCur;
        REQUIRE(Load8bpp(name0.c_str(), curW, curH, grayCur));
        std::string goldenPath = imagesDir + "/" + name0;
        int gw = 0, gh = 0;
        std::vector<uint8_t> grayGold;
        if (accept || !Load8bpp(goldenPath.c_str(), gw, gh, grayGold)) {
            REQUIRE(Save8bpp(goldenPath.c_str(), curW, curH, grayCur.data()));
        } else {
            REQUIRE(gw == curW);
            REQUIRE(gh == curH);
            REQUIRE(Compare8bpp(grayCur.data(), grayGold.data(), grayGold.size()));
        }
    }
    display.fillScreen(display.getBackgroundColor());

    // Render remaining pages (cap to avoid accidental explosion)
    for (int i = 1; !renderer.atEndOfFile() && i < 200; ++i) {
        std::tie(nextPlace, std::ignore) = renderer.renderPage(nextPlace);
        display.present();
        std::string name = std::string("txtfile_") + std::to_string(i) + ".png";
        REQUIRE(display.savePNG(name.c_str()));
        display.fillScreen(display.getBackgroundColor());
    }
    REQUIRE(renderer.getNumRenderedLines() > 0);
}

TEST_CASE("Renderer renders from hardcoded string", "[renderer][txt][string]") {
    std::string content = "The quick brown fox jumps over the lazy dog. "
                          "Pack my box with five dozen liquor jugs. "
                          "Sphinx of black quartz, judge my vow. ";
    // Repeat to fill multiple pages
    std::string longContent;
    for (int i = 0; i < 50; ++i)
        longContent += content;

    const std::string fontPath = std::string(TESTS_DIR) + "/Fixtures/NotoSans-Light.ttf";
    MockDisplay display(600, 800, fontPath.c_str(), 16);
    RendererDisplayRect rect{0, 0, 600, 800};
    FontHandle fontHandle{display.defaultFont()};
    Renderer renderer(display, rect, fontHandle);
    RendererDisplayRect inset{30, 30, static_cast<int16_t>(rect.width - 60),
                              static_cast<int16_t>(rect.height - 60)};
    renderer.setRect(inset);

    auto stream = std::make_shared<StdStringStream>(longContent, DocType::TXT);
    renderer.setStream(stream);

    const std::string imagesDir = std::string(TESTS_DIR) + "/Images";
    std::error_code ec;
    std::filesystem::create_directories(imagesDir, ec);
    const char *accept = std::getenv("ACCEPT_IMAGE_TESTS");

    auto [nextPlace, darkPercent] = renderer.renderFirstPage();
    (void)darkPercent;
    display.present();
    std::string name0 = std::string("txtstring_") + std::to_string(0) + ".png";
    REQUIRE(display.savePNG(name0.c_str()));
    {
        int curW = 0, curH = 0;
        std::vector<uint8_t> grayCur;
        REQUIRE(Load8bpp(name0.c_str(), curW, curH, grayCur));
        std::string goldenPath = imagesDir + "/" + name0;
        int gw = 0, gh = 0;
        std::vector<uint8_t> grayGold;
        if (accept || !Load8bpp(goldenPath.c_str(), gw, gh, grayGold)) {
            REQUIRE(Save8bpp(goldenPath.c_str(), curW, curH, grayCur.data()));
        } else {
            REQUIRE(gw == curW);
            REQUIRE(gh == curH);
            REQUIRE(Compare8bpp(grayCur.data(), grayGold.data(), grayGold.size()));
        }
    }
    display.fillScreen(display.getBackgroundColor());

    for (int i = 1; !renderer.atEndOfFile() && i < 200; ++i) {
        std::tie(nextPlace, std::ignore) = renderer.renderPage(nextPlace);
        display.present();
        std::string name = std::string("txtstring_") + std::to_string(i) + ".png";
        REQUIRE(display.savePNG(name.c_str()));
        display.fillScreen(display.getBackgroundColor());
    }
    REQUIRE(renderer.getNumRenderedLines() > 0);
}

// Records each drawn string with its position and the font state it was drawn with.
class RecordingDisplay : public MockDisplay {
public:
    struct Draw {
        std::string text;
        int16_t x;
        int16_t y;
        bool italic;
        bool supSub;
    };

    using MockDisplay::MockDisplay;

    void setItalicFont(FontHandle, bool italic) override { italic_ = italic; }
    void setSupSubFontSize(FontHandle) override { supSub_ = true; }
    void setNormalFontSize(FontHandle) override { supSub_ = false; }

    int16_t drawSingleLineOfText(const char *cstr, int16_t x, int16_t y, bool invert,
                                 FontHandle font) override {
        draws_.push_back({cstr, x, y, italic_, supSub_});
        return MockDisplay::drawSingleLineOfText(cstr, x, y, invert, font);
    }

    [[nodiscard]] auto italicOn() const -> bool { return italic_; }
    [[nodiscard]] auto draws() const -> const std::vector<Draw> & { return draws_; }
    void clearDraws() { draws_.clear(); }

    // Italic state of every whitespace-separated word drawn so far. A word drawn both upright
    // and italic is reported as inconsistent.
    [[nodiscard]] auto wordStyles(std::vector<std::string> &inconsistent) const
        -> std::map<std::string, bool> {
        std::map<std::string, bool> styles;
        for (const auto &draw : draws_) {
            std::istringstream words(draw.text);
            std::string word;
            while (words >> word) {
                auto [it, inserted] = styles.emplace(word, draw.italic);
                if (!inserted && it->second != draw.italic) {
                    inconsistent.push_back(word);
                }
            }
        }
        return styles;
    }

private:
    bool italic_{false};
    bool supSub_{false};
    std::vector<Draw> draws_;
};

namespace {

const std::string FONT_PATH = std::string(TESTS_DIR) + "/Fixtures/NotoSans-Light.ttf";

auto openEpubStream(const std::string &name) -> std::shared_ptr<EPubFileRendererStream> {
    const std::string path = std::string(TESTS_DIR) + "/Fixtures/epub/" + name;
    auto epub = std::make_shared<EPubFile>(std::make_shared<PosixUnzipperFileStream>(path.c_str()));
    REQUIRE(epub->isOpen());
    return std::make_shared<EPubFileRendererStream>(epub);
}

// Index of the first draw containing text, or -1.
auto findDraw(const std::vector<RecordingDisplay::Draw> &draws, const std::string &text) -> int {
    for (size_t i = 0; i < draws.size(); i++) {
        if (draws[i].text.find(text) != std::string::npos) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

} // namespace

TEST_CASE("Renderer draws italic runs, including nested ones", "[renderer][italic]") {
    RecordingDisplay display(600, 800, FONT_PATH.c_str(), 16);
    FontHandle fontHandle{display.defaultFont()};
    Renderer renderer(display, RendererDisplayRect{0, 0, 600, 800}, fontHandle);
    renderer.setRect(RendererDisplayRect{30, 30, 540, 740});
    renderer.setStream(openEpubStream("italics.epub"));

    renderer.renderFirstPage();
    REQUIRE(renderer.atEndOfFile());
    CHECK_FALSE(display.italicOn());

    std::vector<std::string> inconsistent;
    const auto styles = display.wordStyles(inconsistent);
    CHECK(inconsistent.empty());

    const std::vector<std::pair<std::string, bool>> expected = {
        // <i> containing <em>: text after the inner element stays italic
        {"Plain", false},
        {"The", true},
        {"Odyssey", true},
        {"was", true},
        {"long", true},
        {"afterward.", false},
        // Nested italic-class spans
        {"outer", true},
        {"inner", true},
        {"tail", true},
        {"done", false},
        // Compound class with <em> nested inside
        {"alpha", true},
        {"beta", true},
        {"gamma", true},
        {"delta", false},
        // A non-italic span inside <i> doesn't end the italic run
        {"first", true},
        {"second", true},
        {"third", true},
        {"fourth", false},
    // Superscript inside <i> stays italic
#if CONFIG_EPUB_RENDERER_NATIVE_SUPERSUBSCRIPT_CHARACTERS
        {"note", true},
        {"2", true},
#else
        {"note2", true},
#endif
        {"ending", true},
        {"closing", false},
        // Bold isn't italic
        {"strongword", false},
        {"boldword", false},
        {"regular", false},
        // Link delimiters stay upright around italic link text
        {"[See", false},
        {"linknote", true},
        {"]", false},
        {"after1", false},
        {"[plain", false},
        {"linked]", false},
        {"spaced", true},
        {"after3", false},
    };
    for (const auto &[word, italic] : expected) {
        INFO("word '" << word << "'");
        auto it = styles.find(word);
        REQUIRE(it != styles.end());
        CHECK(it->second == italic);
    }
}

TEST_CASE("Renderer closes links that end in italic text", "[renderer][italic]") {
    RecordingDisplay display(600, 800, FONT_PATH.c_str(), 16);
    FontHandle fontHandle{display.defaultFont()};
    Renderer renderer(display, RendererDisplayRect{0, 0, 600, 800}, fontHandle);
    renderer.setRect(RendererDisplayRect{30, 30, 540, 740});
    renderer.setStream(openEpubStream("italics.epub"));
    renderer.renderFirstPage();
    const auto &draws = display.draws();

    // The closing delimiter follows the italic word directly, with the link's trailing space
    // after it.
    for (const std::string word : {"linknote", "spaced"}) {
        INFO("link ending with '" << word << "'");
        const int idx = findDraw(draws, word);
        REQUIRE(idx >= 0);
        CHECK(draws[idx].text == word);
        REQUIRE(static_cast<size_t>(idx + 1) < draws.size());
        CHECK(draws[idx + 1].text.rfind("] ", 0) == 0);
        CHECK(draws[idx + 1].y == draws[idx].y);
    }
}

TEST_CASE("Renderer keeps superscripts together across italic boundaries", "[renderer][italic]") {
    RecordingDisplay display(600, 800, FONT_PATH.c_str(), 16);
    FontHandle fontHandle{display.defaultFont()};
    Renderer renderer(display, RendererDisplayRect{0, 0, 600, 800}, fontHandle);
    renderer.setRect(RendererDisplayRect{30, 30, 540, 740});
    renderer.setStream(openEpubStream("italics.epub"));
    renderer.renderFirstPage();
    const auto &draws = display.draws();

    // <sup>sa<i>sb</i>sc</sup>: every part keeps the superscript style; only "sb" is italic.
    for (const std::string part : {"sa", "sb", "sc"}) {
        INFO("superscript part '" << part << "'");
        const int idx = findDraw(draws, part);
        REQUIRE(idx >= 0);
        CHECK(draws[idx].italic == (part == "sb"));
#if CONFIG_EPUB_RENDERER_NATIVE_SUPERSUBSCRIPT_CHARACTERS
        CHECK(draws[idx].supSub);
#endif
    }
}

// Pages through wrapping.epub at a narrow width and checks that no word is split across lines or
// pages where it was only split into tokens by formatting (no space or hyphen at the break).
TEST_CASE("Renderer wraps words split by formatting as a unit", "[renderer][italic][wrap]") {
    RecordingDisplay display(600, 800, FONT_PATH.c_str(), 16);
    FontHandle fontHandle{display.defaultFont()};
    Renderer renderer(display, RendererDisplayRect{0, 0, 600, 800}, fontHandle);
    renderer.setRect(RendererDisplayRect{30, 30, 190, 260});
    renderer.setStream(openEpubStream("wrapping.epub"));

    std::vector<std::shared_ptr<Place>> pageStarts;
    std::vector<std::string> pageFirstTexts;
    std::string previousText;
    int lineBreaks = 0;
    int hyphenBreaks = 0;

    auto [nextPlace, darkPercent] = renderer.renderFirstPage();
    (void)darkPercent;
    for (int page = 0; page < 200; page++) {
        pageStarts.push_back(renderer.getCurrentPlace());
        const auto &draws = display.draws();
        REQUIRE_FALSE(draws.empty());
        pageFirstTexts.push_back(draws.front().text);

        for (size_t i = 0; i < draws.size(); i++) {
            // Superscripts are drawn higher, so detect a new line by x going back left.
            const bool newLine = (i == 0) ? (page > 0) : (draws[i].x < draws[i - 1].x);
            if (newLine) {
                lineBreaks++;
                INFO("page " << page << " line starting with '" << draws[i].text << "' after '"
                             << previousText << "'");
                CHECK(((previousText.back() == ' ') || (previousText.back() == '-')));
                hyphenBreaks += (previousText.back() == '-') ? 1 : 0;
            }
            previousText = draws[i].text;
        }

        if (renderer.atEndOfFile()) {
            break;
        }
        display.clearDraws();
        std::tie(nextPlace, std::ignore) = renderer.renderPage(nextPlace);
    }
    REQUIRE(renderer.atEndOfFile());
    REQUIRE(pageStarts.size() > 3);
    CHECK(lineBreaks > 50);
    // Lines can still break right after a hyphen inside a word.
    CHECK(hyphenBreaks > 0);

    // Paging backward must find the same page starts, so a page never starts mid-word there
    // either.
    for (size_t page = pageStarts.size() - 1; page > 0; page--) {
        INFO("previous page of page " << page);
        renderer.setCurrentPlace(pageStarts[page]);
        auto previous = renderer.getPreviousPlace();
        display.clearDraws();
        renderer.renderPage(previous);
        REQUIRE_FALSE(display.draws().empty());
        CHECK(display.draws().front().text == pageFirstTexts[page - 1]);
    }
}

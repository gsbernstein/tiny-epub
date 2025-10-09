#pragma once

#include <Spiram/Map.hpp>
#include <Spiram/String.hpp>
#include <Spiram/Vector.hpp>
#include <StringUtil/StringUtil.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "EPubDefs.hpp"
#include "EPubToc.hpp"

class EPubOpf {
public:
    EPubOpf(const char *path) : path_(path) {}

    struct ManifestItem {
        SpiramString id;
        SpiramString href;
        SpiramString mediaType;
    };
    struct SpineItem {
        ManifestItem *item;
        int chapterIdx;
        bool chapterStart;
    };

    void parse(const std::shared_ptr<uint8_t[]> &buffer, uint32_t length);
    void loadSpine(const EPubTocChapterVector &toc);
    auto findChapterIdxInToc(const EPubTocChapterVector &toc, SpineItem &spineItem) -> ChapterIdx;

    [[nodiscard]] auto getSpine(SpineIdx idx) const -> const SpineItem & { return spine_[idx]; }

    [[nodiscard]] auto getFullPath(const std::string_view &fileName) const -> std::string {
        return StringFormat("%s%.*s", basePath_.c_str(), fileName.size(), fileName.data());
    }

    [[nodiscard]] inline auto getSpineCount() const -> uint16_t { return spine_.size(); }

    void extractPath(const char *fname, std::string &path);

    auto getSpineIdx(const std::string_view &href) const -> SpineIdx;

    auto getChapterIdx(const std::string_view &href) const -> ChapterIdx;
    auto getChapterIdx(SpineIdx spineIdx) const -> ChapterIdx;
    auto spineIdxIsChapterStart(SpineIdx spineIdx) const -> bool;
    auto getChapterFirstSpineIdx(SpineIdx spineIdx) const -> SpineIdx;
    auto getChapterFirstHref(const std::string_view &spineHRef) const -> std::string;

    auto exists(const std::string &id) const -> bool;

    // TODO: It'd probably be good practice to use a type alias for manifest IDs
    auto getHrefByManifestId(const std::string &manifestId) const -> std::string_view;
    auto getManifestIdByHref(const std::string_view &href) const -> std::string;

private:
    const char *path_;

    std::string basePath_;

    using ManifestMap = SpiramMap<SpiramString, ManifestItem>;
    using ManifestIterator = SpiramMap<SpiramString, ManifestItem>::iterator;
    using SpineVector = SpiramVector<SpineItem>;

    std::string title_;
    std::string creator_;
    ManifestMap manifest_;
    SpineVector spine_;
};

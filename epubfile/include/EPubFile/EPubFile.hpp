#pragma once

#include <Unzipper/Unzipper.hpp>
#include <optional>
#include <pugixml.hpp>
#include <utility>

#include "EPubDefs.hpp"
#include "EPubDrm.hpp"
#include "EPubOpf.hpp"

class EPubFile {
public:
    EPubFile(std::shared_ptr<UnzipperFileStream> file, std::shared_ptr<EPubDrm> drm = nullptr)
        : epubUnzipper_(file), currentFileBuffer_(nullptr), drm_(drm) {
        open();
    }

    ~EPubFile() { close(); };

    EPubFile(const EPubFile &) = delete;
    EPubFile &operator=(const EPubFile &) = delete;

    // Shims to maintain old behavior
    // TODO: It would be nice to remove this, or at least make the behavior more explicit
    // e.g. this could be named noPageBreaksOnChapters or something like that depending on
    // what exactly it does
    [[nodiscard]] auto isAnArticle() const -> bool { return isAnArticle_; }
    void setIsAnArticle(bool isAnArticle) { isAnArticle_ = isAnArticle; }

    // [[nodiscard]] inline auto getSpineIdxForOffset(uint32_t offset) const -> SpineIdx {
    //     return opf_->getSpineIdxForOffset(offset);
    // }

    [[nodiscard]] inline auto getSpine(SpineIdx idx) const -> const EPubOpf::SpineItem & {
        return opf_->getSpine(idx);
    }

    [[nodiscard]] auto findFileOffsetAtCharOffset(const std::string &path, uint32_t charOffset)
        -> uint32_t;

    [[nodiscard]] auto findCharOffsetAtFileOffset(const std::string_view &path, uint32_t fileOffset)
        -> uint32_t;

    [[nodiscard]] inline auto getSpineCount() const -> uint16_t { return opf_->getSpineCount(); }

    [[nodiscard]] auto getFile(const std::string &path)
        -> std::pair<std::shared_ptr<uint8_t[]>, uint32_t>;

    [[nodiscard]] inline auto getSpineIdx(const std::string_view &href) const -> SpineIdx {
        return opf_->getSpineIdx(href);
    }

    [[nodiscard]] inline auto getHrefByManifestId(const std::string &manifestId) const
        -> std::string_view {
        return opf_->getHrefByManifestId(manifestId);
    }

    [[nodiscard]] inline auto getManifestIdByHref(const std::string_view &href) const
        -> std::string {
        return opf_->getManifestIdByHref(href);
    }

    [[nodiscard]] auto getXHTMLFile(const std::string_view &path) -> pugi::xml_document &;

    [[nodiscard]] inline auto getOffset(const pugi::char_t *item) const -> uint32_t {
        return item - reinterpret_cast<pugi::char_t *>(currentFileBuffer_.get());
    }

    [[nodiscard]] inline auto getFullPath(const std::string &filename) const -> std::string {
        return opf_->getFullPath(filename);
    }

    [[nodiscard]] inline auto isOpen() const -> bool { return fileOpen_; }

    [[nodiscard]] auto getRelativeFilePath(const std::string &filename, SpineIdx spineIdx) const
        -> std::string {
        std::string mainFilePath =
            opf_->getFullPath(static_cast<std::string>(opf_->getSpine(spineIdx).item->href));
        std::string path;
        opf_->extractPath(mainFilePath.c_str(), path);
        return path + filename;
    }

    auto getUncompressedSize(const std::string &href) -> uint32_t;
    auto getUncompressedSize(SpineIdx spineIdx) -> uint32_t;

    auto getChapterIdx(const std::string_view &href) const -> ChapterIdx;
    auto getChapterIdx(SpineIdx spineIdx) const -> ChapterIdx;
    auto spineIdxIsChapterStart(SpineIdx spineIdx) const -> bool;
    auto getChapterFirstHref(const std::string_view &href) const -> std::string {
        return opf_->getChapterFirstHref(href);
    }
    auto getChapterFirstSpineIdx(SpineIdx spineIdx) const -> SpineIdx;
    auto getOffsetPreviousSpineItemsInChapter(SpineIdx spineIdx) -> uint32_t;

    auto hasToc() -> bool;
    auto getToc() -> EPubTocChapterVector;

    auto hasEpubAddition(const std::string_view &spineHref, const std::string &additionType)
        -> bool;

    void close();

private:
    Unzipper epubUnzipper_;
    std::shared_ptr<EPubTocChapterVector> toc_;

    pugi::xml_document currentFileDoc_;

    bool fileOpen_{false};

    std::shared_ptr<EPubOpf> opf_{nullptr};

    // std::unique_ptr<pugi::char_t[]> currentFileBuffer_;
    std::shared_ptr<uint8_t[]> currentFileBuffer_;
    std::string currentFilePath_;
    std::shared_ptr<EPubDrm> drm_;
    std::optional<std::vector<uint8_t>> drmKey_;

    bool isAnArticle_{false};

    auto open() -> bool;
};

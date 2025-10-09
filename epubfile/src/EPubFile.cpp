#include "EPubFile.hpp"

#include <Spiram/Memory.hpp>
#include <algorithm>
#include <vector>

#include "EPubDefs.hpp"
#include "EPubMetaFile.hpp"

auto EPubFile::findFileOffsetAtCharOffset(const std::string &path, uint32_t charOffset)
    -> uint32_t {

    pugi::xml_document &doc = getXHTMLFile(path);

    struct Walker : pugi::xml_tree_walker {
        auto for_each(pugi::xml_node &node) -> bool override {
            if (node.type() == pugi::xml_node_type::node_pcdata) {
                uint32_t length = strlen(node.value());
                if ((chOffset + length) > charOffset) {
                    offset = epub->getOffset(node.value()) + (charOffset - chOffset);
                    return false;
                }
                chOffset += length;
            }
            return true;
        }

        uint32_t offset{};
        uint32_t chOffset{};
        uint32_t charOffset{};
        EPubFile *epub{};
    } walker;

    walker.offset = walker.chOffset = 0;
    walker.charOffset = charOffset;
    walker.epub = this;

    doc.child("html").child("body").traverse(walker);

    return walker.offset;
}

auto EPubFile::findCharOffsetAtFileOffset(const std::string_view &path, uint32_t fileOffset)
    -> uint32_t {

    pugi::xml_document &doc = getXHTMLFile(path);

    struct Walker : pugi::xml_tree_walker {
        auto for_each(pugi::xml_node &node) -> bool override {
            if (node.type() == pugi::xml_node_type::node_pcdata) {
                uint32_t length = strlen(node.value());
                uint32_t nodeOffset = node.offset_debug();
                if ((nodeOffset + length) >= fileOffset) {
                    uint32_t offset = (fileOffset > nodeOffset) ? fileOffset - nodeOffset : 0;
                    charOffset += offset;
                    return false;
                }
                charOffset += length;
            }
            return true;
        }

        uint32_t charOffset{};
        uint32_t fileOffset{};
    } walker;

    walker.charOffset = 0;
    walker.fileOffset = fileOffset;

    doc.child("html").child("body").traverse(walker);

    return walker.charOffset;
}

// Returns a pointer to the buffer containing the extracted file from the EPub and it's length in a
// tuple
auto EPubFile::getFile(const std::string &completeFilePath)
    -> std::pair<std::shared_ptr<uint8_t[]>, uint32_t> {

    EPUB_PROFILE_START(UnzipperOpenFile);
    auto res = epubUnzipper_.openFile(completeFilePath);
    EPUB_PROFILE_END(UnzipperOpenFile);

    if (!res) {
        LOGE(EPUB_TAG, "Unzipper Failed to open file %s", completeFilePath.c_str());
    } else {
        EPUB_PROFILE_START(UnzipperGetFileSize);
        auto size = epubUnzipper_.getFileSize();
        EPUB_PROFILE_END(UnzipperGetFileSize);

        if (size > 0) {
            uint32_t bufferSize = size + 1;
            auto buffer = SpiramArrayPtr<uint8_t>(bufferSize);
            buffer[size] = 0; // Pugixml requires null termination

            if (buffer != nullptr) {
                EPUB_PROFILE_START(UnzipperReadFromFile);
                uint32_t length = epubUnzipper_.readFile(buffer.get(), size);
                epubUnzipper_.closeFile();
                EPUB_PROFILE_END(UnzipperReadFromFile);
                if (length == size) {
                    return {std::move(buffer), bufferSize};
                } else {
                    LOGW(EPUB_TAG,
                         "Failed to read file %s, read %" PRIu32 " bytes, expected %" PRIu32,
                         completeFilePath.c_str(), length, size);
                }
            } else {
                epubUnzipper_.closeFile();
                LOGE(EPUB_TAG, "Unable to allocate space for file %s, space required: %" PRIu32,
                     completeFilePath.c_str(), static_cast<uint32_t>(size));
            }
        }
    }

    return {nullptr, 0};
}

auto EPubFile::getXHTMLFile(const std::string_view &path) -> pugi::xml_document & {
    std::string filePath = opf_->getFullPath(path);
    // LOGW(EPUB_TAG, "Loading file %s", filePath.c_str());
    uint32_t length;

    if (currentFilePath_ != filePath) {

        // LOGW(EPUB_TAG, "currentFilePath_: %s, filePath : %s", currentFilePath_.c_str(),
        // filePath.c_str());

        std::tie(currentFileBuffer_, length) = getFile(filePath);

        if (currentFileBuffer_ != nullptr) {
            if (drm_ != nullptr && drmKey_.has_value()) {
                std::tie(currentFileBuffer_, length) =
                    drm_->decryptInPlaceIfNecessary(drmKey_.value(), currentFileBuffer_, length);
            }
            // Using load_buffer_inplace() is mandatory here as it permits the
            // computation of a node offset location, as string elements are taken
            // directly in the currentFileBuffer instead of being copied.
            // (see the EPubFile::getOffset() method).

            EPUB_PROFILE_START(ParseXHTML);
            pugi::xml_parse_result result = currentFileDoc_.load_buffer_inplace(
                currentFileBuffer_.get(), length,
                (pugi::parse_default | pugi::parse_ws_pcdata) & ~pugi::parse_escapes);
            EPUB_PROFILE_END(ParseXHTML);

            if (result) {
                currentFilePath_ = filePath;
            } else {
                LOGE(EPUB_TAG, "Failed to parse XHTML file %s: %s", filePath.c_str(),
                     result.description());
            }
        } else {
            LOGE(EPUB_TAG, "Unable to unzip file %s", filePath.c_str());
        }
    }

    return currentFileDoc_;
}

auto EPubFile::getUncompressedSize(const std::string &href) -> uint32_t {
    std::string filePath = opf_->getFullPath(href);
    // LOGW(EPUB_TAG, "Loading file %s", filePath.c_str());
    if (epubUnzipper_.openFile(filePath)) {
        uint32_t size = epubUnzipper_.getFileSize();
        epubUnzipper_.closeFile();
        return size;
    }
    return 0;
}

auto EPubFile::getUncompressedSize(SpineIdx spineIdx) -> uint32_t {
    return getUncompressedSize(static_cast<std::string>(opf_->getSpine(spineIdx).item->href));
}

auto EPubFile::getChapterIdx(const std::string_view &href) const -> ChapterIdx {
    return opf_->getChapterIdx(href);
}

auto EPubFile::getChapterIdx(SpineIdx spineIdx) const -> ChapterIdx {
    return opf_->getChapterIdx(spineIdx);
}

auto EPubFile::spineIdxIsChapterStart(SpineIdx spineIdx) const -> bool {
    return opf_->spineIdxIsChapterStart(spineIdx);
}

auto EPubFile::getChapterFirstSpineIdx(SpineIdx spineIdx) const -> SpineIdx {
    return opf_->getChapterFirstSpineIdx(spineIdx);
}

auto EPubFile::getOffsetPreviousSpineItemsInChapter(SpineIdx spineIdx) -> uint32_t {
    auto previousSpineOffset = 0;
    auto chapterStartSpineIdx = getChapterFirstSpineIdx(spineIdx);
    for (auto i = chapterStartSpineIdx; i < spineIdx; i++) {
        previousSpineOffset += getUncompressedSize(i);
    }
    return previousSpineOffset;
}

auto EPubFile::hasToc() -> bool {
    std::string tocId = "nav";
    return opf_->exists(tocId);
}

auto EPubFile::getToc() -> EPubTocChapterVector {
    if (toc_) {
        return *toc_;
    }

    std::string tocId = "nav";
    auto tocPath = opf_->getHrefByManifestId(tocId);
    pugi::xml_document &tocDoc = getXHTMLFile(tocPath);

    EPubTocChapterVector toc;
    uint32_t cumulativeOffset = 0;

    std::function<void(pugi::xml_node, EPubTocChapterVector &)> processNavPoints;
    processNavPoints = [&](pugi::xml_node navPoint, EPubTocChapterVector &toc) {
        for (auto navPointChild : navPoint.children("li")) {
            EPubTocChapter chapter;
            chapter.title = navPointChild.child("a").child_value();
            chapter.href = navPointChild.child("a").attribute("href").value();
            pugi::xml_attribute epubOffsetAttr =
                navPointChild.child("a").attribute("data-epub-offset");
            if (epubOffsetAttr) {
                chapter.epubOffset =
                    static_cast<uint32_t>(strtoul(epubOffsetAttr.value(), nullptr, 10));
            } else {
                chapter.epubOffset = cumulativeOffset;
            }
            pugi::xml_attribute endEpubOffsetAttr =
                navPointChild.child("a").attribute("data-end-epub-offset");
            if (endEpubOffsetAttr) {
                chapter.endEpubOffset =
                    static_cast<uint32_t>(strtoul(endEpubOffsetAttr.value(), nullptr, 10));
            } else {
                auto fileSize = getUncompressedSize(static_cast<std::string>(chapter.href));
                cumulativeOffset += fileSize;
                chapter.endEpubOffset = cumulativeOffset;
            }

            // LOGW(EPUB_TAG, "Chapter %s: %s (%" PRIu32 " - %" PRIu32 ")", chapter.title.c_str(),
            //       chapter.href.c_str(), chapter.epubOffset, chapter.endEpubOffset);
            toc.push_back(chapter);

            // Recurse into child <ol> elements, if any
            for (auto nestedList : navPointChild.children("ol")) {
                processNavPoints(nestedList, toc);
            }
        }
    };

    // Process the top-level <ol> elements in the TOC
    for (auto navPoint : tocDoc.child("html").child("body").child("nav").children("ol")) {
        processNavPoints(navPoint, toc);
    }

    toc_ = std::make_shared<EPubTocChapterVector>(toc);
    return toc;
}

auto EPubFile::hasEpubAddition(const std::string_view &spineHref, const std::string &additionType)
    -> bool {
    // Find the chapterId corresponding to this spineHref
    auto chapterSpineHref = opf_->getChapterFirstHref(spineHref);
    auto chapterId = opf_->getManifestIdByHref(chapterSpineHref);

    std::string additionId = additionType + "-" + chapterId;

    auto hasDirectChapterAddition = opf_->exists(additionId);
    auto hasToc = opf_->exists("nav");
    if (hasDirectChapterAddition && hasToc) {
        return true;
    }

    return false;
}

auto EPubFile::open() -> bool {

    // PROFILE_START(openEPub);

    EPUB_PROFILE_START(UnzipperOpen);
    if (!epubUnzipper_.open()) {
        LOGE(EPUB_TAG, "Failed to open ePub file");
        return false;
    }
    EPUB_PROFILE_END(UnzipperOpen);

    EPUB_PROFILE_START(EPubManifest);
    EPubMetaFile manifest("META-INF/container.xml");
    auto [manifestBuffer, manifestLength] = getFile("META-INF/container.xml");
    if (manifestBuffer == nullptr) {
        LOGE(EPUB_TAG, "Failed to open ePub's manifest %s", "META-INF/container.xml");
        return false;
    } else {
        manifest.parse(manifestBuffer, manifestLength);
    }
    EPUB_PROFILE_END(EPubManifest);

    // for (auto &path : manifest.rootfilePaths) {
    //     LOGI(EPUB_TAG, "Rootfile path: %s", path.c_str());
    // }

    if (manifest.rootfilePaths.empty()) {
        LOGE(EPUB_TAG, "No rootfile paths found");
        return false;
    } else if (manifest.rootfilePaths.size() > 1) {
        LOGW(EPUB_TAG, "Multiple rootfile paths found");
    }

    EPUB_PROFILE_START(EPubOpf)
    auto [opfBuffer, length] = getFile(manifest.rootfilePaths[0]);
    if (opfBuffer == nullptr) {
        LOGE(EPUB_TAG, "Failed to open ePub's opf %s", manifest.rootfilePaths[0].c_str());
        return false;
    } else {
        opf_ = std::make_shared<EPubOpf>(manifest.rootfilePaths[0].c_str());
        opf_->parse(opfBuffer, length);
        auto toc = getToc();
        opf_->loadSpine(toc);
    }
    EPUB_PROFILE_END(EPubOpf)

    EPUB_PROFILE_START(EPubParseKey)
    // Load the DRM encryption key if it exists
    if (drm_ != nullptr) {
        auto [drmKeyBuffer, drmKeyLength] = getFile("sol_drm.key");
        if (drmKeyBuffer != nullptr) {
            drmKey_ = drm_->parseKeyfile(drmKeyBuffer.get(), drmKeyLength);
        }
    }
    EPUB_PROFILE_END(EPubParseKey)

    // PROFILE_END_LOG_MS(openEPub);

    // LOGD(EPUB_TAG, "Total size of all ePub text is %d", opf_->getTotalSize());
    fileOpen_ = true;
    return true;
}

void EPubFile::close() {
    epubUnzipper_.close();
    fileOpen_ = false;
}

#include "Catch2/catch_amalgamated.hpp"

#include <string>
#include <EPubFile/EPubFile.hpp>

TEST_CASE("EPubFile opens basic EPUB and exposes TOC", "[epub]") {
    const std::string fixturesDir = std::string(TESTS_DIR) + "/Fixtures/epub";
    const std::string sampleEpub = fixturesDir + "/LastQuestion.epub";

    // Placeholder types; compilation will fail until dependencies are provided.
    auto file = std::make_shared<PosixUnzipperFileStream>(sampleEpub.c_str());
    EPubFile epub(file);
    REQUIRE(epub.isOpen());
    REQUIRE(epub.hasToc());
}

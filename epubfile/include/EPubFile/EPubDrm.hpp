#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

// Interface for optional DRM providers used by EPubFile
class EPubDrm {
public:
    virtual ~EPubDrm() = default;

    // Parse a keyfile payload and return an AES key if present
    virtual auto parseKeyfile(const uint8_t *data, size_t length)
        -> std::optional<std::vector<uint8_t>> = 0;

    // Decrypt and optionally decompress data in place if necessary.
    // Returns the (possibly new) data buffer and its length.
    virtual auto decryptInPlaceIfNecessary(const std::vector<uint8_t> &aesKey,
                                           const std::shared_ptr<uint8_t[]> &data, size_t length)
        -> std::pair<std::shared_ptr<uint8_t[]>, size_t> = 0;
};

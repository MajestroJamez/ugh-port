// What of a level is solid.
#pragma once

#include <cstdint>
#include <utility>
#include <vector>

namespace ugh::data {

/**
 * The collision mask of a level: bit 7 of the background page's colours, 384 x 192 px. The original probes the page
 * in VGA memory, so a probe point is a linear pixel index (y * 384 + x) that runs into the next or previous row
 * at the edges; outside the page nothing is solid (re/notes/phase2-data.md).
 */
class CollisionMask {
public:
    static constexpr int WIDTH = 384, HEIGHT = 192;

    CollisionMask() = default;
    explicit CollisionMask(std::vector<uint8_t> bits) : bits_(std::move(bits)) {}

    /** The pixel at y * WIDTH + x is solid. */
    bool solid(int index) const;

private:
    std::vector<uint8_t> bits_;   // 48 bytes per row, pixel x in bit 7 - (x & 7) of byte x / 8
};

}  // namespace ugh::data

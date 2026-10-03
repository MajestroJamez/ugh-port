// A sprite of the original: what the renderer draws for an entity.
#pragma once

#include <cstdint>

namespace ugh::data {

/** A sprite number (assets/sprites). */
using Sprite = uint16_t;

/** Nothing drawn; an entity without a sprite is hidden (a free bonus slot, a passenger behind its door). */
constexpr Sprite NO_SPRITE = 0xffff;

}  // namespace ugh::data

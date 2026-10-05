// What the C API shows of the level being played.
#pragma once

#include <string>

#include "data/GameData.hpp"
#include "game/Game.hpp"
#include "ugh_logic.h"

namespace ugh::api {

/** Fills the level part of `view` (ugh_logic_get_view): copters, passengers, enemies, bonus items, rain. */
void viewLevel(const game::Game& game, ugh_logic_view& view);

/** What a sprite an entity shows is in the data (ugh_logic_get_sprite). */
struct SpriteName {
    std::string name;   // an animation ("kind1.walkLeft"), a sprite of the rules ("standingPassenger"), a bonus kind,
                        //   a speech bubble ("destinationBubble", "impatientBubble")
    int frame = 0;      // the first frame of the animation that shows the sprite; 0 for a single sprite
    int frames = 1;     // the frames of the animation; 1 for a single sprite
};

/** What `sprite` is in `data` (an animation before the rules' sprites); false when no entity or bubble shows it. */
bool nameSprite(const data::GameData& data, int sprite, SpriteName& out);

}  // namespace ugh::api

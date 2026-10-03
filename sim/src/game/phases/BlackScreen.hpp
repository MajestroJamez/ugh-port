// The palette turns black.
#pragma once

#include "game/phases/TimedPhase.hpp"

namespace ugh::game::phases {

/** 113b:4e9b - Host.kt blackPalette: the palette turns black, 32 colours per retrace, before a screen is drawn. */
class BlackScreen : public TimedPhase {
public:
    static constexpr int RETRACES = 8;

    explicit BlackScreen(FlowPhase& next) : TimedPhase(RETRACES, next) {}
};

}  // namespace ugh::game::phases

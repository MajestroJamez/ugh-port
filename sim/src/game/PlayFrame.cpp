#include "game/PlayFrame.hpp"

#include <cstdint>

#include "core/Audit.hpp"

namespace ugh::game {

namespace {

constexpr uint8_t SCANCODE_ESC = 0x01, SCANCODE_P = 0x19;

}  // namespace

PlayFrame::PlayFrame(const data::GameData& data, model::Level& level, input::Keyboard& keyboard)
    : data_(data), level_(level), keyboard_(keyboard), physics_(level) {}

void PlayFrame::run() {
    using core::audit::setContext;
    setContext("water");
    level_.water().move(level_.waterSpeed());
    setContext("keys");
    readKeys();
    setContext("copters");
    flyCopters();
    setContext("passengers");
    level_.updatePassengers();
    setContext("enemies");
    level_.updateEnemies();
    setContext("bonuses");
    level_.bonuses().update(level_);

    setContext("pixels");
    level_.updatePassengerPixels();
    setContext("rotors");
    spinRotors();
    setContext("rain");
    if (level_.windy()) level_.rain().move(level_.water().row(), level_.wind(), level_.session().random(),
                                           level_.diagnostics());
    setContext("water surface");
    level_.water().animateSurface();
    level_.rain().stopAt(level_.water().row());
    setContext("-");
}

void PlayFrame::readKeys() {
    uint8_t key = keyboard_.read().scancode;
    if (key == SCANCODE_P) level_.diagnostics().report("pause (P) is not supported");
    if (key == SCANCODE_ESC) {
        level_.session().giveUp();
        if (!level_.fade().fadingOut()) level_.fade().startFadeOut();
    }
}

/** The copters stand still while the level fades in. */
void PlayFrame::flyCopters() {
    if (level_.fade().coptersWaiting()) return;
    for (int player = 0; player < level_.copterCount(); player++) physics_.fly(player);
}

/** 113b:418d - Draw.kt drawCopter (without the drawing). */
void PlayFrame::spinRotors() {
    for (int player = 0; player < level_.copterCount(); player++)
        level_.copter(player).spinRotor(data_.rotorFirst(player), data_.rotorEnd(player));
}

}  // namespace ugh::game

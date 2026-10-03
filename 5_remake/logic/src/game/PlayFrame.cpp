#include "game/PlayFrame.hpp"

#include "physics/CopterPhysics.hpp"

namespace ugh::game {

void PlayFrame::run() {
    world::Level& level = context_.level;
    level.water().move(level.definition()->waterSpeed);
    readKeys();
    flyCopters();
    passengers_.update({context_, bonuses_});
    enemies_.update({context_, passengers_, bonuses_});
    bonuses_.update(context_);
    passengers_.frameShown();
    spinRotors();
    if (level.windy()) level.rain().move(level.water().row(), level.wind(), context_.session.random(), context_.diagnostics);
    level.water().animateSurface();
    level.rain().stopAt(level.water().row());
}

void PlayFrame::readKeys() {
    input::MenuKey key = menu_.last();
    if (key == input::MenuKey::Pause) context_.diagnostics.report("pause (P) is not supported");
    if (key == input::MenuKey::Escape) {
        context_.session.giveUp();
        if (!context_.level.fade().fadingOut()) context_.level.fade().startFadeOut();
    }
}

/** The copters stand still while the level fades in. */
void PlayFrame::flyCopters() {
    if (context_.level.fade().coptersWaiting()) return;
    physics::CopterPhysics physics(context_);
    for (int player = 0; player < context_.level.copters().count(); player++) physics.fly(player);
}

void PlayFrame::spinRotors() {
    const data::SpriteIds& sprites = context_.data.sprites();
    for (int player = 0; player < context_.level.copters().count(); player++)
        context_.level.copters()[player].rotor().spin(sprites.firstRotor[player], sprites.lastRotor[player]);
}

}  // namespace ugh::game

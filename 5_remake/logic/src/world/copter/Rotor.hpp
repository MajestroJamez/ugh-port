// The rotor of a copter.
#pragma once

namespace ugh::world::copter {

/** The rotor of a copter: the harder the pilot works it this frame (effort), the faster its sprites turn. */
class Rotor {
public:
    /** At the start of an attempt: at its first sprite. */
    void start(int firstSprite) {
        counter_ = -1;
        sprite_ = firstSprite;
    }

    /** A new frame of the physics: no effort yet. */
    void newFrame() { effort_ = 0; }
    void addEffort(int effort) { effort_ += effort; }
    /** How hard the pilot works the rotor this frame; it lasts until the copter flies again. */
    int effort() const { return effort_; }
    /** A game resumed at an attempt (game::AttemptStart): the effort the pilot last worked it with. */
    void resumeEffort(int effort) { effort_ = effort; }

    /** The next of its sprites (first .. last) when its counter runs out; more effort runs it out faster. */
    void spin(int firstSprite, int lastSprite);
    int sprite() const { return sprite_; }
    int counter() const { return counter_; }

private:
    int effort_ = 0;
    int sprite_ = 0;
    int counter_ = 0;
};

}  // namespace ugh::world::copter

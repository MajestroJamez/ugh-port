// An enemy (level list C).
#pragma once

#include "core/Fixed.hpp"
#include "core/Word.hpp"
#include "data/Animation.hpp"
#include "data/AnimationPair.hpp"
#include "data/EnemyKind.hpp"
#include "data/Sprite.hpp"
#include "model/Animator.hpp"
#include "model/EnemyFacing.hpp"
#include "model/EnemyTable.hpp"
#include "model/EnemyTimer.hpp"

namespace ugh::enemies {
class EnemyState;
}

namespace ugh::model {

class Copter;
class Level;

/**
 * An enemy: the flyer (pterodactyl), the walker (triceratops), the blower or the tree. Its kind's behavior
 * (enemies/) places it, its states drive it through the methods below.
 *
 * The enemies live in fixed slots and a level load sets only what the enemy's kind uses: the rest stays from the
 * slot's earlier use, like in the original (the golden replays show it).
 */
class Enemy {
public:
    struct Snapshot {
        const data::EnemyKind* kind = nullptr;
        const enemies::EnemyState* state = nullptr;
        int pad = 0;                // walker, tree
        core::Fixed x, y;           // top left
        EnemyTable table;
        core::Word startDelay;      // flyer: frames before its first flight
        core::Fixed vx;             // per frame
        EnemyTimer timer;
        Animator animator;
        data::Sprite sprite = data::NO_SPRITE;
        EnemyFacing facing;
    };

    Enemy() = default;
    /** The enemy in slot `index` of the level list (the events name it by the index). */
    explicit Enemy(int index) : index_(index) {}

    int index() const { return index_; }
    const data::EnemyKind& kind() const { return *s_.kind; }

    // ------------------------------------------------------------ the level load (enemies/EnemyBehavior)

    /** A new enemy of `kind` in the slot, in `state`. */
    void place(const data::EnemyKind& kind, const enemies::EnemyState& state);
    void placeOnPad(int pad) { s_.pad = pad; }
    void moveTo(core::Fixed x, core::Fixed y);
    void setSpeedX(core::Fixed vx) { s_.vx = vx; }
    void setStartDelay(core::Word frames) { s_.startDelay = frames; }
    void holdDrops(const data::DropList& drops) { s_.table = EnemyTable::drops({&drops, 0}); }

    // ------------------------------------------------------------ the state machine

    void update(Level& level);
    /** Into `next` from the next frame on (see Passenger::changeState). */
    void changeState(const enemies::EnemyState& next, Level& level);
    /** Into `next` and on in it in this frame (see Passenger::continueIn). */
    void continueIn(const enemies::EnemyState& next, Level& level);

    // ------------------------------------------------------------ where it is

    int pad() const { return s_.pad; }
    core::Fixed x() const { return s_.x; }
    core::Fixed y() const { return s_.y; }
    core::Fixed speedX() const { return s_.vx; }
    core::Word startDelay() const { return s_.startDelay; }
    void moveToX(core::Fixed x) { s_.x = x; }
    void moveToY(core::Fixed y) { s_.y = y; }

    /** Its speed points left (negative) or right. */
    void headLeft();
    void headRight();
    void reverse() { s_.vx = -s_.vx; }

    /** 113b:2681 - a walker turns to the copter and runs towards it. */
    void turnTo(const Copter& copter);

    EnemyFacing& facing() { return s_.facing; }
    const EnemyFacing& facing() const { return s_.facing; }
    EnemyTimer& timer() { return s_.timer; }
    EnemyTable& table() { return s_.table; }
    void setTable(const EnemyTable& table) { s_.table = table; }

    // ------------------------------------------------------------ what it shows

    void restartAnimation() { s_.animator.restart(); }
    /** One frame of the animation delay; true when the next frame is due (the delay starts again at `delay`). */
    bool animate(core::Word delay) { return s_.animator.step(delay); }
    /** Shows the current frame of `animation`, from its start again past the end. */
    void show(const data::Animation& animation) { s_.sprite = s_.animator.show(animation); }
    /** Shows the current frame of the variant of `pair` it faces. */
    void showFacing(const data::AnimationPair& pair) { show(pair.towards(s_.facing.right())); }
    /** Shows the current frame of `animation` as it is (an animation that runs once). */
    void showFrameOf(const data::Animation& animation) { s_.sprite = s_.animator.frameOf(animation); }
    bool pastEndOf(const data::Animation& animation) const { return s_.animator.pastEndOf(animation); }
    core::Word animationFrame() const { return s_.animator.frame(); }
    void showSprite(data::Sprite sprite) { s_.sprite = sprite; }
    void hide() { s_.sprite = data::NO_SPRITE; }

    const Snapshot& snapshot() const { return s_; }
    void restore(const Snapshot& snapshot) { s_ = snapshot; }

private:
    int index_ = 0;
    Snapshot s_;
};

}  // namespace ugh::model

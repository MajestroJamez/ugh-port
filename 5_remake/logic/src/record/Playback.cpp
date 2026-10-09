#include "record/Playback.hpp"

namespace ugh::record {

bool Playback::start(game::Game& game) {
    next_ = 0;
    given_ = 0;
    return game.resume(recording_.start);
}

/** Before step n come the inputs that came after step n - 1 (none before the first). */
size_t Playback::dueEnd() const {
    size_t end = given_;
    while (end < recording_.inputs.size() && recording_.inputs[end].after < next_) end++;
    return end;
}

std::span<const Input> Playback::due() const {
    return std::span<const Input>(recording_.inputs).subspan(given_, dueEnd() - given_);
}

void Playback::stepped() {
    given_ = dueEnd();
    next_++;
}

}  // namespace ugh::record

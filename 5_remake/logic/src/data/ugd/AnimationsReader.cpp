#include "data/ugd/AnimationsReader.hpp"

#include <memory>
#include <vector>

namespace ugh::data::ugd {

bool AnimationsReader::readAnimation(const UgdRecord& r) {
    std::vector<int> frames;
    if (!in_.only({"frames"}) || !in_.numbers("frames", 0, frames)) return false;
    if (r.name.empty() || frames.empty()) return in_.fail("an animation without a name or frames");
    if (byName_.count(r.name)) return in_.fail("animation " + r.name + " twice");
    data_.animations.push_back(std::make_unique<kinds::Animation>(kinds::Animation{r.name, frames}));
    byName_[r.name] = data_.animations.back().get();
    return true;
}

bool AnimationsReader::animation(const char* key, const kinds::Animation*& out) {
    std::string name;
    return in_.text(key, name) && find(name, out);
}

bool AnimationsReader::pair(const char* key, kinds::AnimationPair& out) {
    std::string value;
    if (!in_.text(key, value)) return false;
    std::vector<std::string> names = RecordReader::split(value, ',');
    if (names.size() != 2) return in_.fail(std::string(key) + " needs two animations");
    return find(names[0], out.left) && find(names[1], out.right);
}

bool AnimationsReader::find(const std::string& name, const kinds::Animation*& out) {
    auto it = byName_.find(name);
    if (it == byName_.end()) return in_.fail("no animation " + name);
    out = it->second;
    return true;
}

}  // namespace ugh::data::ugd

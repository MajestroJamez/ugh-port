#include "data/ugd/LevelReader.hpp"

#include <charconv>

namespace ugh::data::ugd {

namespace {

using units::Fixed;

constexpr const char* LEVEL = "level";
constexpr const char* ORDER = "order";
constexpr const char* PAD = "pad";
constexpr const char* MASK = "mask";
constexpr int MASK_ROW_DIGITS = levels::CollisionMask::WIDTH / 4;   // hex digits, 4 pixels each

bool isPart(const std::string& type) { return type == PAD || type == MASK || PlacementReader::reads(type); }

}  // namespace

bool LevelReader::reads(const std::string& type) { return type == LEVEL || type == ORDER || isPart(type); }

bool LevelReader::readAll(const std::vector<UgdRecord>& records) {
    for (const UgdRecord& r : records) {
        in_.at(&r);
        if (isPart(r.type)) {
            if (!level_) return in_.fail("outside a level");
            if (!readPart(r)) return false;
        } else if (r.type == LEVEL || r.type == ORDER) {
            if (level_ && !finishLevel()) return false;
            in_.at(&r);
            if (r.type == LEVEL ? !startLevel(r) : !readOrder(r)) return false;
        }
    }
    return !level_ || finishLevel();
}

bool LevelReader::startLevel(const UgdRecord& r) {
    level_ = std::make_unique<levels::LevelDefinition>();
    levelRecord_ = &r;
    mask_.clear();
    maskRows_ = 0;
    levels::LevelDefinition& level = *level_;
    std::string wind;
    std::vector<int> start0, start1;
    int water = 0, waterSpeed = 0;
    if (!RecordReader::parseInt(r.name, level.id)) return in_.fail("a level without a number");
    if (byId_.count(level.id)) return in_.fail("level " + r.name + " twice");
    if (!in_.only({"toDeliver", "wind", "start0", "start1", "water", "waterSpeed"}) ||
        !in_.number("toDeliver", level.toDeliver) || !in_.text("wind", wind) || !in_.numbers("start0", 2, start0) ||
        !in_.numbers("start1", 2, start1) || !in_.number("water", water) || !in_.number("waterSpeed", waterSpeed))
        return false;
    if (wind == "none") level.wind = levels::Wind::None;
    else if (wind == "left") level.wind = levels::Wind::Left;
    else if (wind == "right") level.wind = levels::Wind::Right;
    else return in_.fail("unknown wind " + wind);
    level.startX = {Fixed::fromRaw(start0[0]), Fixed::fromRaw(start1[0])};
    level.startY = {Fixed::fromRaw(start0[1]), Fixed::fromRaw(start1[1])};
    level.water = Fixed::fromRaw(water);
    level.waterSpeed = Fixed::fromRaw(waterSpeed);
    return true;
}

bool LevelReader::readPart(const UgdRecord& r) {
    if (r.type == PAD) return readPad();
    if (r.type == MASK) return readMaskRow(r);
    return placements_.read(r, *level_);
}

bool LevelReader::finishLevel() {
    in_.at(levelRecord_);
    if (maskRows_ != levels::CollisionMask::HEIGHT)
        return in_.fail("level " + levelRecord_->name + ": " + std::to_string(maskRows_) + " mask rows");
    level_->mask = levels::CollisionMask{std::move(mask_)};
    byId_[level_->id] = level_.get();
    data_.levelDefinitions.push_back(std::move(level_));
    return true;
}

bool LevelReader::readOrder(const UgdRecord& r) {
    for (int mode = 0; mode < 2; mode++) {
        const char* key = mode == 0 ? "oneplayer" : "team";
        if (!r.fields.count(key)) continue;
        if (!in_.only({key})) return false;
        if (!data_.order[mode].empty()) return in_.fail(std::string("a second order of ") + key);
        std::vector<int> ids;
        if (!in_.numbers(key, 0, ids)) return false;
        for (int id : ids) {
            auto it = byId_.find(id);
            if (it == byId_.end()) return in_.fail("no level " + std::to_string(id));
            data_.order[mode].push_back(it->second);
        }
        return true;
    }
    return in_.fail("an order of no mode");
}

bool LevelReader::readPad() {
    levels::PadDefinition p;
    if (!in_.only({"left", "right", "y", "door", "wait", "stand", "number"}) || !in_.number("left", p.left) ||
        !in_.number("right", p.right) || !in_.number("y", p.y) || !in_.number("door", p.door) ||
        !in_.number("wait", p.wait) || !in_.number("stand", p.stand) || !in_.number("number", p.number))
        return false;
    level_->pads.push_back(p);
    return true;
}

/** `mask <80 hex digits>`: a row of the collision mask, the leftmost pixel in the highest bit. */
bool LevelReader::readMaskRow(const UgdRecord& r) {
    if (!r.fields.empty() || static_cast<int>(r.name.size()) != MASK_ROW_DIGITS ||
        maskRows_ >= levels::CollisionMask::HEIGHT)
        return in_.fail("a bad mask row");
    for (int d = 0; d < MASK_ROW_DIGITS; d += 2) {
        unsigned value = 0;
        auto [end, err] = std::from_chars(r.name.data() + d, r.name.data() + d + 2, value, 16);
        if (err != std::errc() || end != r.name.data() + d + 2) return in_.fail("a bad mask row");
        mask_.push_back(static_cast<uint8_t>(value));
    }
    maskRows_++;
    return true;
}

}  // namespace ugh::data::ugd

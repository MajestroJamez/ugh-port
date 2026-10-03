#include "TestLevel.hpp"

#include "TestData.hpp"

namespace ugh::test {

const data::GameData& TestLevel::gameData() { return test::gameData(); }

TestLevel::TestLevel(int waterRow, data::levels::Wind wind)
    : mask_(data::levels::CollisionMask::WIDTH / 8 * data::levels::CollisionMask::HEIGHT),
      session_(gameData().rules(), 1, data::Difficulty::Medium, 0, world::RandomNumbers({1, 2, 3, 4})) {
    definition_.wind = wind;
    definition_.water = units::Fixed::fromPixels(waterRow);
    definition_.startX = {units::Fixed::fromPixels(140), units::Fixed::fromPixels(40)};
    definition_.startY = {units::Fixed::fromPixels(100), units::Fixed::fromPixels(40)};
    definition_.pads.push_back({100, 200, 150, 110, 120, 130, 1});
    definition_.toDeliver = 1;
}

void TestLevel::solid(int x, int y) {
    mask_[y * (data::levels::CollisionMask::WIDTH / 8) + x / 8] |= static_cast<uint8_t>(0x80 >> (x % 8));
}

void TestLevel::start() {
    definition_.mask = data::levels::CollisionMask{mask_};
    level_.startAttempt(definition_, gameData().sprites(), session_.random(), diagnostics);
}

}  // namespace ugh::test

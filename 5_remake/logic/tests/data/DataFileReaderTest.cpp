#include <string>

#include "TestData.hpp"
#include "TestFramework.hpp"
#include "data/DataFileReader.hpp"
#include "data/FlyerPlacement.hpp"
#include "data/RoutePassengerPlacement.hpp"
#include "data/StandingPassengerPlacement.hpp"
#include "data/TreePlacement.hpp"

using namespace ugh;

namespace {

/** Counts the passenger placements of a level by type. */
class PassengerCounter : public data::PassengerPlacementVisitor {
public:
    int route = 0, standing = 0;
    const data::RoutePassengerPlacement* first = nullptr;
    void visit(const data::RoutePassengerPlacement& p) override {
        if (!first) first = &p;
        route++;
    }
    void visit(const data::StandingPassengerPlacement&) override { standing++; }
};

std::string readError(const std::string& text) {
    std::string error;
    auto data = data::DataFileReader::parse(text, error);
    return data ? "" : error;
}

const char* const VALID = "UGD 1\n"
                          "rules crashLimit=1,2,3 multiplierLimit=3,9,99 quickDeliveryBonus=m\n"
                          "sprites standingPassenger=1 droppedPassenger=2 bouncedPassenger=3 shakenTree=4 "
                          "destinationBubbles=5..6 impatientBubble=7 rotor0=8..9 rotor1=10..11\n"
                          "bonusKind m effect=multiplier amount=0 lift=1 sprite=2 anchor=8,16\n";

}  // namespace

TEST(the_real_data_has_all_levels) {
    const data::GameData& d = test::gameData();
    CHECK_EQUAL(69, d.levelCount(1));
    CHECK_EQUAL(81, d.levelCount(2));
    CHECK(d.level(1, 69) == nullptr);
    CHECK_EQUAL(3100, d.rules().crashLimit(data::Difficulty::Easy));
    CHECK_EQUAL(99, d.rules().multiplierLimit(data::Difficulty::Hard));
    CHECK_EQUAL(218, d.sprites().firstRotor[0]);
    CHECK_EQUAL(223, d.sprites().lastRotor[0]);
}

TEST(the_real_data_has_level_one) {
    const data::LevelDefinition& level = *test::gameData().level(1, 0);
    CHECK_EQUAL(3, static_cast<int>(level.pads.size()));
    CHECK_EQUAL(4608, level.startX[0].raw());
    CHECK_EQUAL(5520, level.water.raw());
    PassengerCounter passengers;
    for (const auto& p : level.passengers) p->accept(passengers);
    CHECK_EQUAL(2, passengers.route);
    CHECK_EQUAL(1, passengers.standing);
    CHECK_EQUAL(size_t{3}, passengers.first->route.stops.size());
    CHECK_EQUAL(150, passengers.first->route.stops[0].delay);
    CHECK_EQUAL(2, passengers.first->route.stops[0].targetPad);
    CHECK(!level.mask.solid(-1, 0));
    CHECK(!level.mask.solid(320, 100));
}

TEST(a_minimal_file_lacks_the_level_orders) {
    CHECK_EQUAL(std::string("a level order missing"), readError(VALID));
}

TEST(extra_spaces_are_allowed_and_an_order_only_once) {
    CHECK_EQUAL(std::string("a level order missing"), readError(std::string(VALID) + "animation  a   frames=1  \n"));
    std::string level = "level 0 toDeliver=1 wind=none start0=0,0 start1=0,0 water=0 waterSpeed=0\n";
    for (int row = 0; row < 192; row++) level += "mask " + std::string(80, '0') + "\n";
    CHECK_EQUAL(std::string("line 199 (order): a second order of oneplayer"),
                readError(std::string(VALID) + level + "order oneplayer=0\norder oneplayer=0\n"));
}

TEST(broken_files_are_refused_with_the_line) {
    CHECK_EQUAL(std::string("not a UGD 1 file"), readError("UGD 2\n"));
    CHECK_EQUAL(std::string("line 2 (bogus): unknown record"), readError("UGD 1\nbogus x=1\n"));
    CHECK_EQUAL(std::string("line 5 (animation): unknown key color"),
                readError(std::string(VALID) + "animation a frames=1 color=2\n"));
    CHECK_EQUAL(std::string("line 2 (rules): no bonus kind x"),
                readError("UGD 1\nrules crashLimit=1,2,3 multiplierLimit=3,9,99 quickDeliveryBonus=x\n"));
    CHECK_EQUAL(std::string("line 6 (walker): no pad 3"),
                readError(std::string(VALID) +
                          "level 0 toDeliver=1 wind=none start0=0,0 start1=0,0 water=0 waterSpeed=0\n"
                          "walker pad=3 x=0 y=0 speed=0\n"));
    CHECK_EQUAL(std::string("line 5 (level): level 0: 0 mask rows"),
                readError(std::string(VALID) +
                          "level 0 toDeliver=1 wind=none start0=0,0 start1=0,0 water=0 waterSpeed=0\n"));
}

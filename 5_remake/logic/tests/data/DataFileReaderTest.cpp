#include <string>

#include "TestData.hpp"
#include "TestFramework.hpp"
#include "data/levels/FlyerPlacement.hpp"
#include "data/levels/RoutePassengerPlacement.hpp"
#include "data/levels/StandingPassengerPlacement.hpp"
#include "data/levels/TreePlacement.hpp"
#include "data/ugd/DataFileReader.hpp"

using namespace ugh;

namespace {

/** Counts the passenger placements of a level by type. */
class PassengerCounter : public data::levels::PassengerPlacementVisitor {
public:
    int route = 0, standing = 0;
    const data::levels::RoutePassengerPlacement* first = nullptr;
    void visit(const data::levels::RoutePassengerPlacement& p) override {
        if (!first) first = &p;
        route++;
    }
    void visit(const data::levels::StandingPassengerPlacement&) override { standing++; }
};

std::string readError(const std::string& text) {
    std::string error;
    auto data = data::ugd::DataFileReader::parse(text, error);
    return data ? "" : error;
}

// the kinds of the enemies, each once (5 lines)
const std::string ENEMY_KINDS = "animation e frames=1\n"
                                "flyerKind box=1,1,1,1 flight=e,e hitSprite=1,2 score=1\n"
                                "walkerKind box=1,1,1,1 walk=e,e watch=e,e charge=e,e recover=e,e stunned=e,e score=1\n"
                                "blowerKind box=1,1,1,1 blowing=e stunnedSprite=1 score=1\n"
                                "treeKind swaying=e\n";

// a file without levels (9 lines)
const std::string VALID = "UGD 1\n"
                          "rules crashLimit=1,2,3 multiplierLimit=3,9,99 quickDeliveryBonus=m\n"
                          "sprites standingPassenger=1 droppedPassenger=2 bouncedPassenger=3 shakenTree=4 "
                          "destinationBubbles=5..6 impatientBubble=7 rotor0=8..9 rotor1=10..11\n"
                          "bonusKind m effect=multiplier amount=0 lift=1 sprite=2 anchor=8,16\n" +
                          ENEMY_KINDS;

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
    const data::levels::LevelDefinition& level = *test::gameData().level(1, 0);
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
    CHECK_EQUAL(std::string("a level order missing"), readError(VALID + "animation  a   frames=1  \n"));
    std::string level = "level 0 toDeliver=1 wind=none start0=0,0 start1=0,0 water=0 waterSpeed=0\n";
    for (int row = 0; row < 192; row++) level += "mask " + std::string(80, '0') + "\n";
    CHECK_EQUAL(std::string("line 204 (order): a second order of oneplayer"),
                readError(VALID + level + "order oneplayer=0\norder oneplayer=0\n"));
}

TEST(broken_files_are_refused_with_the_line) {
    CHECK_EQUAL(std::string("not a UGD 1 file"), readError("UGD 2\n"));
    CHECK_EQUAL(std::string("line 2 (bogus): unknown record"), readError("UGD 1\nbogus x=1\n"));
    CHECK_EQUAL(std::string("line 10 (animation): unknown key color"),
                readError(VALID + "animation a frames=1 color=2\n"));
    CHECK_EQUAL(std::string("line 7 (rules): no bonus kind x"),
                readError("UGD 1\n" + ENEMY_KINDS +
                          "rules crashLimit=1,2,3 multiplierLimit=3,9,99 quickDeliveryBonus=x\n"));
    CHECK_EQUAL(std::string("line 11 (walker): no pad 3"),
                readError(VALID +
                          "level 0 toDeliver=1 wind=none start0=0,0 start1=0,0 water=0 waterSpeed=0\n"
                          "walker pad=3 x=0 y=0 speed=0\n"));
    CHECK_EQUAL(std::string("line 10 (level): level 0: 0 mask rows"),
                readError(VALID +
                          "level 0 toDeliver=1 wind=none start0=0,0 start1=0,0 water=0 waterSpeed=0\n"));
}

TEST(the_kinds_of_the_enemies_the_rules_and_the_sprites_are_there_once) {
    std::string withoutTree = ENEMY_KINDS.substr(0, ENEMY_KINDS.rfind("treeKind"));
    CHECK_EQUAL(std::string("no treeKind"), readError("UGD 1\n" + withoutTree));
    CHECK_EQUAL(std::string("line 10 (flyerKind): a second flyerKind"),
                readError(VALID + "flyerKind box=1,1,1,1 flight=e,e hitSprite=1,2 score=1\n"));
    CHECK_EQUAL(std::string("line 10 (rules): a second rules record"),
                readError(VALID + "rules crashLimit=1,2,3 multiplierLimit=3,9,99 quickDeliveryBonus=m\n"));
    CHECK_EQUAL(std::string("line 10 (sprites): a second sprites record"),
                readError(VALID + "sprites standingPassenger=1 droppedPassenger=2 bouncedPassenger=3 shakenTree=4 "
                                  "destinationBubbles=5..6 impatientBubble=7 rotor0=8..9 rotor1=10..11\n"));
}

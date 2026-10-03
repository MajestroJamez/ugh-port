// N1 audit (branch audit/n1 only): which UGR 0 fields a state defines (hypothesis of rewrite-design.md, chap. 9).
#pragma once

#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace ugh::tool::audit {

struct Rule {
    const char* group;   // game, copter, passenger, object, bonus
    const char* kind;    // object kind (flyer ...), or "*"
    const char* states;  // space separated states, or "*"
    const char* phases;  // space separated phases, or "*"
    const char* fields;  // space separated UGR 0 field names
};

inline const std::vector<Rule>& rules() {
    static const std::vector<Rule> r = {
        // game
        {"game", "*", "*", "*", "phase level players difficulty lives multiplier score rng rainFloor"},
        {"game", "*", "*", "caption setup play",
         "energy fade fadeStep levelDone wind passengersLeft waterYf waterHold waterToggle waterAnim waterAnimDelay rain"},
        {"game", "*", "*", "setup play", "waterRow"},
        // copters
        {"copter", "*", "*", "*", "xf yf x y vx vy landedPad sprite animCounter keys carrying targetPad fare"},
        {"copter", "*", "*", "play", "effort impact"},
        // passengers: always
        {"passenger", "*", "*", "*", "kind state route sprite bubble"},
        // route passengers from the first stop
        {"passenger", "*",
         "Arriving Appearing Waiting Calling Impatient Boarding Riding WalkingAway Entering Splash Swimming SwimCalling "
         "SwimWaving SwimBoarding Sinking",
         "*", "pickupPad targetPad"},
        {"passenger", "*", "Arriving", "*", "timer"},
        {"passenger", "*",
         "Appearing Waiting Calling Impatient Boarding WalkingAway Entering Splash Swimming SwimCalling SwimWaving "
         "SwimBoarding Sinking",
         "*", "xf yf x y anim animDelay"},
        {"passenger", "*", "Waiting Calling Impatient SwimCalling SwimWaving", "*", "counter"},
        {"passenger", "*", "Riding", "*", "counter bonusTimer x y"},
        {"passenger", "*", "Splash Sinking", "*", "vy"},
        {"passenger", "*", "Swimming", "*", "timer"},
        {"passenger", "*", "StartStanding Standing", "*", "xf yf"},
        {"passenger", "*", "Hanging", "*", "counter"},
        {"passenger", "*", "Falling", "*", "timer vy xf yf"},
        // enemies
        {"object", "*", "*", "*", "kind state sprite"},
        {"object", "flyer", "*", "*", "vx facing startDelay"},
        {"object", "flyer", "FlyerWait FlyerWait2", "*", "timer anim animDelay"},
        {"object", "flyer", "Flying", "*", "xf yf table anim animDelay"},
        {"object", "flyer", "FlyerFalling", "*", "xf yf timer"},
        {"object", "walker", "*", "*", "pad xf yf vx facing"},
        {"object", "walker", "Walking Watching Charging Recovering Stunned", "*", "anim animDelay"},
        {"object", "walker", "Watching Charging Stunned", "*", "timer"},
        {"object", "blower", "*", "*", "xf yf"},
        {"object", "blower", "Blowing", "*", "anim animDelay"},
        {"object", "blower", "BlowerWait", "*", "timer"},
        {"object", "tree", "*", "*", "xf yf table"},
        {"object", "tree", "Tree TreeWait", "*", "anim animDelay"},
        {"object", "tree", "TreeWait", "*", "timer"},
        // bonus items
        {"bonus", "*", "*", "*", "kind state xf yf sprite"},
        {"bonus", "*", "Falling", "*", "vx vy"},
        {"bonus", "*", "Lying", "*", "vx"},
    };
    return r;
}

inline bool listed(const char* list, const std::string& word) {
    if (std::string(list) == "*") return true;
    std::istringstream in(list);
    std::string w;
    while (in >> w)
        if (w == word) return true;
    return false;
}

/** The fields of an entity that are defined in its state. */
inline std::set<std::string> definedFields(const std::string& group, const std::string& kind, const std::string& state,
                                           const std::string& phase) {
    std::set<std::string> d;
    for (const Rule& rule : rules()) {
        if (group != rule.group || !listed(rule.kind, kind) || !listed(rule.states, state) || !listed(rule.phases, phase))
            continue;
        std::istringstream in(rule.fields);
        std::string w;
        while (in >> w) d.insert(w);
    }
    return d;
}

/** Two poison values (core a, core b) for a field; empty when it cannot be poisoned. */
inline std::pair<std::string, std::string> poison(const std::string& group, const std::string& field) {
    if (field == "kind" || field == "state" || field == "route") return {};
    if (group == "game") {
        if (field == "rain") return {"poison", "poison"};
        if (field == "waterHold" || field == "waterToggle" || field == "waterAnim" || field == "waterAnimDelay" ||
            field == "levelDone" || field == "wind" || field == "passengersLeft")
            return {"0", "1"};
        if (field == "fade" || field == "fadeStep") return {"0", "2"};
        return {"11", "-7"};
    }
    if (field == "sprite" || field == "bubble") {
        if (group == "copter") return {"0x00da", "0x00db"};
        if (group == "bonus") return {};
        return {"none", "0x0010"};
    }
    if (field == "carrying") return {"none", "0x0001"};
    if (field == "keys") return {"-", "UDLRF"};
    if (field == "landedPad") return {"-1", "0"};
    if (field == "pickupPad" || field == "targetPad" || field == "pad") {
        if (group == "copter") return {"0", "7"};
        return {"0", "1"};
    }
    if (field == "anim") return {"2", "4"};
    if (field == "animDelay") return {"3", "5"};
    if (field == "counter" || field == "timer" || field == "startDelay") return {"0", "2"};
    if (field == "facing") return {"0", "1"};
    if (field == "table") return {"0x1111", "0x2222"};
    if (field == "vy" && group == "passenger") return {"3", "-5"};
    if (field == "bonusTimer") return {"0", "5"};
    if (field == "startPad") return {"0", "1"};
    return {"11", "-7"};
}

}  // namespace ugh::tool::audit

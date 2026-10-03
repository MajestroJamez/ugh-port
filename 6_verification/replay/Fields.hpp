// The fields of a replay state.
#pragma once

#include <map>
#include <string>

namespace ugh::replay {

/** Field name -> value as the replays "UGR 1" write them ("copter.0.x" -> "4608"), sorted by name. */
using Fields = std::map<std::string, std::string>;

}  // namespace ugh::replay

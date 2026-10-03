// Where a waiting passenger is.
#pragma once

namespace ugh::passengers::route {

/** A waiting passenger: it just started, it walks to the waiting spot of its pad, or it stands there. */
enum class WaitingSpot { Starting, Walking, Reached };

}  // namespace ugh::passengers::route

// The rain of the windy levels.
#pragma once

namespace ugh::world {

/** The rain of the windy levels. */
class Rain {
public:
    /** The drops start again at this row (the water surface last shown); at a new game it is an input. */
    int floorRow() const { return floorRow_; }
    void setFloorRow(int row) { floorRow_ = row; }

private:
    int floorRow_ = 0;
};

}  // namespace ugh::world

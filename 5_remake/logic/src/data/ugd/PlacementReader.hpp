// Reads who is in a level: its passengers and enemies.
#pragma once

#include <string>
#include <vector>

#include "data/levels/LevelDefinition.hpp"
#include "data/levels/Route.hpp"
#include "data/ugd/KindsReader.hpp"
#include "data/ugd/RecordReader.hpp"
#include "data/ugd/RecordTable.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/** Reads the placements of the passengers and the enemies of a level, in their order; checks the pads they name. */
class PlacementReader {
public:
    PlacementReader(RecordReader& in, const KindsReader& kinds) : in_(in), kinds_(kinds) {}

    /** It reads records of `type`. */
    static bool reads(const std::string& type);
    /** The placement of record `r` into `level` (which has its pads). */
    bool read(const UgdRecord& r, levels::LevelDefinition& level);

private:
    /** The placements by the type of their record. */
    static const RecordTable<PlacementReader>::Entry PLACEMENTS[];

    RecordReader& in_;
    const KindsReader& kinds_;
    levels::LevelDefinition* level_ = nullptr;   // the level being read

    bool readRoutePassenger(const UgdRecord& r);
    bool readStandingPassenger(const UgdRecord& r);
    bool readFlyer(const UgdRecord& r);
    bool readWalker(const UgdRecord& r);
    bool readBlower(const UgdRecord& r);
    bool readTree(const UgdRecord& r);

    /** `route=<pad>/<delay>,...,<pad>`: a stop from each pad to the next one. */
    bool readRoute(std::vector<levels::Route::Stop>& stops);
    /** Pad `index` is one of the level; false (an error) when it is not. */
    bool pad(int index);
};

}  // namespace ugh::data::ugd

// Replay player: checks the C++ core against golden replays (format "UGR 0", see
// verify/src/test/kotlin/ugh/verify/replay/ReplayWriter.kt). The modes are described in ReplayPlayer.hpp, the two
// cores in TwinCores.hpp.
//
//   ugh_replay [--each] <ugh-sim.bin> <replay.ugr> ...
#include <cstdio>
#include <cstring>

#include "ReplayPlayer.hpp"
#include "ReplayReport.hpp"
#include "TwinCores.hpp"

namespace ugh::tool {
void printAuditViolations();
}

int main(int argc, char** argv) {
    using ugh::tool::ReplayPlayer;
    int first = 1;
    ReplayPlayer::Mode mode = ReplayPlayer::Mode::WholeGame;
    if (argc > 1 && std::strcmp(argv[1], "--each") == 0) {
        mode = ReplayPlayer::Mode::EachTransition;
        first = 2;
    }
    if (argc > 1 && std::strcmp(argv[1], "--audit") == 0) {
        mode = ReplayPlayer::Mode::Audit;
        first = 2;
    }
    if (argc < first + 2) {
        std::fprintf(stderr, "usage: ugh_replay [--each] <ugh-sim.bin> <replay.ugr> ...\n");
        return 2;
    }
    ugh::tool::TwinCores cores(argv[first]);
    if (!cores.ok()) {
        std::fprintf(stderr, "%s\n", cores.error().c_str());
        return 2;
    }
    bool ok = true;
    for (int i = first + 1; i < argc; i++) {
        ugh::tool::ReplayReport report;
        if (!ReplayPlayer(cores, report).play(argv[i], mode)) {
            ok = false;
            continue;
        }
        report.print(argv[i]);
        if (!report.passed()) ok = false;
    }
    if (mode == ReplayPlayer::Mode::Audit) ugh::tool::printAuditViolations();
    return ok ? 0 : 1;
}

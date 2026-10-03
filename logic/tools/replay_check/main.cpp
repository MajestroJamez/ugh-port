// replay_check: the logic against golden replays "UGR 1" (verify/build/replays/ugr1).
//
//   replay_check [--continue] <ugh-data.ugd> <replay.ugr> ...
#include <cstdio>
#include <cstring>
#include <string>

#include "ReplayCheck.hpp"
#include "ReplayReport.hpp"
#include "data/DataFileReader.hpp"


int main(int argc, char** argv) {
    ugh::tool::CheckOptions options;
    int i = 1;
    for (; i < argc && std::strncmp(argv[i], "--", 2) == 0; i++) {
        std::string option = argv[i];
        if (option == "--continue") {
            options.continueAfterMismatch = true;
        } else {
            std::fprintf(stderr, "unknown option %s\n", argv[i]);
            return 2;
        }
    }
    if (argc - i < 2) {
        std::fprintf(stderr, "usage: replay_check [--continue] <ugh-data.ugd> <replay.ugr> ...\n");
        return 2;
    }
    std::string error;
    auto data = ugh::data::DataFileReader::read(argv[i], error);
    if (!data) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }
    bool ok = true;
    for (int r = i + 1; r < argc; r++) {
        ugh::tool::ReplayReport report;
        if (!ugh::tool::ReplayCheck(*data, options, report).run(argv[r])) {
            ok = false;
            continue;
        }
        report.print(argv[r]);
        ok = ok && report.passed();
    }
    return ok ? 0 : 1;
}

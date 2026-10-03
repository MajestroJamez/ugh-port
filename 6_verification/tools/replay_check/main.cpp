// replay_check: the logic against golden replays "UGR 1" (4_test_data/verify/build/replays).
//
//   replay_check [--continue] <ugh-data.ugd> <replay.ugr> ...
#include <cstdio>
#include <cstring>
#include <string>

#include "check/ReplayCheck.hpp"
#include "check/ReplayReport.hpp"
#include "data/ugd/DataFileReader.hpp"
#include "keyboard/KeyFile.hpp"

int main(int argc, char** argv) {
    ugh::check::ReplayCheck::Options options;
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
    auto data = ugh::data::ugd::DataFileReader::read(argv[i], error);
    if (!data) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }
    std::vector<ugh::keyboard::KeyBinding> keys;
    if (!ugh::keyboard::KeyFile::read(argv[i], keys, error)) {
        std::fprintf(stderr, "%s\n", error.c_str());
        return 2;
    }
    bool ok = true;
    for (int r = i + 1; r < argc; r++) {
        ugh::check::ReplayReport report;
        ugh::check::ReplayCheck(*data, keys, options, report).run(argv[r]);
        report.print(argv[r]);
        ok = ok && report.passed();
    }
    return ok ? 0 : 1;
}

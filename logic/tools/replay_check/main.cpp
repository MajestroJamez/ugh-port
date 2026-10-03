// replay_check: the logic against golden replays "UGR 1" (verify/build/replays/ugr1).
//
//   replay_check [--continue] [--only game.,copter.] [--skip copter.N.fareMin,...] [--until game.phase=caption]
//                <ugh-data.ugd> <replay.ugr> ...
#include <cstdio>
#include <cstring>
#include <sstream>
#include <string>

#include "ReplayCheck.hpp"
#include "ReplayReport.hpp"
#include "data/DataFileReader.hpp"

namespace {

std::vector<std::string> list(const char* text) {
    std::vector<std::string> items;
    std::istringstream in(text);
    std::string item;
    while (std::getline(in, item, ',')) items.push_back(item);
    return items;
}

}  // namespace

int main(int argc, char** argv) {
    ugh::tool::CheckOptions options;
    int i = 1;
    for (; i < argc && std::strncmp(argv[i], "--", 2) == 0; i++) {
        std::string option = argv[i];
        if (option == "--continue") {
            options.continueAfterMismatch = true;
        } else if (option == "--only" && i + 1 < argc) {
            options.only = list(argv[++i]);
        } else if (option == "--skip" && i + 1 < argc) {
            options.skip = list(argv[++i]);
        } else if (option == "--until" && i + 1 < argc) {
            std::string until = argv[++i];
            size_t at = until.find("!=");
            options.untilNot = at != std::string::npos;
            if (at == std::string::npos) at = until.find('=');
            options.untilField = until.substr(0, at);
            options.untilValue = until.substr(at + (options.untilNot ? 2 : 1));
        } else {
            std::fprintf(stderr, "unknown option %s\n", argv[i]);
            return 2;
        }
    }
    if (argc - i < 2) {
        std::fprintf(stderr, "usage: replay_check [--continue] [--only <prefixes>] [--skip <fields>] "
                             "[--until <field>=<value>] <ugh-data.ugd> <replay.ugr> ...\n");
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

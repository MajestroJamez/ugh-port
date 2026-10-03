#include "ReplayFile.hpp"

#include <cstdlib>
#include <sstream>

namespace ugh::tool {

namespace {

/** Empty lines, comments and the meta data. */
bool ignored(const std::string& line) { return line.empty() || line[0] == '#' || startsWith(line, "meta "); }

}  // namespace

ReplayFile::ReplayFile(const std::string& path) : path_(path), in_(path) {}

bool ReplayFile::open() {
    if (!in_) {
        error_ = "cannot open " + path_;
        return false;
    }
    std::string line;
    if (!std::getline(in_, line) || line != "UGR 0") {
        error_ = path_ + ": not a UGR 0 file";
        return false;
    }
    return true;
}

bool ReplayFile::next(Tick& tick) {
    std::string line;
    // the first T line; the later ones are read at the end of the tick before them
    while (pending_.empty() && std::getline(in_, line)) {
        if (startsWith(line, "T ")) {
            pending_ = line;
        } else if (!ignored(line)) {
            error_ = path_ + ": unexpected line: " + line;
            return false;
        }
    }
    if (pending_.empty()) return false;

    tick = Tick{};
    readHead(pending_, tick);
    pending_.clear();
    while (std::getline(in_, line)) {
        if (ignored(line)) continue;
        if (startsWith(line, "T ")) {
            pending_ = line;
            break;
        }
        if (startsWith(line, "B ")) {
            size_t space = line.find(' ', 2);
            tick.before.emplace_back(line.substr(2, space - 2),
                                     pairs(space == std::string::npos ? "" : line.substr(space + 1)));
        } else if (startsWith(line, "I ")) {
            tick.inject = pairs(line.substr(2));
        } else {
            error_ = path_ + ": unexpected line: " + line;
            return false;
        }
    }
    return true;
}

/** "T <number> k=<hex scancodes, comma-separated, or -> | <name=value ...>" */
void ReplayFile::readHead(const std::string& line, Tick& tick) {
    size_t bar = line.find(" |");
    std::istringstream head(line.substr(2, bar == std::string::npos ? std::string::npos : bar - 2));
    head >> tick.number;
    std::string item;
    while (head >> item) {
        if (!startsWith(item, "k=") || item == "k=-") continue;
        std::istringstream codes(item.substr(2));
        std::string code;
        while (std::getline(codes, code, ','))
            tick.keys.push_back(static_cast<int>(std::strtol(code.c_str(), nullptr, 16)));
    }
    if (bar != std::string::npos) {
        for (const auto& [name, value] : pairs(line.substr(bar + 2))) {
            if (value == "~") state_.erase(name);
            else state_[name] = value;
        }
    }
    tick.state = state_;
}

Fields ReplayFile::pairs(const std::string& text) {
    Fields fields;
    std::istringstream in(text);
    std::string item;
    while (in >> item) {
        size_t eq = item.find('=');
        if (eq != std::string::npos) fields[item.substr(0, eq)] = item.substr(eq + 1);
    }
    return fields;
}

}  // namespace ugh::tool

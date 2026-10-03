#include "ReplayFile.hpp"

#include <sstream>

namespace ugh::tool {

ReplayFile::ReplayFile(const std::string& path) : path_(path), in_(path, std::ios::binary) {}

bool ReplayFile::open() {
    std::string header;
    if (!in_ || !std::getline(in_, header)) {
        error_ = "cannot read " + path_;
        return false;
    }
    if (!header.empty() && header.back() == '\r') header.pop_back();
    if (header != "UGR 1") {
        error_ = path_ + ": not a UGR 1 replay";
        return false;
    }
    line_ = 1;
    return true;
}

bool ReplayFile::next(Tick& tick) {
    tick = Tick();
    std::string line;
    bool haveTick = false;
    if (!pending_.empty()) {
        if (!readTickLine(pending_, tick)) return false;
        pending_.clear();
        haveTick = true;
    }
    while (std::getline(in_, line)) {
        line_++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty() || line[0] == '#' || line.rfind("meta ", 0) == 0) continue;
        if (line.rfind("T ", 0) == 0) {
            if (haveTick) {
                pending_ = line;
                return true;
            }
            if (!readTickLine(line, tick)) return false;
            haveTick = true;
        } else if (line.rfind("I ", 0) == 0 && haveTick) {
            tick.inject = pairs(line.substr(2));
        } else {
            error_ = path_ + ":" + std::to_string(line_) + ": unexpected line";
            return false;
        }
    }
    return haveTick;
}

bool ReplayFile::readTickLine(const std::string& line, Tick& tick) {
    size_t bar = line.find(" |");
    if (bar == std::string::npos) {
        error_ = path_ + ":" + std::to_string(line_) + ": a T line without |";
        return false;
    }
    std::istringstream head(line.substr(2, bar - 2));
    std::string keys;
    head >> tick.number >> keys;
    if (keys.rfind("k=", 0) != 0) {
        error_ = path_ + ":" + std::to_string(line_) + ": a T line without k=";
        return false;
    }
    keys = keys.substr(2);
    if (keys != "-") {
        std::istringstream codes(keys);
        std::string code;
        while (std::getline(codes, code, ',')) tick.scancodes.push_back(std::stoi(code, nullptr, 16));
    }
    for (const auto& [name, value] : pairs(line.substr(bar + 2))) {
        if (value == "~") state_.erase(name);
        else state_[name] = value;
    }
    tick.state = state_;
    return true;
}

replay::Fields ReplayFile::pairs(const std::string& text) {
    replay::Fields fields;
    std::istringstream in(text);
    std::string word;
    while (in >> word) {
        size_t eq = word.find('=');
        if (eq != std::string::npos) fields[word.substr(0, eq)] = word.substr(eq + 1);
    }
    return fields;
}

}  // namespace ugh::tool

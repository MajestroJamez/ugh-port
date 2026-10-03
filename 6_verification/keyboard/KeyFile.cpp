#include "keyboard/KeyFile.hpp"

#include <charconv>
#include <fstream>
#include <map>
#include <sstream>

namespace ugh::keyboard {

namespace {

bool parseInt(const std::string& text, int& out) {
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
    return ec == std::errc() && end == text.data() + text.size() && !text.empty();
}

/** One `key` record: `key codes=<a>[,<b>] key=none` or `key codes=... player=<p> key=<up|down|left|right|fire> press=<0|1>`. */
bool parseKey(const std::map<std::string, std::string>& fields, KeyBinding& binding) {
    auto field = [&](const char* name) -> const std::string* {
        auto it = fields.find(name);
        return it == fields.end() ? nullptr : &it->second;
    };
    const std::string* codes = field("codes");
    const std::string* key = field("key");
    if (!codes || !key) return false;
    std::stringstream list(*codes);
    std::string code;
    while (std::getline(list, code, ',')) {
        int value = 0;
        if (!parseInt(code, value) || value < 0 || value > 255) return false;
        binding.scancodes.push_back(static_cast<uint8_t>(value));
    }
    if (binding.scancodes.empty() || binding.scancodes.size() > 2) return false;
    if (*key == "none") return true;
    static const char* const KEYS[] = {"up", "down", "left", "right", "fire"};
    const std::string* player = field("player");
    const std::string* press = field("press");
    KeyBinding::Action action;
    int pressed = 0, k = 0;
    while (k < 5 && *key != KEYS[k]) k++;
    if (k == 5 || !player || !press || !parseInt(*player, action.player) || !parseInt(*press, pressed) ||
        (action.player != 0 && action.player != 1))
        return false;
    action.key = static_cast<data::PlayerKey>(k);
    action.press = pressed != 0;
    binding.action = action;
    return true;
}

}  // namespace

bool KeyFile::read(const std::string& path, std::vector<KeyBinding>& out, std::string& error) {
    std::ifstream in(path);
    if (!in) {
        error = "cannot read " + path;
        return false;
    }
    std::string line;
    for (int number = 1; std::getline(in, line); number++) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.rfind("key ", 0) != 0) continue;
        std::stringstream words(line.substr(4));
        std::map<std::string, std::string> fields;
        std::string word;
        while (words >> word) {
            size_t equals = word.find('=');
            if (equals != std::string::npos) fields[word.substr(0, equals)] = word.substr(equals + 1);
        }
        KeyBinding binding;
        if (!parseKey(fields, binding)) {
            error = path + ":" + std::to_string(number) + ": a bad key";
            return false;
        }
        out.push_back(binding);
    }
    if (out.empty()) {
        error = path + ": no keys";
        return false;
    }
    return true;
}

}  // namespace ugh::keyboard

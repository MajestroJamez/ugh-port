#include "keyboard/KeyFile.hpp"

#include <fstream>
#include <iterator>

#include "data/RecordReader.hpp"
#include "data/UgdTokenizer.hpp"

namespace ugh::keyboard {

namespace {

/** One `key` record: `key codes=<a>[,<b>] key=none` or `key codes=... player=<p> key=<up|down|...> press=<0|1>`. */
bool readKey(const data::UgdRecord& r, data::RecordReader& in, KeyBinding& binding) {
    std::vector<int> codes;
    std::string key;
    if (!in.numbers(r, "codes", 0, codes) || !in.text(r, "key", key)) return false;
    if (codes.empty() || codes.size() > 2) return in.fail("a key of " + std::to_string(codes.size()) + " scancodes");
    for (int code : codes) {
        if (code < 0 || code > 255) return in.fail("a scancode out of range");
        binding.scancodes.push_back(static_cast<uint8_t>(code));
    }
    if (key == "none") return in.only(r, {"codes", "key"});
    static const char* const KEYS[] = {"up", "down", "left", "right", "fire"};
    int k = 0;
    while (k < 5 && key != KEYS[k]) k++;
    if (k == 5) return in.fail("unknown key " + key);
    KeyBinding::Action action;
    int press = 0;
    if (!in.only(r, {"codes", "key", "player", "press"}) || !in.number(r, "player", action.player) ||
        !in.number(r, "press", press))
        return false;
    if (action.player != 0 && action.player != 1) return in.fail("a key of player " + std::to_string(action.player));
    action.key = static_cast<data::PlayerKey>(k);
    action.press = press != 0;
    binding.action = action;
    return true;
}

}  // namespace

bool KeyFile::read(const std::string& path, std::vector<KeyBinding>& out, std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "cannot open " + path;
        return false;
    }
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<data::UgdRecord> records;
    if (!data::UgdTokenizer::tokenize(text, records, error)) {
        error = path + ": " + error;
        return false;
    }
    data::RecordReader in;
    for (const data::UgdRecord& r : records) {
        if (r.type != "key") continue;
        in.at(&r);
        KeyBinding binding;
        if (!readKey(r, in, binding)) {
            error = path + ": " + in.error();
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

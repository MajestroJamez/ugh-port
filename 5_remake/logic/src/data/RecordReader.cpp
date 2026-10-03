#include "data/RecordReader.hpp"

#include <charconv>

namespace ugh::data {

bool RecordReader::fail(const std::string& what) {
    error_ = current_ ? "line " + std::to_string(current_->line) + " (" + current_->type + "): " + what : what;
    return false;
}

bool RecordReader::only(const UgdRecord& r, std::initializer_list<const char*> keys) {
    for (const auto& [key, value] : r.fields) {
        bool known = false;
        for (const char* k : keys) known = known || key == k;
        if (!known) return fail("unknown key " + key);
    }
    return true;
}

bool RecordReader::text(const UgdRecord& r, const char* key, std::string& out) {
    auto it = r.fields.find(key);
    if (it == r.fields.end()) return fail(std::string("no ") + key);
    out = it->second;
    return true;
}

bool RecordReader::number(const UgdRecord& r, const char* key, int& out) {
    std::string value;
    if (!text(r, key, value)) return false;
    return parseInt(value, out) || fail(std::string("bad number ") + key + "=" + value);
}

bool RecordReader::numbers(const UgdRecord& r, const char* key, size_t count, std::vector<int>& out) {
    std::string value;
    if (!text(r, key, value)) return false;
    out.clear();
    for (const std::string& part : split(value, ',')) {
        int v = 0;
        if (!parseInt(part, v)) return fail(std::string("bad numbers ") + key + "=" + value);
        out.push_back(v);
    }
    if (count != 0 && out.size() != count) return fail(std::string(key) + " needs " + std::to_string(count) + " numbers");
    return true;
}

bool RecordReader::range(const UgdRecord& r, const char* key, int& first, int& last) {
    std::string value;
    if (!text(r, key, value)) return false;
    size_t dots = value.find("..");
    if (dots == std::string::npos || !parseInt(value.substr(0, dots), first) || !parseInt(value.substr(dots + 2), last) ||
        last < first)
        return fail(std::string("bad range ") + key + "=" + value);
    return true;
}

bool RecordReader::box(const UgdRecord& r, Box& out) {
    std::vector<int> v;
    if (!numbers(r, "box", 4, v)) return false;
    out = {v[0], v[1], v[2], v[3]};
    return true;
}

bool RecordReader::parseInt(const std::string& text, int& out) {
    auto [end, err] = std::from_chars(text.data(), text.data() + text.size(), out);
    return err == std::errc() && end == text.data() + text.size() && !text.empty();
}

std::vector<std::string> RecordReader::split(const std::string& text, char separator) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        size_t at = text.find(separator, start);
        parts.push_back(text.substr(start, at - start));
        if (at == std::string::npos) return parts;
        start = at + 1;
    }
}

}  // namespace ugh::data

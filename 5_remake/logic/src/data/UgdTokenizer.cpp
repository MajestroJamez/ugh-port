#include "data/UgdTokenizer.hpp"

#include <fstream>
#include <iterator>
#include <sstream>

#include "data/RecordReader.hpp"

namespace ugh::data {

namespace {

constexpr std::string_view HEADER = "UGD 1";

}  // namespace

bool UgdTokenizer::tokenize(std::string_view text, std::vector<UgdRecord>& out, std::string& error) {
    std::istringstream in{std::string(text)};
    std::string line;
    int number = 0;
    bool header = false;
    while (std::getline(in, line)) {
        number++;
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (!header) {
            if (line != HEADER) {
                error = "not a UGD 1 file";
                return false;
            }
            header = true;
            continue;
        }
        if (line.empty() || line[0] == '#') continue;
        UgdRecord r;
        r.line = number;
        std::vector<std::string> words = RecordReader::split(line, ' ');
        r.type = words[0];
        bool firstWord = true;
        for (size_t w = 1; w < words.size(); w++) {
            if (words[w].empty()) continue;   // more spaces than one
            size_t eq = words[w].find('=');
            bool name = firstWord;
            firstWord = false;
            if (eq == std::string::npos) {
                if (!name) {
                    error = "line " + std::to_string(number) + ": a word without a value: " + words[w];
                    return false;
                }
                r.name = words[w];
            } else {
                r.fields[words[w].substr(0, eq)] = words[w].substr(eq + 1);
            }
        }
        out.push_back(std::move(r));
    }
    if (!header) {
        error = "empty file";
        return false;
    }
    return true;
}

bool UgdTokenizer::tokenizeFile(const std::string& path, std::vector<UgdRecord>& out, std::string& error) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        error = "cannot open " + path;
        return false;
    }
    std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    if (tokenize(text, out, error)) return true;
    error = path + ": " + error;
    return false;
}

}  // namespace ugh::data

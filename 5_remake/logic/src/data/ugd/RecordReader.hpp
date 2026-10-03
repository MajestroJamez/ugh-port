// The values of a record of the game data.
#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "data/kinds/Box.hpp"
#include "data/ugd/UgdRecord.hpp"

namespace ugh::data::ugd {

/**
 * The values of the record being read (at): its keys, texts, numbers, lists and ranges. A missing key or a bad value is an
 * error with the line number and the type of the record; the readers stop at the first one.
 */
class RecordReader {
public:
    /** The record the next values and errors are about (nullptr: none, an error of the whole file). */
    void at(const UgdRecord* record) { current_ = record; }

    /** Always false: the error `what` at the current record. */
    bool fail(const std::string& what);
    const std::string& error() const { return error_; }

    /** The record has no other keys than `keys`. */
    bool only(std::initializer_list<const char*> keys);
    bool text(const char* key, std::string& out);
    bool number(const char* key, int& out);
    /** A list `a,b,...`; `count` 0: any length. */
    bool numbers(const char* key, size_t count, std::vector<int>& out);
    /** A range `first..last`. */
    bool range(const char* key, int& first, int& last);
    /** `box=<x>,<y>,<halfWidth>,<halfHeight>`. */
    bool box(kinds::Box& out);

    static bool parseInt(const std::string& text, int& out);
    static std::vector<std::string> split(const std::string& text, char separator);

private:
    const UgdRecord* current_ = nullptr;
    std::string error_;
};

}  // namespace ugh::data::ugd

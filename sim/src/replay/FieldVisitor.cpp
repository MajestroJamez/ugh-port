#include "replay/FieldVisitor.hpp"

#include "replay/IndexField.hpp"
#include "replay/WordField.hpp"

namespace ugh::replay {

// A typed value is visited as a word (or an int) and set back from it: unchanged unless the visitor changed it.

void FieldVisitor::signedWord(const char* name, core::Word& value) { visit(name, WordField(value, Format::Signed)); }

void FieldVisitor::signedWord(const char* name, core::Fixed& value) {
    core::Word word = value.raw();
    signedWord(name, word);
    value = core::Fixed(word);
}

void FieldVisitor::signedWord(const char* name, core::Speed& value) {
    core::Word word = value.raw();
    signedWord(name, word);
    value = core::Speed(word);
}

void FieldVisitor::signedWord(const char* name, model::SharedWord& value) {
    core::Word word = value.word();
    signedWord(name, word);
    value.restoreWord(word);
}

void FieldVisitor::unsignedWord(const char* name, core::Word& value) {
    visit(name, WordField(value, Format::Unsigned));
}

void FieldVisitor::byte(const char* name, uint8_t& value) {
    core::Word word = value;
    visit(name, WordField(word, Format::Byte));
    value = static_cast<uint8_t>(word.bits());
}

void FieldVisitor::hex(const char* name, data::Sprite& value) {
    core::Word word = value;
    visit(name, WordField(word, Format::Hex));
    value = word.bits();
}

void FieldVisitor::hexOrNone(const char* name, data::Sprite& value) {
    core::Word word = value;
    visit(name, WordField(word, Format::HexOrNone));
    value = word.bits();
}

void FieldVisitor::hexOrNoneIfZero(const char* name, core::Word& value) {
    visit(name, WordField(value, Format::HexOrNoneIfZero));
}

void FieldVisitor::index(const char* name, int& value, int count, int fillMask) {
    visit(name, IndexField(value, count, fillMask));
}

void FieldVisitor::index(const char* name, core::Word& value, int count, int fillMask) {
    int i = value.bits();
    index(name, i, count, fillMask);
    value = i;
}

void FieldVisitor::index(const char* name, uint8_t& value, int count, int fillMask) {
    int i = value;
    index(name, i, count, fillMask);
    value = static_cast<uint8_t>(i);
}

}  // namespace ugh::replay

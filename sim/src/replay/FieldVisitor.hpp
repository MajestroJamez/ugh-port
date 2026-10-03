// What is done with the fields of the replay state.
#pragma once

#include <cstdint>

#include "core/Fixed.hpp"
#include "core/Speed.hpp"
#include "core/Word.hpp"
#include "data/Sprite.hpp"
#include "model/SharedWord.hpp"
#include "replay/Field.hpp"

namespace ugh::replay {

/**
 * Visitor over the fields of the replay state. Every entity has one list of its fields (CopterFields,
 * PassengerFields ...), and the visitors do one thing with each: FieldWriter writes them as text, FieldReader sets
 * the one named, FieldFiller fills them with a pattern. A new field is one line in the list of its entity.
 *
 * The shortcuts below visit the plain values of the model (positions, speeds, the shared words of the original,
 * indexes) as a WordField or an IndexField in the format the replays write them.
 */
class FieldVisitor {
public:
    virtual ~FieldVisitor() = default;

    virtual void visit(const char* name, const Field& field) = 0;

    /** The visitor changed a value: the snapshot it visited is to be restored. */
    virtual bool changedValues() const = 0;

    void signedWord(const char* name, core::Word& value);
    void signedWord(const char* name, core::Fixed& value);
    void signedWord(const char* name, core::Speed& value);
    void signedWord(const char* name, model::SharedWord& value);
    void unsignedWord(const char* name, core::Word& value);
    void byte(const char* name, uint8_t& value);
    void hex(const char* name, data::Sprite& value);
    /** A sprite, or "none" (NO_SPRITE). */
    void hexOrNone(const char* name, data::Sprite& value);
    /** A word, or "none" when it is 0. */
    void hexOrNoneIfZero(const char* name, core::Word& value);

    /** An index 0 .. count - 1 (IndexField); the pattern fills it with `pattern & fillMask`. */
    void index(const char* name, int& value, int count, int fillMask);
    void index(const char* name, core::Word& value, int count, int fillMask);
    void index(const char* name, uint8_t& value, int count, int fillMask);
};

}  // namespace ugh::replay

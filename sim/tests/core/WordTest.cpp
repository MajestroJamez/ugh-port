#include "TestFramework.hpp"
#include "core/Word.hpp"

using ugh::core::Word;

TEST(wordWrapsLikeARegister) {
    CHECK(Word(0x7fff) + 1 == Word(-0x8000));
    CHECK(Word(0x12345) == Word(0x2345));                // an int is cut to 16 bits
    CHECK(Word(-1).bits() == 0xffff && Word(-1).value() == -1);
    Word w = 0;
    CHECK(--w == Word(-1));
}

TEST(wordShiftsAndComparesLikeTheOriginal) {
    CHECK((Word(-5) >> 1) == Word(-3));                  // SAR rounds down
    CHECK((Word(0x4001) << 1) == Word(-0x7ffe));         // SHL wraps
    CHECK(Word(-1) < Word(1));                           // signed (JL)
    CHECK(Word::unsignedLess(Word(1), Word(-1)));        // unsigned (JB)
}

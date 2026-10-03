#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "data/UgdTokenizer.hpp"

using namespace ugh;

TEST(a_line_is_a_record_of_a_type_a_name_and_keys) {
    std::vector<data::UgdRecord> records;
    std::string error;
    CHECK(data::UgdTokenizer::tokenize("UGD 1\n# a comment\n\nanimation walk  frames=1,2\r\n", records, error));
    CHECK_EQUAL(size_t{1}, records.size());
    CHECK_EQUAL(4, records[0].line);
    CHECK_EQUAL(std::string("animation"), records[0].type);
    CHECK_EQUAL(std::string("walk"), records[0].name);
    CHECK_EQUAL(std::string("1,2"), records[0].fields.at("frames"));
}

TEST(a_word_without_a_value_after_the_name_is_an_error) {
    std::vector<data::UgdRecord> records;
    std::string error;
    CHECK(!data::UgdTokenizer::tokenize("UGD 1\nlevel 3 wind\n", records, error));
    CHECK_EQUAL(std::string("line 2: a word without a value: wind"), error);
    CHECK(!data::UgdTokenizer::tokenize("UGD 2\n", records, error));
    CHECK_EQUAL(std::string("not a UGD 1 file"), error);
}

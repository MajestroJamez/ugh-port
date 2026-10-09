#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "TestFramework.hpp"
#include "record/Base64Url.hpp"
#include "record/Crc32.hpp"
#include "record/ReplayCodec.hpp"
#include "record/ReplayText.hpp"

using ugh::record::Input;
using ugh::record::Recording;
using ugh::record::ReplayCodec;
using ugh::record::ReplayText;

namespace {

/** A replay of a team on hard with every kind of input: keys of both pilots, Other after a key, Esc, P, gaps. */
Recording sample() {
    Recording r;
    r.dataHash = 0x12345678;
    r.start.players = 2;
    r.start.difficulty = ugh::data::Difficulty::Hard;
    r.start.level = 53;
    r.start.lives = 7;
    r.start.points = 123456;
    r.start.multiplier = 3;
    r.start.random = {0x0003, 0x8134, 0x48bc, 0xffff};
    r.start.rainFloorRow = 180;
    r.start.effort = {153, -2};
    r.start.lastMenuKey = ugh::input::MenuKey::Escape;
    r.steps = 9000;
    r.playSteps = 8000;
    r.attempts = 2;
    r.points = 4321;
    r.done = true;
    r.date = 1791000000;
    r.password = "PASSWORD";
    r.names = {"Jan", "\xc5\xa0imon"};   // UTF-8
    int after = 0;
    for (int player = 0; player < 2; player++) {
        for (int key = 0; key < 5; key++) {
            for (bool pressed : {true, false}) {
                after += key * 37 + player;
                r.inputs.push_back(Input::pilot(after, player, static_cast<ugh::input::PlayerKey>(key), pressed));
                if (key % 2 == 0) r.inputs.push_back(Input::menu(after, ugh::input::MenuKey::Other));
            }
        }
    }
    r.inputs.push_back(Input::menu(after, ugh::input::MenuKey::Other));   // Other twice between two steps
    r.inputs.push_back(Input::menu(after + 1000, ugh::input::MenuKey::Pause));
    r.inputs.push_back(Input::menu(after + 1000, ugh::input::MenuKey::Escape));
    r.inputs.push_back(Input::menu(8999, ugh::input::MenuKey::Other));   // after the last step
    return r;
}

std::span<const uint8_t> bytesOf(const std::string& text) {
    return {reinterpret_cast<const uint8_t*>(text.data()), text.size()};
}

std::string refused(std::span<const uint8_t> bytes) {
    std::string error;
    return ReplayText::read(bytes, error) ? std::string("read") : error;
}

}  // namespace

TEST(a_replay_reads_back_the_same_from_its_file_and_its_text) {
    const Recording r = sample();
    std::vector<uint8_t> file = ReplayCodec::write(r);
    std::string error;
    std::optional<Recording> fromFile = ReplayText::read(file, error);
    CHECK(fromFile && *fromFile == r);
    const std::string text = ReplayText::write(r);
    CHECK(text.rfind("UGHR1:", 0) == 0);
    CHECK(text.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_", 6) == std::string::npos);
    std::optional<Recording> fromText = ReplayText::read(bytesOf(text), error);
    CHECK(fromText && *fromText == r);
    // pasted from a mail: a byte order mark, white space around it, its lines broken
    std::string pasted = "\xef\xbb\xbf  \r\n" + text.substr(0, 20) + "\r\n " + text.substr(20, 30) + "\n" + text.substr(50) + "\n\t";
    std::optional<Recording> fromMail = ReplayText::read(bytesOf(pasted), error);
    CHECK(fromMail && *fromMail == r);
}

TEST(a_replay_is_small) {
    Recording r = sample();
    r.inputs.clear();
    for (int i = 0; i < 300; i++) {   // a level of two minutes: 150 keys pressed and released, each told the game loop too
        r.inputs.push_back(Input::pilot(i * 28, 0, static_cast<ugh::input::PlayerKey>(i % 5), i % 2 == 0));
        r.inputs.push_back(Input::menu(i * 28, ugh::input::MenuKey::Other));
    }
    const size_t file = ReplayCodec::write(r).size();
    CHECK(file < 700);   // two bytes an input
    CHECK(ReplayText::write(r).size() < 940);
}

TEST(a_replay_is_refused_when_it_is_none_newer_or_damaged) {
    const Recording r = sample();
    const std::vector<uint8_t> file = ReplayCodec::write(r);
    const std::string text = ReplayText::write(r);
    CHECK_EQUAL(std::string(ReplayCodec::NOT_A_REPLAY), refused({}));
    CHECK_EQUAL(std::string(ReplayCodec::NOT_A_REPLAY), refused(bytesOf("hello")));
    CHECK_EQUAL(std::string(ReplayCodec::NOT_A_REPLAY), refused(bytesOf("\x89PNG\r\n\x1a\n")));
    CHECK_EQUAL(std::string(ReplayCodec::NOT_A_REPLAY), refused(bytesOf("UGR 1\n# a golden replay\n")));
    CHECK_EQUAL(std::string(ReplayCodec::NOT_A_REPLAY), refused(bytesOf("UGHR:abc")));
    // newer: the file's format byte, the text's number
    std::vector<uint8_t> newer = file;
    newer[4] = 2;
    CHECK_EQUAL(std::string(ReplayCodec::NEWER), refused(newer));
    CHECK_EQUAL(std::string(ReplayCodec::NEWER), refused(bytesOf("UGHR2:" + text.substr(6))));
    CHECK_EQUAL(std::string(ReplayCodec::NEWER), refused(bytesOf("UGHR12:AAAA")));
    // a byte changed anywhere, one cut off, a character of the text changed or added
    for (size_t at = 5; at < file.size(); at += 7) {
        std::vector<uint8_t> damaged = file;
        damaged[at] ^= 0x10;
        CHECK_EQUAL(std::string(ReplayCodec::DAMAGED_CHECKSUM), refused(damaged));
    }
    std::vector<uint8_t> shorter(file.begin(), file.end() - 1);
    CHECK_EQUAL(std::string(ReplayCodec::DAMAGED_CHECKSUM), refused(shorter));
    CHECK_EQUAL(std::string(ReplayCodec::DAMAGED), refused(std::vector<uint8_t>(file.begin(), file.begin() + 6)));
    std::string changed = text;
    changed[40] = changed[40] == 'A' ? 'B' : 'A';
    CHECK(refused(bytesOf(changed)) != "read");
    CHECK_EQUAL(std::string(ReplayText::NOT_TEXT), refused(bytesOf(text.substr(0, 30) + "!" + text.substr(30))));
    CHECK(refused(bytesOf(text.substr(0, text.size() - 3))) != "read");
    // a checksum right over values out of range
    Recording bad = r;
    bad.start.players = 3;
    std::vector<uint8_t> badFile = ReplayCodec::write(bad);
    CHECK_EQUAL(std::string(ReplayCodec::DAMAGED), refused(badFile));
}

TEST(the_label_of_a_replay_is_kept_as_its_file_keeps_it) {
    Recording r = sample();
    r.password = std::string(40, 'A');
    r.names = {"Jan\nX", std::string(31, 'b') + "\xc5\xa0", "third"};   // a control, a character over the 32 bytes, a third
    std::string error;
    std::optional<Recording> read = ReplayCodec::read(ReplayCodec::write(r), error);
    CHECK(read.has_value());
    if (!read) return;
    CHECK_EQUAL(std::string(32, 'A'), read->password);
    CHECK_EQUAL(size_t{2}, read->names.size());
    CHECK_EQUAL(std::string("JanX"), read->names[0]);
    CHECK_EQUAL(std::string(31, 'b'), read->names[1]);
}

TEST(the_best_replay_is_a_level_done_with_more_points_then_less_time) {
    Recording a = sample(), b = sample();
    CHECK(a.betterThan(nullptr));
    b.points = a.points + 1;
    CHECK(b.betterThan(&a) && !a.betterThan(&b));
    b.points = a.points;
    b.playSteps = a.playSteps - 1;
    CHECK(b.betterThan(&a) && !a.betterThan(&b));
    b.playSteps = a.playSteps;
    CHECK(!b.betterThan(&a) && !a.betterThan(&b));   // the same: the one kept stays
    b.done = false;
    b.points = a.points + 1000;
    CHECK(!b.betterThan(&a) && a.betterThan(&b) && !b.betterThan(nullptr));
}

TEST(base64url_and_crc32_are_the_standard_ones) {
    const std::string hello = "hello world";
    std::span<const uint8_t> bytes = bytesOf(hello);
    CHECK_EQUAL(std::string("aGVsbG8gd29ybGQ"), ugh::record::Base64Url::encode(bytes));
    CHECK_EQUAL(0x0d4a1185u, ugh::record::Crc32::of(bytes));
    std::vector<uint8_t> all;
    for (int i = 0; i < 256; i++) all.push_back(static_cast<uint8_t>(i));
    for (size_t n = 0; n < 7; n++) {
        std::span<const uint8_t> part(all.data() + 250 - n, n);
        auto back = ugh::record::Base64Url::decode(ugh::record::Base64Url::encode(part));
        CHECK(back && *back == std::vector<uint8_t>(part.begin(), part.end()));
    }
    CHECK(!ugh::record::Base64Url::decode("A"));    // no byte in one character
    CHECK(!ugh::record::Base64Url::decode("AB"));   // its last bits not 0
    CHECK(!ugh::record::Base64Url::decode("A+"));   // standard base64, not the URL's
}

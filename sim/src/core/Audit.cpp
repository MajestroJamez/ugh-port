#include "core/Audit.hpp"

#ifdef UGH_AUDIT
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>

namespace ugh::core::audit {

namespace {

struct Counters {
    std::map<std::string, long long> counts;
    ~Counters() {
        const char* path = std::getenv("UGH_AUDIT_OUT");
        FILE* out = path ? std::fopen(path, "a") : stderr;
        if (!out) return;
        for (const auto& [key, n] : counts) std::fprintf(out, "%lld\t%s\n", n, key.c_str());
        if (out != stderr) std::fclose(out);
    }
};

Counters& counters() {
    static Counters c;
    return c;
}

const char* current = "-";
bool on = true;

}  // namespace

void count(const std::string& key) {
    if (on) counters().counts[key + " @" + current]++;
}
void setContext(const char* context) { current = context; }
const char* intern(const std::string& text) {
    static std::set<std::string> texts;
    return texts.insert(text).first->c_str();
}
const char* context() { return current; }
bool enabled() { return on; }
void enable(bool value) { on = value; }

}  // namespace ugh::core::audit

extern "C" void ugh_sim_audit_enable(int on) { ugh::core::audit::enable(on != 0); }
#endif

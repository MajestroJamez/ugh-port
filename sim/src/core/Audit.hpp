// N1 audit (branch audit/n1 only): counters of what the core does with the original's memory.
#pragma once

#include <string>

namespace ugh::core::audit {

#ifdef UGH_AUDIT
void count(const std::string& key);
const char* intern(const std::string& text);
void setContext(const char* context);
const char* context();
bool enabled();
void enable(bool on);
#else
inline void count(const std::string&) {}
inline const char* intern(const std::string&) { return ""; }
inline void setContext(const char*) {}
inline const char* context() { return ""; }
inline bool enabled() { return false; }
inline void enable(bool) {}
#endif

/** Sets the context for a scope and restores the previous one. */
class Scope {
public:
    explicit Scope(const char* context) : previous_(audit::context()) { setContext(context); }
    ~Scope() { setContext(previous_); }

private:
    const char* previous_;
};

}  // namespace ugh::core::audit

// Coroutines for the game flow: the original waits for the vertical retrace in many places (fades, captions,
// the level loop); the core writes those places as `co_await vsync()` inside nested coroutines (Task), so the
// control flow and its local state (a fade position, the water row the caption keeps) stay the original ones.
// The driver (Sim::step) resumes the innermost waiting coroutine once per frame.
#pragma once

#include <coroutine>
#include <exception>
#include <utility>

namespace ugh {

/** A lazily started coroutine that can be awaited by another one; resumes its awaiter when it finishes. */
class Task {
public:
    struct promise_type {
        std::coroutine_handle<> continuation = std::noop_coroutine();

        Task get_return_object() { return Task(std::coroutine_handle<promise_type>::from_promise(*this)); }
        std::suspend_always initial_suspend() noexcept { return {}; }
        auto final_suspend() noexcept {
            struct Resume {
                bool await_ready() noexcept { return false; }
                std::coroutine_handle<> await_suspend(std::coroutine_handle<promise_type> h) noexcept {
                    return h.promise().continuation;
                }
                void await_resume() noexcept {}
            };
            return Resume{};
        }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };

    Task() = default;
    explicit Task(std::coroutine_handle<promise_type> h) : h_(h) {}
    Task(Task&& other) noexcept : h_(std::exchange(other.h_, {})) {}
    Task& operator=(Task&& other) noexcept {
        if (this != &other) { if (h_) h_.destroy(); h_ = std::exchange(other.h_, {}); }
        return *this;
    }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;
    ~Task() { if (h_) h_.destroy(); }

    bool await_ready() const noexcept { return false; }
    std::coroutine_handle<> await_suspend(std::coroutine_handle<> awaiter) noexcept {
        h_.promise().continuation = awaiter;
        return h_;
    }
    void await_resume() const noexcept {}

    /** Starts a top-level task (no awaiter). */
    void start() { h_.resume(); }
    bool done() const { return !h_ || h_.done(); }

private:
    std::coroutine_handle<promise_type> h_;
};

/** Awaiting a retrace: the chain of coroutines stops here until the driver resumes it for the next frame. */
struct Retrace {
    std::coroutine_handle<>* waiting;

    bool await_ready() const noexcept { return false; }
    void await_suspend(std::coroutine_handle<> h) const noexcept { *waiting = h; }
    void await_resume() const noexcept {}
};

}  // namespace ugh

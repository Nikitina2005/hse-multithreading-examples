#include <linux/futex.h>
#include <sys/syscall.h>

#include <atomic>

enum class lock_state{UNLOCK, LOCK_NO_WAIT, LOCK_WAIT};

void futex_wait(void * value, lock_state expected_value) {
    syscall(SYS_futex, value, FUTEX_WAIT_PRIVATE, expected_value, nullptr, nullptr, 0);
}

void futex_wake(void * value) {
    syscall(SYS_futex, value, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
}

class mutex {
    std::atomic<lock_state> state_{};
public:
    constexpr mutex() noexcept = default;
    ~mutex() = default;

    mutex(const mutex&) = delete;
    mutex& operator=(const mutex&) = delete;

    void lock();
    bool try_lock();
    void unlock();

    using native_handle_type = std::atomic<lock_state> *;
    native_handle_type native_handle();
};

void mutex::lock() {
    lock_state expected{};
    if (state_.compare_exchange_strong(expected, lock_state::LOCK_NO_WAIT)) {
        return;
    }
    while (true) {
        expected = lock_state::UNLOCK;
        if (state_.compare_exchange_strong(expected, lock_state::LOCK_WAIT)) {
            return;
        }
        expected = lock_state::LOCK_NO_WAIT;
        state_.compare_exchange_strong(expected, lock_state::LOCK_WAIT);
        futex_wait(&state_, lock_state::LOCK_WAIT);
    }
}

bool mutex::try_lock() {
    lock_state expected{};
    return state_.compare_exchange_strong(expected, lock_state::LOCK_NO_WAIT);
}

void mutex::unlock() {
    const lock_state old_state = state_.exchange(lock_state::UNLOCK);
    if (old_state == lock_state::LOCK_WAIT) {
        futex_wake(&state_);
    }
}

mutex::native_handle_type mutex::native_handle() {
    return &state_;
}
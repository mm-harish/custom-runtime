#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>
#include <emmintrin.h>

/**
 * @brief A simple spinlock implementation using std::atomic_flag.
 *
 * This lock uses an exponential backoff strategy with _mm_pause() to reduce
 * contention on the bus.
 */
struct alignas(64) SpinLock {
  std::atomic<bool> locked{false}; ///< The atomic flag used for locking.

  /**
   * @brief Acquires the lock.
   *
   * Spins until the lock is acquired. Uses exponential backoff.
   */
  void lock() {
    for (;;) {
      // First: spin on shared reads
      while (locked.load(std::memory_order_relaxed)) {
        _mm_pause();
      }
      // Then: attempt to acquire
      if (!locked.exchange(true, std::memory_order_acquire)) {
        return;
      }
    }
  }

  /**
   * @brief Releases the lock.
   */
  void unlock() { locked.store(false, std::memory_order_release); }
};

#endif // SPINLOCK_H

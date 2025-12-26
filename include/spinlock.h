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
  std::atomic_flag flag =
      ATOMIC_FLAG_INIT; ///< The atomic flag used for locking.

  /**
   * @brief Acquires the lock.
   *
   * Spins until the lock is acquired. Uses exponential backoff.
   */
  void lock() {
    int spins = 128;
    while (flag.test_and_set(std::memory_order_acquire)) {
      for (int i = 0; i < spins; i++)
        _mm_pause();
      if (spins < 1024)
        spins *= 2;
    }
  }

  /**
   * @brief Releases the lock.
   */
  void unlock() { flag.clear(std::memory_order_release); }
};

#endif // SPINLOCK_H

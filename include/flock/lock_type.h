// This selects between using a hashlock or inline lock at compile time
// For the hashlock the address of the structure is hashed to one of a
// fixed number of lock locations and there is no need for a lock per
// structure.  However, the hashing can create lock cycles so this
// cannot be used with a strict lock, just a try_lock.

#include "spin_lock.h"

#ifdef HashLock
struct lock_type {
  template <typename F>
  bool try_with_lock(F f) { return try_lock_loc(this, f); }
  void clear_the_lock() { clear_lock_loc(this); }
  bool is_locked() { return is_locked_loc(this); }
};
#else 
struct lock_type {
  lock lck;
  template <typename F>
  bool try_with_lock(F f) {
    return lck->try_lock(f); }
  template <typename F>
  auto try_with_lock_result(F f) -> std::optional<decltype(f())> {
    return lck->try_lock_result(f); }
  void clear_the_lock() { lck->clear_lock(); }
  bool is_locked() { return lck->is_locked; }
};
#endif


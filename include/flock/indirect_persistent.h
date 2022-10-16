#pragma once

void print_counts() {}

struct persistent {};

struct version_link {
  std::atomic<TS> time_stamp;
  write_once<version_link*> next_version;
  void* value;
  version_link() : time_stamp(tbd) {}
  version_link(TS time, version_link* next, void* value) :
    time_stamp(time), next_version(next), value(value) {}  
};

memory_pool<version_link> link_pool;

template <typename V>
struct persistent_ptr {
private:
  mutable_val<version_link*> v;

  static version_link* set_stamp(version_link* ptr) {
    if (ptr->time_stamp.load() == tbd) {
      TS old_t = tbd;
      TS new_t = global_stamp.get_write_stamp();
      ptr->time_stamp.compare_exchange_strong(old_t, new_t);
    }
    return ptr;
  }

  static version_link* init_ptr(V* ptr) {
    return link_pool.new_obj(zero_stamp, nullptr, (void*) ptr);
  }

public:

  persistent_ptr(): v(init_ptr(nullptr)) {}
  persistent_ptr(V* ptr) : v(init_ptr(ptr)) {}
  ~persistent_ptr() { link_pool.pool.retire(v.read());}
  void init(V* ptr) {v = init_ptr(ptr);}
  
  V* read_snapshot() {
    version_link* head = set_stamp(v.load());
    while (head->time_stamp.load() > local_stamp)
      head = head->next_version.load();
    return (V*) head->value;
  }

  V* load() {  // can be used anywhere
    if (local_stamp != -1) return read_snapshot();
    else return (V*) set_stamp(v.load())->value;
  }
  
  V* read() {  // only safe on journey
    return (V*) v.read()->value;
  }

  void validate() {
    set_stamp(v.load());     // ensure time stamp is set
  }

  void store(V* ptr) {
    version_link* old_v = v.load();
    version_link* new_v = link_pool.new_obj(tbd, old_v, (void*) ptr);
    v = new_v;
    set_stamp(new_v);
    link_pool.retire(old_v);    
  }

  V* operator=(V* b) {store(b); return b; }
};

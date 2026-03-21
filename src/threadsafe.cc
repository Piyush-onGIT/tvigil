#include "threadsafe.h"
#include <iostream>
using namespace std;

namespace ts {

void enter_critical_section(Registry &registry, const std::string &name) {
  {
    lock_guard<mutex> lock(registry.map_mtx);
    if (registry.sections.find(name) == registry.sections.end()) {
      registry.sections[name] = std::make_unique<SectionStats>();
      registry.sections[name]->name = name;
    }
  }
  SectionStats &stats = *registry.sections[name];
  stats.total_entries++;
  int current_threads = ++stats.active_threads;

  int peak = stats.max_concurrent.load();

  /*
   * --------------------------------CAS-------------------------------------
   * value.compare_exchange_weak(expected, desired): it compares max_concurrent
   * and peak if fails (unequal) peak is updated to max_concurrent and retry
   * loop if pass (equal, value is still the same which was last read by this
   * thread), max_concurrent is set to current.
   * -------------------------------------------------------------------------
   */
  while (current_threads > peak && !stats.max_concurrent.compare_exchange_weak(
                                       peak, current_threads)) { /* retry */
  }

  if (current_threads > 1) {
    stats.violation_detected = true;
    cout << "[VIOLATION] section \"" << name << "\""
         << " — " << current_threads << " threads inside simultaneously\n";
  }
}

void exit_critical_section(Registry &registry, const std::string &name) {
  if (registry.sections.find(name) == registry.sections.end())
    return;
  SectionStats &stats = *registry.sections[name];
  stats.active_threads--;
}

} // namespace ts

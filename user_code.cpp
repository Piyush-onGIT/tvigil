// User written code that contains various critical sections which requires
// thread-safety.
#include "include/threadsafe.h"
#include <iostream>
#include <thread>
#include <vector>

// --- shared data (no protection at all) ---
int counter = 0;
double balance = 1000.0;
std::vector<int> log_entries;
ts::Registry registry;

// unsafe section 1: two threads increment counter simultaneously
void increment() {
  ts::enter_critical_section(registry, "increment");
  for (int i = 0; i < 1000; i++) {
    counter++; // read-modify-write, no lock
  }
  ts::exit_critical_section(registry, "increment");
}

// unsafe section 2: two threads modify balance simultaneously
void deposit(double amount) {
  ts::enter_critical_section(registry, "deposit");
  for (int i = 0; i < 100; i++) {
    double tmp = balance;
    tmp += amount;
    balance = tmp; // classic lost-update race
  }
  ts::exit_critical_section(registry, "deposit");
}

// unsafe section 3: two threads push into the same vector simultaneously
void add_log(int value) {
  ts::enter_critical_section(registry, "add_log");
  for (int i = 0; i < 50; i++) {
    log_entries.push_back(value); // vector resize is not thread-safe
  }
  ts::exit_critical_section(registry, "add_log");
}

int main() {
  std::thread t1(increment);
  std::thread t2(increment);
  t1.join();
  t2.join();
  std::cout << "counter (expected 2000): " << counter << "\n";

  std::thread t3(deposit, 100.0);
  std::thread t4(deposit, 200.0);
  t3.join();
  t4.join();
  std::cout << "balance (expected 31000): " << balance << "\n";

  std::thread t5(add_log, 1);
  std::thread t6(add_log, 2);
  t5.join();
  t6.join();
  std::cout << "log size (expected 100): " << log_entries.size() << "\n";

  std::cout << "\n--- summary ---\n";
  for (auto &[name, stats] : registry.sections) {
    std::cout << "section \"" << name << "\""
              << " | entries: " << stats->total_entries
              << " | peak threads: " << stats->max_concurrent
              << " | violation: " << (stats->violation_detected ? "YES" : "NO")
              << "\n";
  }

  return 0;
}
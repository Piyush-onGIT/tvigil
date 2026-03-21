#pragma once

#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ts {

// -----------------------------------------------------------------------------
//  ViolationEvent
//  Captures one moment where two or more threads were inside the
//  same critical section simultaneously.
// -----------------------------------------------------------------------------
struct ViolationEvent {
    std::thread::id  thread_id;       // the thread that caused the violation
    std::string      section_name;    // which section it happened in
    long long        timestamp_us;    // when it happened (microseconds)
};

// -----------------------------------------------------------------------------
//  SectionStats
//  Everything known about one registered critical section.
// -----------------------------------------------------------------------------
struct SectionStats {
    std::string           name;               // user-given name e.g. "increment"

    std::atomic<int>      active_threads{0};  // how many threads are inside RIGHT NOW
    std::atomic<int>      max_concurrent{0};  // peak: most threads ever inside at once
    std::atomic<int>      total_entries{0};   // total times any thread entered

    std::atomic<bool>     violation_detected{false};

    std::mutex            events_mtx;
    std::vector<ViolationEvent> events;       // all recorded violation moments

    // non-copyable (atomics cannot be copied)
    SectionStats() = default;
    SectionStats(const SectionStats&) = delete;
    SectionStats& operator=(const SectionStats&) = delete;
};

// -----------------------------------------------------------------------------
//  Registry
//  Holds all registered critical sections.
//  One global instance, lives for the entire program lifetime.
// -----------------------------------------------------------------------------
struct Registry {
    std::mutex map_mtx;
    std::unordered_map<std::string, std::unique_ptr<SectionStats>> sections;
};

void enter_critical_section(Registry &registry, const std::string& name);
void exit_critical_section(Registry &registry, const std::string& name);

} // namespace ts
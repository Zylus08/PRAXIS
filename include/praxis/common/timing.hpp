#pragma once
#include <chrono>
#include <cstdint>
namespace praxis {
struct Timer {
    std::uint64_t& total;
    std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
    explicit Timer(std::uint64_t& t):total(t){}
    ~Timer(){total+=static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count());}
};
}

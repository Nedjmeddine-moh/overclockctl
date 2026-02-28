#pragma once
#include <functional>
#include <string>

struct CpuBenchResult {
    double single_core_score = 0;  // single-thread score
    double multi_core_score  = 0;  // all-thread score
    double gflops_single     = 0;
    double gflops_multi      = 0;
    int    thread_count      = 0;
    double duration_sec      = 0;
    bool   completed         = false;
};

// progress callback: (0.0 - 1.0, status string)
using ProgressCb = std::function<void(double, const std::string&)>;

class CpuBenchmark {
public:
    // Runs synchronously - call from a thread!
    CpuBenchResult run(ProgressCb progress = nullptr);

private:
    double runSingleThread(int iterations);
    double runMultiThread(int iterations);
};

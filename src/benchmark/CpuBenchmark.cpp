#include "CpuBenchmark.h"
#include <cmath>
#include <chrono>
#include <thread>
#include <vector>
#include <atomic>
#include <numeric>

using Clock = std::chrono::high_resolution_clock;

// Heavy FP kernel: runs Mandelbrot iterations on a buffer.
// Returns time taken in seconds.
static double mandelbrotKernel(int width, int height, int max_iter) {
    std::vector<double> buf(width * height);
    auto t0 = Clock::now();

    for (int py = 0; py < height; ++py) {
        double ci = (py / (double)height) * 3.0 - 1.5;
        for (int px = 0; px < width; ++px) {
            double cr = (px / (double)width) * 3.5 - 2.5;
            double zr = 0, zi = 0;
            int iter = 0;
            while (zr * zr + zi * zi < 4.0 && iter < max_iter) {
                double tmp = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = tmp;
                ++iter;
            }
            buf[py * width + px] = iter;
        }
    }

    auto t1 = Clock::now();
    // prevent dead-code elimination
    volatile double sink = buf[width * height / 2];
    (void)sink;
    return std::chrono::duration<double>(t1 - t0).count();
}

// Scalar FLOP count: per pixel ~ 8 FLOPs * avg iterations
static double estimateGFlops(int w, int h, int max_iter, double sec) {
    double flops = (double)w * h * max_iter * 8.0;
    return flops / sec / 1e9;
}

double CpuBenchmark::runSingleThread(int iterations) {
    return mandelbrotKernel(800, 600, iterations);
}

double CpuBenchmark::runMultiThread(int iterations) {
    int n = (int)std::thread::hardware_concurrency();
    if (n < 1) n = 1;

    int rows_per_thread = 600 / n;
    std::vector<double> times(n, 0.0);
    std::vector<std::thread> threads;

    auto t0 = Clock::now();

    for (int t = 0; t < n; ++t) {
        threads.emplace_back([t, rows_per_thread, iterations, &times] {
            int h = rows_per_thread;
            int w = 800;
            std::vector<double> buf(w * h);
            for (int py = 0; py < h; ++py) {
                double ci = (py / (double)h) * 3.0 - 1.5;
                for (int px = 0; px < w; ++px) {
                    double cr = (px / (double)w) * 3.5 - 2.5;
                    double zr = 0, zi = 0;
                    int iter = 0;
                    while (zr * zr + zi * zi < 4.0 && iter < iterations) {
                        double tmp = zr * zr - zi * zi + cr;
                        zi = 2.0 * zr * zi + ci;
                        zr = tmp;
                        ++iter;
                    }
                    buf[py * w + px] = iter;
                }
            }
            volatile double s = buf[0]; (void)s;
        });
    }
    for (auto& th : threads) th.join();

    auto t1 = Clock::now();
    return std::chrono::duration<double>(t1 - t0).count();
}

CpuBenchResult CpuBenchmark::run(ProgressCb progress) {
    CpuBenchResult res;
    res.thread_count = (int)std::thread::hardware_concurrency();
    if (res.thread_count < 1) res.thread_count = 1;

    const int ITER = 256;

    if (progress) progress(0.05, "Warming up...");
    mandelbrotKernel(200, 150, ITER); // warm up

    if (progress) progress(0.15, "Single-core benchmark...");
    auto t0 = Clock::now();
    double st = runSingleThread(ITER);
    if (progress) progress(0.50, "Multi-core benchmark...");
    double mt = runMultiThread(ITER);
    auto t1 = Clock::now();

    res.duration_sec      = std::chrono::duration<double>(t1 - t0).count();
    res.gflops_single     = estimateGFlops(800, 600, ITER, st);
    res.gflops_multi      = estimateGFlops(800, 600 / res.thread_count, ITER, mt) * res.thread_count;

    // Normalize to a "score" relative to a reference system (~1.0 GFlops single = 1000 pts)
    res.single_core_score = res.gflops_single * 1000.0;
    res.multi_core_score  = res.gflops_multi  * 1000.0;
    res.completed         = true;

    if (progress) progress(1.0, "Done!");
    return res;
}

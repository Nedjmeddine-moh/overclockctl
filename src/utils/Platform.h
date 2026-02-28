#pragma once

// ── OS detection ─────────────────────────────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
  #define OC_PLATFORM_WINDOWS 1
#elif defined(__linux__)
  #define OC_PLATFORM_LINUX   1
#else
  #error "Unsupported platform"
#endif

// ── Windows helpers ──────────────────────────────────────────────────────────
#ifdef OC_PLATFORM_WINDOWS
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  #include <intrin.h>     // __cpuid
  #include <powerbase.h>  // CallNtPowerInformation
  #pragma comment(lib, "PowrProf.lib")
#endif

// ── Linux helpers ────────────────────────────────────────────────────────────
#ifdef OC_PLATFORM_LINUX
  #include <unistd.h>     // geteuid
#endif

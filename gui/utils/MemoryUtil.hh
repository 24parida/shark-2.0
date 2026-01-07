#pragma once
#include <cstddef>
#include <string>
#include <atomic>

#ifdef _WIN32
  #include <windows.h>
#elif __linux__
  #include <fstream>
  #include <string>
#elif __APPLE__
  #include <sys/sysctl.h>
  #include <mach/mach.h>
  #include <mach/vm_statistics.h>
#endif

namespace MemoryUtil {
  // Get available physical RAM in bytes
  inline size_t getAvailableMemory() {
    #ifdef _WIN32
      MEMORYSTATUSEX status;
      status.dwLength = sizeof(status);
      GlobalMemoryStatusEx(&status);
      return static_cast<size_t>(status.ullAvailPhys);
    #elif __linux__
      std::ifstream meminfo("/proc/meminfo");
      std::string line;
      while (std::getline(meminfo, line)) {
        if (line.find("MemAvailable:") == 0) {
          size_t kb = std::stoull(line.substr(14));
          return kb * 1024;
        }
      }
      return 0;
    #elif __APPLE__
      vm_statistics64_data_t vm_stats;
      mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;
      host_statistics64(mach_host_self(), HOST_VM_INFO64,
                       (host_info64_t)&vm_stats, &count);
      return (vm_stats.free_count + vm_stats.inactive_count) * vm_page_size;
    #else
      return 0;
    #endif
  }

  // Get total physical RAM in bytes
  inline size_t getTotalMemory() {
    #ifdef _WIN32
      MEMORYSTATUSEX status;
      status.dwLength = sizeof(status);
      GlobalMemoryStatusEx(&status);
      return static_cast<size_t>(status.ullTotalPhys);
    #elif __linux__
      std::ifstream meminfo("/proc/meminfo");
      std::string line;
      while (std::getline(meminfo, line)) {
        if (line.find("MemTotal:") == 0) {
          size_t kb = std::stoull(line.substr(10));
          return kb * 1024;
        }
      }
      return 0;
    #elif __APPLE__
      int64_t mem;
      size_t len = sizeof(mem);
      sysctlbyname("hw.memsize", &mem, &len, NULL, 0);
      return static_cast<size_t>(mem);
    #else
      return 0;
    #endif
  }

  // Format bytes as human-readable string
  inline std::string formatBytes(size_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit = 0;
    double size = static_cast<double>(bytes);

    while (size >= 1024.0 && unit < 4) {
      size /= 1024.0;
      unit++;
    }

    char buffer[64];
    snprintf(buffer, sizeof(buffer), "%.2f %s", size, units[unit]);
    return std::string(buffer);
  }

  // Memory estimate structure
  struct MemoryEstimate {
    size_t tree_structure_bytes;
    size_t dcfr_storage_bytes;
    size_t overhead_bytes;
    size_t total_bytes;

    bool fits_in_memory(float safety_margin = 0.9f) const {
      size_t available = getAvailableMemory();
      return total_bytes < (available * safety_margin);
    }
  };
}

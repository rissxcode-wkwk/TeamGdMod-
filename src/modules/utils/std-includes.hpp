#pragma once

// Geode v6 mengurangi header yang di-include secara transitif ("Reduced includes across
// the whole codebase"). Header ini di-include paksa lewat CMake supaya kode lama yang
// mengandalkan include transitif tetap bisa dikompilasi.

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cmath>
#include <concepts>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <ranges>
#include <set>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

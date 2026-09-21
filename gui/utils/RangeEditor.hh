#pragma once
#include "hands/PreflopRange.hh"
#include <string>
#include <vector>

namespace RangeEditor {
inline std::string join(const std::vector<std::string> &tokens) {
  std::string text;
  for (const auto &token : tokens) {
    if (!text.empty()) text += ',';
    text += token;
  }
  return text;
}
}

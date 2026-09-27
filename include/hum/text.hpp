#pragma once

#include <array>
#include <string>
#include <vector>

namespace hum {

std::string stripAnsi(const std::string& input);
int displayWidth(const std::string& input);
std::vector<std::string> splitLines(const std::string& input);
std::string trim(const std::string& input);
std::array<int, 4> parseBox(const std::string& value);

}  // namespace hum

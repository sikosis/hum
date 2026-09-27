#pragma once

#include <string>
#include <vector>

namespace hum {

int runJoin(const std::vector<std::string>& args);
std::string joinHorizontal(const std::vector<std::string>& blocks, const std::string& align);
std::string joinVertical(const std::vector<std::string>& blocks, const std::string& align);
void printJoinHelp();

}  // namespace hum

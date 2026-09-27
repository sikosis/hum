#pragma once

#include <string>
#include <vector>

namespace hum {

struct ConfirmOptions {
    std::string prompt = "Are you sure?";
    std::string affirmative = "Yes";
    std::string negative = "No";
    bool defaultAffirmative = true;
    bool showOutput = false;
    bool showHelp = true;
    int timeoutMilliseconds = 0;
};

int runConfirm(const std::vector<std::string>& args);
void printConfirmHelp();

}  // namespace hum

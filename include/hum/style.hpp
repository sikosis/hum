#pragma once

#include <string>
#include <vector>

namespace hum {

struct StyleOptions {
    std::string foreground;
    std::string background;
    std::string border = "none";
    std::string borderForeground;
    std::string borderBackground;
    std::string align = "left";
    int width = 0;
    int height = 0;
    std::string margin = "0";
    std::string padding = "0";
    bool bold = false;
    bool faint = false;
    bool italic = false;
    bool strikethrough = false;
    bool underline = false;
    bool trimLines = false;
    bool stripInputAnsi = true;
};

int runStyle(const std::vector<std::string>& args);
std::string renderStyle(const std::string& text, const StyleOptions& options);
std::string ansiColourCode(const std::string& value, bool background = false);
void printStyleHelp();

}  // namespace hum

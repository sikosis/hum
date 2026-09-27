#include "hum/join.hpp"

#include "hum/text.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace hum {
namespace {

std::string spaces(int count) {
//---------------------------------------------------------------------------------------------------------------------------------//

    return std::string(static_cast<std::size_t>(std::max(0, count)), ' ');
}

}  // namespace

std::string joinHorizontal(const std::vector<std::string>& blocks, const std::string& align) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::vector<std::vector<std::string>> lines;
    std::vector<int> widths;
    std::size_t maximumHeight = 0;
    for (const std::string& block : blocks) {
        lines.push_back(splitLines(block));
        int width = 0;
        for (const std::string& line : lines.back()) width = std::max(width, displayWidth(line));
        widths.push_back(width);
        maximumHeight = std::max(maximumHeight, lines.back().size());
    }

    std::ostringstream output;
    for (std::size_t row = 0; row < maximumHeight; ++row) {
        for (std::size_t block = 0; block < lines.size(); ++block) {
            int offset = 0;
            if (align == "middle") offset = static_cast<int>((maximumHeight - lines[block].size()) / 2);
            else if (align == "bottom") offset = static_cast<int>(maximumHeight - lines[block].size());
            const int sourceRow = static_cast<int>(row) - offset;
            std::string value;
            if (sourceRow >= 0 && sourceRow < static_cast<int>(lines[block].size())) {
                value = lines[block][static_cast<std::size_t>(sourceRow)];
            }
            output << value << spaces(widths[block] - displayWidth(value));
        }
        if (row + 1 < maximumHeight) output << '\n';
    }
    return output.str();
}

std::string joinVertical(const std::vector<std::string>& blocks, const std::string& align) {
//---------------------------------------------------------------------------------------------------------------------------------//

    int maximumWidth = 0;
    std::vector<std::vector<std::string>> lines;
    for (const std::string& block : blocks) {
        lines.push_back(splitLines(block));
        for (const std::string& line : lines.back()) maximumWidth = std::max(maximumWidth, displayWidth(line));
    }

    std::ostringstream output;
    bool first = true;
    for (const auto& blockLines : lines) {
        for (const std::string& line : blockLines) {
            if (!first) output << '\n';
            first = false;
            const int remaining = maximumWidth - displayWidth(line);
            int left = 0;
            if (align == "center") left = remaining / 2;
            else if (align == "right") left = remaining;
            output << spaces(left) << line << spaces(remaining - left);
        }
    }
    return output.str();
}

int runJoin(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    bool vertical = false;
    std::string align = "left";
    std::vector<std::string> blocks;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--help" || arg == "-h") { printJoinHelp(); return 0; }
            if (arg == "--horizontal") vertical = false;
            else if (arg == "--vertical") vertical = true;
            else if (arg == "--align") {
                if (++i >= args.size()) throw std::invalid_argument("--align requires a value");
                align = args[i];
            } else if (!arg.empty() && arg[0] == '-') {
                throw std::invalid_argument("unknown option: " + arg);
            } else {
                blocks.push_back(arg);
            }
        }
        if (blocks.empty()) throw std::invalid_argument("provide at least one text block");
        if (vertical && align != "left" && align != "center" && align != "right") {
            throw std::invalid_argument("vertical alignment must be left, center, or right");
        }
        if (!vertical && align != "top" && align != "middle" && align != "bottom" && align != "left") {
            throw std::invalid_argument("horizontal alignment must be top, middle, or bottom");
        }
        std::cout << (vertical ? joinVertical(blocks, align) : joinHorizontal(blocks, align)) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hum join: " << error.what() << '\n';
        return 2;
    }
}

void printJoinHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum join [options] text ...\n\n"
        "Join potentially multi-line text blocks.\n\n"
        "Options:\n"
        "  --horizontal       Join side-by-side (default)\n"
        "  --vertical         Stack blocks vertically\n"
        "  --align POSITION   top, middle, bottom, left, center, or right\n";
}

}  // namespace hum

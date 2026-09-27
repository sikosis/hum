#include "hum/pager.hpp"

#include "hum/terminal.hpp"
#include "hum/text.hpp"
#include "hum/viewport.hpp"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace hum {
namespace {

std::string requireValue(const std::vector<std::string>& args, std::size_t& index,
                         const std::string& option) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (++index >= args.size()) throw std::invalid_argument(option + " requires a value");
    return args[index];
}

int parseDuration(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::size_t consumed = 0;
    const double amount = std::stod(value, &consumed);
    const std::string unit = value.substr(consumed);
    if (amount < 0) throw std::invalid_argument("timeout cannot be negative");
    if (unit == "ms") return static_cast<int>(amount);
    if (unit.empty() || unit == "s") return static_cast<int>(amount * 1000.0);
    throw std::invalid_argument("timeout unit must be ms or s");
}

std::vector<std::string> wrapLines(const std::vector<std::string>& input, int width, bool softWrap) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::vector<std::string> output;
    for (const std::string& line : input) {
        if (!softWrap || displayWidth(line) <= width) {
            output.push_back(line);
            continue;
        }
        std::string remaining = line;
        while (!remaining.empty()) {
            std::string part = fitText(remaining, width);
            if (!part.empty() && part.back() == ' ') {
                while (!part.empty() && part.back() == ' ') part.pop_back();
            }
            if (part.size() >= std::string("…").size() &&
                part.compare(part.size() - std::string("…").size(), std::string("…").size(), "…") == 0) {
                part.erase(part.size() - std::string("…").size());
            }
            if (part.empty()) break;
            output.push_back(part);
            remaining.erase(0, part.size());
        }
        if (line.empty()) output.push_back("");
    }
    return output;
}

std::string repeat(const std::string& value, int count) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string result;
    for (int index = 0; index < count; ++index) result += value;
    return result;
}

}  // namespace

int runPager(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    bool showLineNumbers = true;
    bool softWrap = true;
    int requestedHeight = 0;
    int requestedWidth = 0;
    int timeoutMilliseconds = 0;
    std::string content;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--help" || arg == "-h") { printPagerHelp(); return 0; }
            if (arg == "--show-line-numbers") showLineNumbers = true;
            else if (arg == "--no-show-line-numbers") showLineNumbers = false;
            else if (arg == "--soft-wrap") softWrap = true;
            else if (arg == "--no-soft-wrap") softWrap = false;
            else if (arg == "--height") requestedHeight = std::stoi(requireValue(args, i, arg));
            else if (arg == "--width") requestedWidth = std::stoi(requireValue(args, i, arg));
            else if (arg == "--timeout") timeoutMilliseconds = parseDuration(requireValue(args, i, arg));
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option: " + arg);
            else {
                if (!content.empty()) content += " ";
                content += arg;
            }
        }
        if (requestedHeight < 0 || requestedWidth < 0) throw std::invalid_argument("dimensions cannot be negative");
    } catch (const std::exception& error) {
        std::cerr << "hum pager: " << error.what() << '\n';
        return 2;
    }

    if (content.empty() && ::isatty(STDIN_FILENO) == 0) {
        content.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
    }
    TerminalSession terminal;
    if (!terminal.interactive() || !terminal.enableRawMode()) {
        std::cout << content;
        if (!content.empty() && content.back() != '\n') std::cout << '\n';
        return 0;
    }

    const TerminalDimensions dimensions = terminalDimensions();
    const int width = requestedWidth > 0 ? requestedWidth : std::max(20, dimensions.columns - 4);
    const int height = requestedHeight > 0 ? requestedHeight : std::max(3, dimensions.rows - 5);
    const int numberWidth = static_cast<int>(std::to_string(std::max<std::size_t>(1, splitLines(content).size())).size());
    const int textWidth = std::max(1, width - (showLineNumbers ? numberWidth + 3 : 0));
    const std::vector<std::string> lines = wrapLines(splitLines(content), textWidth, softWrap);
    std::size_t top = 0;
    LiveRegion region(terminal);
    while (true) {
        std::vector<std::string> screen;
        screen.push_back("╭" + repeat("─", width) + "╮");
        for (int row = 0; row < height; ++row) {
            const std::size_t index = top + static_cast<std::size_t>(row);
            std::string value;
            if (index < lines.size()) {
                if (showLineNumbers) {
                    std::ostringstream number;
                    number << std::setw(numberWidth) << index + 1 << " │ ";
                    value = number.str();
                }
                value += fitText(lines[index], textWidth);
            }
            screen.push_back("│" + fitText(value, width) + "│");
        }
        screen.push_back("╰" + repeat("─", width) + "╯");
        screen.push_back("↑/↓ scroll • pgup/pgdn page • g/G ends • q quit");
        region.render(screen);

        const InputEvent event = terminal.readEvent(timeoutMilliseconds);
        const std::size_t maximumTop = lines.size() > static_cast<std::size_t>(height)
            ? lines.size() - static_cast<std::size_t>(height) : 0;
        if (event.key == Key::Up || event.text == "k") {
            if (top > 0) --top;
        } else if (event.key == Key::Down || event.text == "j") {
            if (top < maximumTop) ++top;
        } else if (event.key == Key::PageUp) {
            top = top > static_cast<std::size_t>(height) ? top - static_cast<std::size_t>(height) : 0;
        } else if (event.key == Key::PageDown) {
            top = std::min(maximumTop, top + static_cast<std::size_t>(height));
        } else if (event.text == "g" || event.key == Key::Home) {
            top = 0;
        } else if (event.text == "G" || event.key == Key::End) {
            top = maximumTop;
        } else if (event.text == "q" || event.key == Key::Escape || event.key == Key::Interrupt ||
                   event.key == Key::EndOfInput || event.key == Key::Timeout) {
            return 0;
        }
    }
}

void printPagerHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum pager [options] [content]\n\n"
        "Scroll through content supplied as an argument or on stdin.\n\n"
        "Options:\n"
        "  --[no-]show-line-numbers  Toggle line numbers\n"
        "  --[no-]soft-wrap          Toggle long-line wrapping\n"
        "  --width N                 Viewport width\n"
        "  --height N                Viewport height\n"
        "  --timeout DURATION        Exit after an idle duration\n";
}

}  // namespace hum

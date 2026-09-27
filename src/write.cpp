#include "hum/write.hpp"

#include "hum/style.hpp"
#include "hum/terminal.hpp"
#include "hum/text.hpp"
#include "hum/viewport.hpp"

#include <algorithm>
#include <cstdlib>
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

std::size_t previousCharacter(const std::string& value, std::size_t position) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (position == 0) return 0;
    --position;
    while (position > 0 && (static_cast<unsigned char>(value[position]) & 0xc0) == 0x80) --position;
    return position;
}

std::size_t nextCharacter(const std::string& value, std::size_t position) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (position >= value.size()) return value.size();
    ++position;
    while (position < value.size() && (static_cast<unsigned char>(value[position]) & 0xc0) == 0x80) ++position;
    return position;
}

int characterCount(const std::vector<std::string>& lines) {
//---------------------------------------------------------------------------------------------------------------------------------//

    int count = lines.empty() ? 0 : static_cast<int>(lines.size() - 1);
    for (const std::string& line : lines) {
        for (unsigned char character : line) if ((character & 0xc0) != 0x80) ++count;
    }
    return count;
}

std::string joinLines(const std::vector<std::string>& lines) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::ostringstream output;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        if (index > 0) output << '\n';
        output << lines[index];
    }
    return output.str();
}

std::string cursorLine(const std::string& value, std::size_t column, const std::string& cursorColour) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const std::string prefix = ansiColourCode(cursorColour);
    if (column >= value.size()) return value + (prefix.empty() ? " " : prefix + " " + "\x1b[0m");
    const std::size_t next = nextCharacter(value, column);
    const std::string character = value.substr(column, next - column);
    return value.substr(0, column) + (prefix.empty() ? character : prefix + character + "\x1b[0m") +
           value.substr(next);
}

}  // namespace

int runWrite(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    int width = 0;
    int height = 5;
    int characterLimit = 0;
    int maximumLines = 0;
    int timeoutMilliseconds = 0;
    std::string header;
    std::string placeholder = "Write something...";
    std::string prompt = "┃ ";
    std::string value;
    std::string cursorForeground = "212";
    bool showCursorLine = false;
    bool showLineNumbers = false;
    bool showHelp = true;
    bool stripInputAnsi = true;
    try {
        for (std::size_t index = 0; index < args.size(); ++index) {
            const std::string& arg = args[index];
            if (arg == "--help" || arg == "-h") { printWriteHelp(); return 0; }
            if (arg == "--width") width = std::stoi(requireValue(args, index, arg));
            else if (arg == "--height") height = std::stoi(requireValue(args, index, arg));
            else if (arg == "--header") header = requireValue(args, index, arg);
            else if (arg == "--placeholder") placeholder = requireValue(args, index, arg);
            else if (arg == "--prompt") prompt = requireValue(args, index, arg);
            else if (arg == "--value") value = requireValue(args, index, arg);
            else if (arg == "--char-limit") characterLimit = std::stoi(requireValue(args, index, arg));
            else if (arg == "--max-lines") maximumLines = std::stoi(requireValue(args, index, arg));
            else if (arg == "--timeout") timeoutMilliseconds = parseDuration(requireValue(args, index, arg));
            else if (arg == "--cursor.foreground") cursorForeground = requireValue(args, index, arg);
            else if (arg == "--show-cursor-line") showCursorLine = true;
            else if (arg == "--show-line-numbers") showLineNumbers = true;
            else if (arg == "--show-help") showHelp = true;
            else if (arg == "--no-show-help") showHelp = false;
            else if (arg == "--strip-ansi") stripInputAnsi = true;
            else if (arg == "--no-strip-ansi") stripInputAnsi = false;
            else throw std::invalid_argument("unknown option: " + arg);
        }
        if (width < 0 || height < 1 || characterLimit < 0 || maximumLines < 0) {
            throw std::invalid_argument("dimensions and limits cannot be negative");
        }
    } catch (const std::exception& error) {
        std::cerr << "hum write: " << error.what() << '\n';
        return 2;
    }

    if (::isatty(STDIN_FILENO) == 0) {
        std::string piped{std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};
        while (!piped.empty() && (piped.back() == '\n' || piped.back() == '\r')) piped.pop_back();
        if (!piped.empty()) value = stripInputAnsi ? stripAnsi(piped) : piped;
    }
    TerminalSession terminal;
    if (!terminal.interactive() || !terminal.enableRawMode()) {
        std::cout << value << '\n';
        return 0;
    }

    std::vector<std::string> lines = splitLines(value);
    std::size_t row = lines.empty() ? 0 : lines.size() - 1;
    if (lines.empty()) lines.push_back("");
    std::size_t column = lines[row].size();
    LiveRegion region(terminal);
    while (true) {
        const std::size_t start = row >= static_cast<std::size_t>(height)
            ? row - static_cast<std::size_t>(height) + 1 : 0;
        std::vector<std::string> screen;
        if (!header.empty()) screen.push_back(header);
        for (int visibleRow = 0; visibleRow < height; ++visibleRow) {
            const std::size_t lineIndex = start + static_cast<std::size_t>(visibleRow);
            std::string lineValue;
            if (lineIndex < lines.size()) {
                if (lines.size() == 1 && lines.front().empty() && lineIndex == 0) {
                    lineValue = placeholder;
                } else {
                    lineValue = lineIndex == row ? cursorLine(lines[lineIndex], column, cursorForeground)
                                                 : lines[lineIndex];
                }
                if (showCursorLine && lineIndex == row && colorEnabled()) lineValue = "\x1b[7m" + lineValue + "\x1b[0m";
            }
            std::string linePrefix = prompt;
            if (showLineNumbers) linePrefix = std::to_string(lineIndex + 1) + " " + linePrefix;
            const int contentWidth = width > 0 ? width : std::max(10, terminalDimensions().columns - displayWidth(linePrefix));
            screen.push_back(linePrefix + fitText(lineValue, contentWidth));
        }
        if (showHelp) screen.push_back("enter newline • ctrl+d submit • esc cancel");
        region.render(screen);

        const InputEvent event = terminal.readEvent(timeoutMilliseconds);
        if (!event.text.empty()) {
            if (characterLimit == 0 || characterCount(lines) < characterLimit) {
                lines[row].insert(column, event.text);
                column += event.text.size();
            }
        } else if (event.key == Key::Left) {
            if (column > 0) column = previousCharacter(lines[row], column);
            else if (row > 0) { --row; column = lines[row].size(); }
        } else if (event.key == Key::Right) {
            if (column < lines[row].size()) column = nextCharacter(lines[row], column);
            else if (row + 1 < lines.size()) { ++row; column = 0; }
        } else if (event.key == Key::Up) {
            if (row > 0) { --row; column = std::min(column, lines[row].size()); }
        } else if (event.key == Key::Down) {
            if (row + 1 < lines.size()) { ++row; column = std::min(column, lines[row].size()); }
        } else if (event.key == Key::Home) {
            column = 0;
        } else if (event.key == Key::End) {
            column = lines[row].size();
        } else if (event.key == Key::Backspace) {
            if (column > 0) {
                const std::size_t previous = previousCharacter(lines[row], column);
                lines[row].erase(previous, column - previous);
                column = previous;
            } else if (row > 0) {
                column = lines[row - 1].size();
                lines[row - 1] += lines[row];
                lines.erase(lines.begin() + static_cast<long>(row));
                --row;
            }
        } else if (event.key == Key::Delete) {
            if (column < lines[row].size()) lines[row].erase(column, nextCharacter(lines[row], column) - column);
            else if (row + 1 < lines.size()) { lines[row] += lines[row + 1]; lines.erase(lines.begin() + static_cast<long>(row + 1)); }
        } else if (event.key == Key::Enter) {
            if (maximumLines == 0 || static_cast<int>(lines.size()) < maximumLines) {
                const std::string remainder = lines[row].substr(column);
                lines[row].erase(column);
                lines.insert(lines.begin() + static_cast<long>(row + 1), remainder);
                ++row;
                column = 0;
            }
        } else if (event.key == Key::Submit) {
            region.clear();
            std::cout << joinLines(lines) << '\n';
            return 0;
        } else if (event.key == Key::Escape || event.key == Key::Interrupt ||
                   event.key == Key::EndOfInput || event.key == Key::Timeout) {
            return 1;
        }
    }
}

void printWriteHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum write [options]\n\n"
        "Edit multi-line text and submit it with Ctrl+D.\n\n"
        "Options:\n"
        "  --width N                 Text area width\n"
        "  --height N                Visible line count\n"
        "  --header TEXT             Heading above the editor\n"
        "  --placeholder TEXT        Empty editor placeholder\n"
        "  --prompt TEXT             Line prefix\n"
        "  --value TEXT              Initial value\n"
        "  --char-limit N            Maximum characters\n"
        "  --max-lines N             Maximum lines\n"
        "  --show-cursor-line        Highlight the active line\n"
        "  --show-line-numbers       Display line numbers\n"
        "  --cursor.foreground C     Cursor colour\n"
        "  --timeout DURATION        Abort after an idle duration\n";
}

}  // namespace hum

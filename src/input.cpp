#include "hum/input.hpp"

#include "hum/style.hpp"
#include "hum/terminal.hpp"
#include "hum/text.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <iterator>
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

int characterCount(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    int count = 0;
    for (unsigned char character : value) {
        if ((character & 0xc0) != 0x80) ++count;
    }
    return count;
}

std::string masked(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    return std::string(static_cast<std::size_t>(characterCount(value)), '*');
}

std::string suffixWithinWidth(const std::string& value, int width) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (width <= 0 || displayWidth(value) <= width) return value;
    std::size_t start = value.size();
    int used = 0;
    while (start > 0) {
        const std::size_t previous = previousCharacter(value, start);
        const std::string character = value.substr(previous, start - previous);
        const int characterWidth = displayWidth(character);
        if (used + characterWidth > width) break;
        used += characterWidth;
        start = previous;
    }
    return value.substr(start);
}

std::string prefixWithinWidth(const std::string& value, int width) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (width <= 0 || displayWidth(value) <= width) return value;
    std::size_t end = 0;
    int used = 0;
    while (end < value.size()) {
        const std::size_t next = nextCharacter(value, end);
        const std::string character = value.substr(end, next - end);
        const int characterWidth = displayWidth(character);
        if (used + characterWidth > width) break;
        used += characterWidth;
        end = next;
    }
    return value.substr(0, end);
}

std::string applyForeground(const std::string& value, const std::string& colour) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const std::string prefix = ansiColourCode(colour);
    return prefix.empty() ? value : prefix + value + "\x1b[0m";
}

void readEnvironmentString(const char* name, std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const char* environmentValue = std::getenv(name);
    if (environmentValue != nullptr) value = environmentValue;
}

bool parseBoolean(const std::string& value, const std::string& name) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string normalised = value;
    std::transform(normalised.begin(), normalised.end(), normalised.begin(), [](unsigned char character) {
//---------------------------------------------------------------------------------------------------------------------------------//

        return static_cast<char>(std::tolower(character));
    });
    if (normalised == "1" || normalised == "true" || normalised == "yes" || normalised == "on") return true;
    if (normalised == "0" || normalised == "false" || normalised == "no" || normalised == "off") return false;
    throw std::invalid_argument(name + " must be true or false");
}

void readEnvironmentBoolean(const char* name, bool& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const char* environmentValue = std::getenv(name);
    if (environmentValue != nullptr) value = parseBoolean(environmentValue, name);
}

}  // namespace

int runInput(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string placeholder = "Type something...";
    std::string prompt = "> ";
    std::string header;
    std::string value;
    std::string cursorForeground = "212";
    std::string promptForeground;
    std::string placeholderForeground = "240";
    int characterLimit = 400;
    int width = 0;
    int timeoutMilliseconds = 0;
    bool password = false;
    bool showHelp = true;
    bool stripInputAnsi = true;

    try {
        readEnvironmentString("HUM_INPUT_PLACEHOLDER", placeholder);
        readEnvironmentString("HUM_INPUT_PROMPT", prompt);
        readEnvironmentString("HUM_INPUT_HEADER", header);
        readEnvironmentString("HUM_INPUT_VALUE", value);
        readEnvironmentString("HUM_INPUT_CURSOR_FOREGROUND", cursorForeground);
        readEnvironmentString("HUM_INPUT_PROMPT_FOREGROUND", promptForeground);
        readEnvironmentString("HUM_INPUT_PLACEHOLDER_FOREGROUND", placeholderForeground);
        if (const char* environmentValue = std::getenv("HUM_INPUT_CHAR_LIMIT")) {
            characterLimit = std::stoi(environmentValue);
        }
        if (const char* environmentValue = std::getenv("HUM_INPUT_WIDTH")) {
            width = std::stoi(environmentValue);
        }
        if (const char* environmentValue = std::getenv("HUM_INPUT_TIMEOUT")) {
            timeoutMilliseconds = parseDuration(environmentValue);
        }
        readEnvironmentBoolean("HUM_INPUT_PASSWORD", password);
        readEnvironmentBoolean("HUM_INPUT_SHOW_HELP", showHelp);
        readEnvironmentBoolean("HUM_INPUT_STRIP_ANSI", stripInputAnsi);

        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--help" || arg == "-h") { printInputHelp(); return 0; }
            if (arg == "--placeholder") placeholder = requireValue(args, i, arg);
            else if (arg == "--prompt") prompt = requireValue(args, i, arg);
            else if (arg == "--cursor.foreground") cursorForeground = requireValue(args, i, arg);
            else if (arg == "--prompt.foreground") promptForeground = requireValue(args, i, arg);
            else if (arg == "--placeholder.foreground") placeholderForeground = requireValue(args, i, arg);
            else if (arg == "--header") header = requireValue(args, i, arg);
            else if (arg == "--value") value = requireValue(args, i, arg);
            else if (arg == "--char-limit") characterLimit = std::stoi(requireValue(args, i, arg));
            else if (arg == "--width") width = std::stoi(requireValue(args, i, arg));
            else if (arg == "--timeout") timeoutMilliseconds = parseDuration(requireValue(args, i, arg));
            else if (arg == "--password") password = true;
            else if (arg == "--show-help") showHelp = true;
            else if (arg == "--no-show-help") showHelp = false;
            else if (arg == "--strip-ansi") stripInputAnsi = true;
            else if (arg == "--no-strip-ansi") stripInputAnsi = false;
            else throw std::invalid_argument("unknown option: " + arg);
        }
        if (characterLimit < 0 || width < 0) throw std::invalid_argument("limits cannot be negative");
    } catch (const std::exception& error) {
        std::cerr << "hum input: " << error.what() << '\n';
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

    if (!header.empty()) terminal.write(header + "\n");
    if (showHelp) terminal.write("enter submit • esc cancel\n");
    std::size_t cursor = value.size();
    while (true) {
        const std::string rawLeft = value.substr(0, cursor);
        const std::string rawRight = value.substr(cursor);
        const std::string shownLeft = password ? masked(rawLeft) : rawLeft;
        const std::string shownRight = password ? masked(rawRight) : rawRight;
        std::string visibleLeft = shownLeft;
        std::string visibleRight = shownRight;
        if (width > 0 && displayWidth(visibleLeft) + displayWidth(visibleRight) > width) {
            visibleLeft = suffixWithinWidth(visibleLeft, width);
            visibleRight = prefixWithinWidth(visibleRight, width - displayWidth(visibleLeft));
        }
        const std::string shown = visibleLeft + visibleRight;
        std::string line;
        int moveLeft = 0;
        if (shown.empty()) {
            line = applyForeground(placeholder, placeholderForeground);
            moveLeft = displayWidth(placeholder);
        } else {
            line = visibleLeft;
            if (!visibleRight.empty()) {
                const std::size_t cursorCharacterEnd = nextCharacter(visibleRight, 0);
                line += applyForeground(visibleRight.substr(0, cursorCharacterEnd), cursorForeground);
                line += visibleRight.substr(cursorCharacterEnd);
                moveLeft = displayWidth(visibleRight);
            } else {
                line += applyForeground(" ", cursorForeground);
                moveLeft = 1;
            }
        }
        terminal.write("\r\x1b[2K" + applyForeground(prompt, promptForeground) + line);
        if (moveLeft > 0) terminal.write("\x1b[" + std::to_string(moveLeft) + "D");

        const InputEvent event = terminal.readEvent(timeoutMilliseconds);
        if (!event.text.empty()) {
            if (characterLimit == 0 || characterCount(value) < characterLimit) {
                value.insert(cursor, event.text);
                cursor += event.text.size();
            }
        } else if (event.key == Key::Left) {
            cursor = previousCharacter(value, cursor);
        } else if (event.key == Key::Right) {
            cursor = nextCharacter(value, cursor);
        } else if (event.key == Key::Home) {
            cursor = 0;
        } else if (event.key == Key::End) {
            cursor = value.size();
        } else if (event.key == Key::Backspace && cursor > 0) {
            const std::size_t previous = previousCharacter(value, cursor);
            value.erase(previous, cursor - previous);
            cursor = previous;
        } else if (event.key == Key::Delete && cursor < value.size()) {
            value.erase(cursor, nextCharacter(value, cursor) - cursor);
        } else if (event.key == Key::Enter) {
            terminal.write("\r\x1b[2K");
            std::cout << value << '\n';
            return 0;
        } else if (event.key == Key::Escape || event.key == Key::Interrupt ||
                   event.key == Key::EndOfInput || event.key == Key::Timeout) {
            terminal.write("\r\x1b[2K");
            return 1;
        }
    }
}

void printInputHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum input [options]\n\n"
        "Prompt for a single line and print it to stdout.\n\n"
        "Options:\n"
        "  --placeholder TEXT       Placeholder when empty\n"
        "  --prompt TEXT            Prompt prefix (default: > )\n"
        "  --cursor.foreground C    Cursor foreground colour\n"
        "  --prompt.foreground C    Prompt foreground colour\n"
        "  --placeholder.foreground C  Placeholder foreground colour\n"
        "  --header TEXT            Heading above the input\n"
        "  --value TEXT             Initial value\n"
        "  --char-limit N           Maximum characters; 0 is unlimited\n"
        "  --width N                Maximum displayed width\n"
        "  --password               Mask entered characters\n"
        "  --timeout DURATION       Abort after an idle duration\n"
        "  --[no-]show-help         Toggle keyboard hints\n"
        "  --[no-]strip-ansi        Strip ANSI from piped input\n\n"
        "Environment:\n"
        "  HUM_INPUT_PLACEHOLDER, HUM_INPUT_PROMPT, HUM_INPUT_HEADER, HUM_INPUT_VALUE\n"
        "  HUM_INPUT_CURSOR_FOREGROUND, HUM_INPUT_PROMPT_FOREGROUND\n"
        "  HUM_INPUT_PLACEHOLDER_FOREGROUND, HUM_INPUT_CHAR_LIMIT, HUM_INPUT_WIDTH\n"
        "  HUM_INPUT_PASSWORD, HUM_INPUT_SHOW_HELP, HUM_INPUT_TIMEOUT\n"
        "  HUM_INPUT_STRIP_ANSI\n";
}

}  // namespace hum

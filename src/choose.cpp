#include "hum/choose.hpp"

#include "hum/terminal.hpp"
#include "hum/text.hpp"

#include <algorithm>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace hum {
namespace {

struct ChooseOptions {
    int limit = 1;
    bool noLimit = false;
    bool ordered = false;
    int height = 10;
    std::string cursor = "> ";
    std::string header = "Choose:";
    std::string selectedPrefix = "✓ ";
    std::string unselectedPrefix = "• ";
    bool showHelp = true;
    bool selectIfOne = false;
    int timeoutMilliseconds = 0;
    std::string inputDelimiter = "\n";
    std::string outputDelimiter = "\n";
    std::string labelDelimiter;
    bool stripInputAnsi = true;
    std::vector<std::string> initiallySelected;
};

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

std::string decodeDelimiter(std::string value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string result;
    for (std::size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '\\' && i + 1 < value.size()) {
            if (value[i + 1] == 'n') { result.push_back('\n'); ++i; continue; }
            if (value[i + 1] == 't') { result.push_back('\t'); ++i; continue; }
            if (value[i + 1] == '0') { result.push_back('\0'); ++i; continue; }
        }
        result.push_back(value[i]);
    }
    return result;
}

std::vector<std::string> split(const std::string& input, const std::string& delimiter) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (delimiter.empty()) throw std::invalid_argument("input delimiter cannot be empty");
    std::vector<std::string> values;
    std::size_t start = 0;
    while (start <= input.size()) {
        const std::size_t end = input.find(delimiter, start);
        if (end == std::string::npos) {
            if (start < input.size()) values.push_back(input.substr(start));
            break;
        }
        values.push_back(input.substr(start, end - start));
        start = end + delimiter.size();
    }
    return values;
}

std::string outputValue(const std::string& option, const std::string& labelDelimiter) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (labelDelimiter.empty()) return option;
    const std::size_t position = option.find(labelDelimiter);
    return position == std::string::npos ? option : option.substr(position + labelDelimiter.size());
}

std::string displayLabel(const std::string& option, const std::string& labelDelimiter) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (labelDelimiter.empty()) return option;
    const std::size_t position = option.find(labelDelimiter);
    return position == std::string::npos ? option : option.substr(0, position);
}

void clearBlock(TerminalSession& terminal, int lines) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (lines <= 0) return;
    terminal.write("\x1b[" + std::to_string(lines) + "A");
    for (int i = 0; i < lines; ++i) terminal.write("\r\x1b[2K\x1b[1B");
}

int renderList(TerminalSession& terminal, const std::vector<std::string>& options,
               const std::vector<bool>& selected, std::size_t cursorPosition,
               const ChooseOptions& settings, int previousLines) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (previousLines > 0) terminal.write("\x1b[" + std::to_string(previousLines) + "A");
    std::vector<std::string> lines;
    if (!settings.header.empty()) lines.push_back(settings.header);
    const std::size_t visibleHeight = static_cast<std::size_t>(std::max(1, settings.height));
    std::size_t start = cursorPosition >= visibleHeight ? cursorPosition - visibleHeight + 1 : 0;
    if (start + visibleHeight > options.size() && options.size() > visibleHeight) start = options.size() - visibleHeight;
    const std::size_t end = std::min(options.size(), start + visibleHeight);
    for (std::size_t i = start; i < end; ++i) {
        const bool atCursor = i == cursorPosition;
        std::string prefix = atCursor ? settings.cursor : "  ";
        if (settings.limit != 1 || settings.noLimit) {
            prefix += selected[i] ? settings.selectedPrefix : settings.unselectedPrefix;
        }
        std::string line = prefix + displayLabel(options[i], settings.labelDelimiter);
        if (atCursor && colorEnabled()) line = "\x1b[38;5;212m" + line + "\x1b[0m";
        lines.push_back(line);
    }
    if (settings.showHelp) {
        lines.push_back(settings.limit == 1 && !settings.noLimit
            ? "↑/↓ navigate • enter select • esc cancel"
            : "↑/↓ navigate • space toggle • enter submit");
    }
    const int lineCount = static_cast<int>(lines.size());
    const int totalLines = std::max(previousLines, lineCount);
    for (int i = 0; i < totalLines; ++i) {
        terminal.write("\r\x1b[2K");
        if (i < lineCount) terminal.write(lines[static_cast<std::size_t>(i)]);
        terminal.write("\n");
    }
    return totalLines;
}

}  // namespace

int runChoose(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    ChooseOptions settings;
    std::vector<std::string> options;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--help" || arg == "-h") { printChooseHelp(); return 0; }
            if (arg == "--limit") settings.limit = std::stoi(requireValue(args, i, arg));
            else if (arg == "--no-limit") settings.noLimit = true;
            else if (arg == "--ordered") settings.ordered = true;
            else if (arg == "--height") settings.height = std::stoi(requireValue(args, i, arg));
            else if (arg == "--cursor") settings.cursor = requireValue(args, i, arg);
            else if (arg == "--header") settings.header = requireValue(args, i, arg);
            else if (arg == "--selected-prefix") settings.selectedPrefix = requireValue(args, i, arg);
            else if (arg == "--unselected-prefix") settings.unselectedPrefix = requireValue(args, i, arg);
            else if (arg == "--selected") settings.initiallySelected.push_back(requireValue(args, i, arg));
            else if (arg == "--select-if-one") settings.selectIfOne = true;
            else if (arg == "--input-delimiter") settings.inputDelimiter = decodeDelimiter(requireValue(args, i, arg));
            else if (arg == "--output-delimiter") settings.outputDelimiter = decodeDelimiter(requireValue(args, i, arg));
            else if (arg == "--label-delimiter") settings.labelDelimiter = requireValue(args, i, arg);
            else if (arg == "--timeout") settings.timeoutMilliseconds = parseDuration(requireValue(args, i, arg));
            else if (arg == "--show-help") settings.showHelp = true;
            else if (arg == "--no-show-help") settings.showHelp = false;
            else if (arg == "--strip-ansi") settings.stripInputAnsi = true;
            else if (arg == "--no-strip-ansi") settings.stripInputAnsi = false;
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option: " + arg);
            else options.push_back(arg);
        }
        if (settings.limit < 1 && !settings.noLimit) throw std::invalid_argument("limit must be at least one");
        if (settings.height < 1) throw std::invalid_argument("height must be at least one");

        if (options.empty() && ::isatty(STDIN_FILENO) == 0) {
            std::string input{std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};
            if (settings.stripInputAnsi) input = stripAnsi(input);
            options = split(input, settings.inputDelimiter);
        }
        if (options.empty()) throw std::invalid_argument("no options supplied");

        if (options.size() == 1 && settings.selectIfOne) {
            std::cout << outputValue(options.front(), settings.labelDelimiter) << '\n';
            return 0;
        }
    } catch (const std::exception& error) {
        std::cerr << "hum choose: " << error.what() << '\n';
        return 2;
    }

    TerminalSession terminal;
    if (!terminal.interactive() || !terminal.enableRawMode()) {
        std::cerr << "hum choose: an interactive terminal is required for multiple options\n";
        return 2;
    }

    std::vector<bool> selected(options.size(), false);
    std::vector<std::size_t> selectionOrder;
    for (std::size_t i = 0; i < options.size(); ++i) {
        if (std::find(settings.initiallySelected.begin(), settings.initiallySelected.end(), "*") !=
                settings.initiallySelected.end() ||
            std::find(settings.initiallySelected.begin(), settings.initiallySelected.end(), options[i]) !=
                settings.initiallySelected.end()) {
            selected[i] = true;
            selectionOrder.push_back(i);
        }
    }

    std::size_t cursorPosition = 0;
    int renderedLines = 0;
    terminal.write("\x1b[?25l");
    while (true) {
        renderedLines = renderList(terminal, options, selected, cursorPosition, settings, renderedLines);
        const InputEvent event = terminal.readEvent(settings.timeoutMilliseconds);
        if (event.key == Key::Up || event.text == "k") {
            cursorPosition = cursorPosition == 0 ? options.size() - 1 : cursorPosition - 1;
        } else if (event.key == Key::Down || event.text == "j") {
            cursorPosition = (cursorPosition + 1) % options.size();
        } else if ((event.text == " " || event.key == Key::Tab) &&
                   (settings.limit != 1 || settings.noLimit)) {
            if (selected[cursorPosition]) {
                selected[cursorPosition] = false;
                selectionOrder.erase(std::remove(selectionOrder.begin(), selectionOrder.end(), cursorPosition),
                                     selectionOrder.end());
            } else {
                const int selectedCount = static_cast<int>(std::count(selected.begin(), selected.end(), true));
                if (settings.noLimit || selectedCount < settings.limit) {
                    selected[cursorPosition] = true;
                    selectionOrder.push_back(cursorPosition);
                }
            }
        } else if (event.key == Key::Enter || event.key == Key::Timeout) {
            if (settings.limit == 1 && !settings.noLimit) {
                std::fill(selected.begin(), selected.end(), false);
                selected[cursorPosition] = true;
                selectionOrder.clear();
                selectionOrder.push_back(cursorPosition);
            }
            break;
        } else if (event.key == Key::Escape || event.key == Key::Interrupt ||
                   event.key == Key::EndOfInput) {
            clearBlock(terminal, renderedLines);
            terminal.write("\x1b[?25h");
            return 1;
        }
    }
    clearBlock(terminal, renderedLines);
    terminal.write("\x1b[?25h");

    std::vector<std::size_t> result;
    if (settings.ordered) {
        for (std::size_t index : selectionOrder) if (selected[index]) result.push_back(index);
        if (settings.limit == 1 && !settings.noLimit && result.empty()) result.push_back(cursorPosition);
    } else {
        for (std::size_t i = 0; i < selected.size(); ++i) if (selected[i]) result.push_back(i);
    }
    for (std::size_t i = 0; i < result.size(); ++i) {
        if (i > 0) std::cout << settings.outputDelimiter;
        std::cout << outputValue(options[result[i]], settings.labelDelimiter);
    }
    if (settings.outputDelimiter.empty() || settings.outputDelimiter.back() != '\0') std::cout << '\n';
    return 0;
}

void printChooseHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum choose [options] [items ...]\n\n"
        "Choose one or more items from arguments or delimited stdin.\n\n"
        "Options:\n"
        "  --limit N                  Maximum selections (default: 1)\n"
        "  --no-limit                 Allow unlimited selections\n"
        "  --ordered                  Output in selection order\n"
        "  --height N                 Visible list height\n"
        "  --header TEXT              Heading above the list\n"
        "  --cursor TEXT              Cursor prefix\n"
        "  --selected VALUE           Initially select a value or *\n"
        "  --select-if-one            Immediately return a sole option\n"
        "  --input-delimiter TEXT     Stdin delimiter (default: \\n)\n"
        "  --output-delimiter TEXT    Output delimiter (default: \\n)\n"
        "  --label-delimiter TEXT     Split display labels from output values\n"
        "  --timeout DURATION         Submit after an idle duration\n"
        "  --[no-]show-help           Toggle keyboard hints\n";
}

}  // namespace hum

#include "hum/style.hpp"

#include "hum/terminal.hpp"
#include "hum/text.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace hum {
namespace {

struct BorderChars {
    std::string topLeft, top, topRight, left, right, bottomLeft, bottom, bottomRight;
};

std::string colorCode(const std::string& value, bool background) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (value.empty() || !colorEnabled()) {
        return "";
    }
    const int base = background ? 48 : 38;
    if ((value.size() == 4 || value.size() == 7) && value[0] == '#') {
        const bool shorthand = value.size() == 4;
        const int red = std::stoi(value.substr(1, shorthand ? 1 : 2), nullptr, 16) * (shorthand ? 17 : 1);
        const int green = std::stoi(value.substr(shorthand ? 2 : 3, shorthand ? 1 : 2), nullptr, 16) *
                          (shorthand ? 17 : 1);
        const int blue = std::stoi(value.substr(shorthand ? 3 : 5, shorthand ? 1 : 2), nullptr, 16) *
                         (shorthand ? 17 : 1);
        return "\x1b[" + std::to_string(base) + ";2;" + std::to_string(red) + ";" +
               std::to_string(green) + ";" + std::to_string(blue) + "m";
    }

    static const std::unordered_map<std::string, int> named = {
        {"black", 0}, {"red", 1}, {"green", 2}, {"yellow", 3},
        {"blue", 4}, {"magenta", 5}, {"cyan", 6}, {"white", 7},
    };
    int index = -1;
    const auto found = named.find(value);
    if (found != named.end()) {
        index = found->second;
    } else {
        std::size_t consumed = 0;
        index = std::stoi(value, &consumed);
        if (consumed != value.size() || index < 0 || index > 255) {
            throw std::invalid_argument("colour must be 0-255, a basic name, or #RRGGBB");
        }
    }
    return "\x1b[" + std::to_string(base) + ";5;" + std::to_string(index) + "m";
}

std::string textPrefix(const StyleOptions& options) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string prefix = colorCode(options.foreground, false) + colorCode(options.background, true);
    if (colorEnabled()) {
        if (options.bold) prefix += "\x1b[1m";
        if (options.faint) prefix += "\x1b[2m";
        if (options.italic) prefix += "\x1b[3m";
        if (options.underline) prefix += "\x1b[4m";
        if (options.strikethrough) prefix += "\x1b[9m";
    }
    return prefix;
}

std::string applyAnsi(const std::string& value, const std::string& prefix) {
//---------------------------------------------------------------------------------------------------------------------------------//

    return prefix.empty() ? value : prefix + value + "\x1b[0m";
}

BorderChars borderFor(const std::string& name) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (name == "normal") return {"┌", "─", "┐", "│", "│", "└", "─", "┘"};
    if (name == "rounded") return {"╭", "─", "╮", "│", "│", "╰", "─", "╯"};
    if (name == "thick") return {"┏", "━", "┓", "┃", "┃", "┗", "━", "┛"};
    if (name == "double") return {"╔", "═", "╗", "║", "║", "╚", "═", "╝"};
    if (name == "hidden") return {" ", " ", " ", " ", " ", " ", " ", " "};
    if (name == "none") return {};
    throw std::invalid_argument("unknown border style: " + name);
}

std::string repeat(const std::string& value, int count) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string result;
    for (int i = 0; i < count; ++i) result += value;
    return result;
}

std::string spaces(int count) {
//---------------------------------------------------------------------------------------------------------------------------------//

    return std::string(static_cast<std::size_t>(std::max(0, count)), ' ');
}

std::string requireValue(const std::vector<std::string>& args, std::size_t& index,
                         const std::string& option) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (++index >= args.size()) {
        throw std::invalid_argument(option + " requires a value");
    }
    return args[index];
}

}  // namespace

std::string ansiColourCode(const std::string& value, bool background) {
//---------------------------------------------------------------------------------------------------------------------------------//

    return colorCode(value, background);
}

std::string renderStyle(const std::string& original, const StyleOptions& options) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string input = options.stripInputAnsi ? stripAnsi(original) : original;
    std::vector<std::string> lines = splitLines(input);
    if (options.trimLines) {
        for (std::string& line : lines) line = trim(line);
    }

    const auto padding = parseBox(options.padding);
    const auto margin = parseBox(options.margin);
    const bool hasBorder = options.border != "none";
    const BorderChars border = borderFor(options.border);

    int naturalWidth = 0;
    for (const std::string& line : lines) naturalWidth = std::max(naturalWidth, displayWidth(line));
    const int contentWidth = options.width > 0 ? std::max(options.width, naturalWidth) : naturalWidth;
    const int requestedHeight = options.height > 0 ? options.height : static_cast<int>(lines.size());
    const int contentHeight = std::max(requestedHeight, static_cast<int>(lines.size()));
    const int innerWidth = padding[3] + contentWidth + padding[1];

    int extraVertical = contentHeight - static_cast<int>(lines.size());
    int leadingVertical = 0;
    if (options.align == "middle") leadingVertical = extraVertical / 2;
    if (options.align == "bottom") leadingVertical = extraVertical;
    std::vector<std::string> content;
    content.insert(content.end(), padding[0] + leadingVertical, "");
    content.insert(content.end(), lines.begin(), lines.end());
    content.insert(content.end(), padding[2] + extraVertical - leadingVertical, "");

    const std::string prefix = textPrefix(options);
    const std::string borderPrefix = colorCode(options.borderForeground, false) +
                                     colorCode(options.borderBackground, true);
    std::vector<std::string> rendered;
    if (hasBorder) {
        rendered.push_back(applyAnsi(border.topLeft + repeat(border.top, innerWidth) + border.topRight,
                                     borderPrefix));
    }
    for (std::size_t row = 0; row < content.size(); ++row) {
        const bool paddingRow = row < static_cast<std::size_t>(padding[0]) ||
            row >= content.size() - static_cast<std::size_t>(padding[2]);
        std::string value = paddingRow ? "" : content[row];
        const int remaining = std::max(0, contentWidth - displayWidth(value));
        int left = 0;
        if (options.align == "center") left = remaining / 2;
        if (options.align == "right") left = remaining;
        const std::string body = spaces(padding[3] + left) + applyAnsi(value, prefix) +
                                 spaces(padding[1] + remaining - left);
        rendered.push_back(hasBorder
            ? applyAnsi(border.left, borderPrefix) + body + applyAnsi(border.right, borderPrefix)
            : body);
    }
    if (hasBorder) {
        rendered.push_back(applyAnsi(border.bottomLeft + repeat(border.bottom, innerWidth) +
                                     border.bottomRight, borderPrefix));
    }

    const int outerWidth = innerWidth + (hasBorder ? 2 : 0);
    std::ostringstream output;
    for (int i = 0; i < margin[0]; ++i) output << spaces(margin[3] + outerWidth + margin[1]) << '\n';
    for (std::size_t i = 0; i < rendered.size(); ++i) {
        output << spaces(margin[3]) << rendered[i] << spaces(margin[1]);
        if (i + 1 < rendered.size() || margin[2] > 0) output << '\n';
    }
    for (int i = 0; i < margin[2]; ++i) {
        output << spaces(margin[3] + outerWidth + margin[1]);
        if (i + 1 < margin[2]) output << '\n';
    }
    return output.str();
}

int runStyle(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    StyleOptions options;
    std::vector<std::string> text;
    bool positional = false;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (positional || arg.empty() || arg[0] != '-') text.push_back(arg);
            else if (arg == "--") positional = true;
            else if (arg == "--help" || arg == "-h") { printStyleHelp(); return 0; }
            else if (arg == "--foreground") options.foreground = requireValue(args, i, arg);
            else if (arg == "--background") options.background = requireValue(args, i, arg);
            else if (arg == "--border") options.border = requireValue(args, i, arg);
            else if (arg == "--border-foreground") options.borderForeground = requireValue(args, i, arg);
            else if (arg == "--border-background") options.borderBackground = requireValue(args, i, arg);
            else if (arg == "--align") options.align = requireValue(args, i, arg);
            else if (arg == "--width") options.width = std::stoi(requireValue(args, i, arg));
            else if (arg == "--height") options.height = std::stoi(requireValue(args, i, arg));
            else if (arg == "--margin") options.margin = requireValue(args, i, arg);
            else if (arg == "--padding") options.padding = requireValue(args, i, arg);
            else if (arg == "--bold") options.bold = true;
            else if (arg == "--faint") options.faint = true;
            else if (arg == "--italic") options.italic = true;
            else if (arg == "--underline") options.underline = true;
            else if (arg == "--strikethrough") options.strikethrough = true;
            else if (arg == "--trim") options.trimLines = true;
            else if (arg == "--strip-ansi") options.stripInputAnsi = true;
            else if (arg == "--no-strip-ansi") options.stripInputAnsi = false;
            else throw std::invalid_argument("unknown option: " + arg);
        }

        std::string input;
        if (!text.empty()) {
            for (std::size_t i = 0; i < text.size(); ++i) {
                if (i > 0) input += '\n';
                input += text[i];
            }
        } else {
            input.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
            if (!input.empty() && input.back() == '\n') input.pop_back();
        }
        std::cout << renderStyle(input, options) << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hum style: " << error.what() << '\n';
        return 2;
    }
}

void printStyleHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum style [options] [text ...]\n\n"
        "Apply colour, borders, spacing and text attributes. With no text, read stdin.\n\n"
        "Options:\n"
        "  --foreground COLOR          Text colour (0-255, name, or #RRGGBB)\n"
        "  --background COLOR          Text background colour\n"
        "  --border STYLE              none, hidden, normal, rounded, thick, double\n"
        "  --border-foreground COLOR   Border colour\n"
        "  --border-background COLOR   Border background colour\n"
        "  --align POSITION            left, center, right, top, middle, bottom\n"
        "  --width N, --height N       Minimum content dimensions\n"
        "  --margin \"T R B L\"         Outer spacing (CSS shorthand)\n"
        "  --padding \"T R B L\"        Inner spacing (CSS shorthand)\n"
        "  --bold --faint --italic --underline --strikethrough\n"
        "  --trim                      Trim each input line\n"
        "  --[no-]strip-ansi           Strip input ANSI sequences (default: on)\n";
}

}  // namespace hum

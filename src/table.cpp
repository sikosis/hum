#include "hum/table.hpp"

#include "hum/terminal.hpp"
#include "hum/text.hpp"
#include "hum/viewport.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <unistd.h>

namespace hum {
namespace {

struct Borders {
    std::string topLeft, topJoin, topRight, horizontal;
    std::string leftJoin, centre, rightJoin, vertical;
    std::string bottomLeft, bottomJoin, bottomRight;
};

Borders bordersFor(const std::string& style) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (style == "rounded") return {"╭", "┬", "╮", "─", "├", "┼", "┤", "│", "╰", "┴", "╯"};
    if (style == "normal") return {"┌", "┬", "┐", "─", "├", "┼", "┤", "│", "└", "┴", "┘"};
    if (style == "thick") return {"┏", "┳", "┓", "━", "┣", "╋", "┫", "┃", "┗", "┻", "┛"};
    if (style == "double") return {"╔", "╦", "╗", "═", "╠", "╬", "╣", "║", "╚", "╩", "╝"};
    if (style == "hidden") return {" ", " ", " ", " ", " ", " ", " ", " ", " ", " ", " "};
    if (style == "none") return {};
    throw std::invalid_argument("unknown border style: " + style);
}

std::string repeat(const std::string& value, int count) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string result;
    for (int index = 0; index < count; ++index) result += value;
    return result;
}

std::vector<std::vector<std::string>> parseDelimited(const std::string& input, char separator,
                                                      bool lazyQuotes) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::vector<std::vector<std::string>> rows;
    std::vector<std::string> row;
    std::string field;
    bool quoted = false;
    for (std::size_t index = 0; index < input.size(); ++index) {
        const char character = input[index];
        if (quoted) {
            if (character == '"') {
                if (index + 1 < input.size() && input[index + 1] == '"') { field.push_back('"'); ++index; }
                else quoted = false;
            } else {
                field.push_back(character);
            }
        } else if (character == '"' && (field.empty() || lazyQuotes)) {
            quoted = true;
        } else if (character == separator) {
            row.push_back(field);
            field.clear();
        } else if (character == '\n') {
            row.push_back(field);
            field.clear();
            rows.push_back(row);
            row.clear();
        } else if (character != '\r') {
            field.push_back(character);
        }
    }
    if (!field.empty() || !row.empty()) {
        row.push_back(field);
        rows.push_back(row);
    }
    return rows;
}

std::vector<std::string> splitComma(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::vector<std::string> result;
    std::size_t start = 0;
    while (start <= value.size()) {
        const std::size_t end = value.find(',', start);
        if (end == std::string::npos) { result.push_back(value.substr(start)); break; }
        result.push_back(value.substr(start, end - start));
        start = end + 1;
    }
    return result;
}

std::string borderLine(const Borders& border, const std::vector<int>& widths,
                       const std::string& left, const std::string& join, const std::string& right) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (border.vertical.empty()) return "";
    std::string result = left;
    for (std::size_t column = 0; column < widths.size(); ++column) {
        if (column > 0) result += join;
        result += repeat(border.horizontal, widths[column] + 2);
    }
    return result + right;
}

std::string renderRow(const std::vector<std::string>& row, const std::vector<int>& widths,
                      const Borders& border) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string result = border.vertical.empty() ? "" : border.vertical;
    for (std::size_t column = 0; column < widths.size(); ++column) {
        if (column > 0) result += border.vertical.empty() ? "  " : border.vertical;
        const std::string value = column < row.size() ? row[column] : "";
        result += " " + fitText(value, widths[column]) + " ";
    }
    if (!border.vertical.empty()) result += border.vertical;
    return result;
}

std::vector<std::string> renderTable(const std::vector<std::string>& headers,
                                     const std::vector<std::vector<std::string>>& rows,
                                     const std::vector<int>& widths, const std::string& borderStyle,
                                     std::size_t selected, std::size_t start, std::size_t count,
                                     bool interactive) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const Borders border = bordersFor(borderStyle);
    std::vector<std::string> output;
    const std::string top = borderLine(border, widths, border.topLeft, border.topJoin, border.topRight);
    if (!top.empty()) output.push_back(top);
    if (!headers.empty()) {
        output.push_back(renderRow(headers, widths, border));
        const std::string middle = borderLine(border, widths, border.leftJoin, border.centre, border.rightJoin);
        if (!middle.empty()) output.push_back(middle);
    }
    const std::size_t end = std::min(rows.size(), start + count);
    for (std::size_t index = start; index < end; ++index) {
        std::string line = renderRow(rows[index], widths, border);
        if (interactive && index == selected) {
            line = colorEnabled() ? "\x1b[38;5;212m" + line + "\x1b[0m" : "> " + line;
        }
        output.push_back(line);
    }
    const std::string bottom = borderLine(border, widths, border.bottomLeft, border.bottomJoin, border.bottomRight);
    if (!bottom.empty()) output.push_back(bottom);
    return output;
}

std::string requireValue(const std::vector<std::string>& args, std::size_t& index,
                         const std::string& option) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (++index >= args.size()) throw std::invalid_argument(option + " requires a value");
    return args[index];
}

}  // namespace

int runTable(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    char separator = ',';
    std::vector<std::string> headers;
    std::vector<int> widths;
    int height = 0;
    bool staticPrint = false;
    bool showHelp = true;
    bool lazyQuotes = false;
    int fieldsPerRecord = 0;
    int returnColumn = 0;
    std::string file;
    std::string border = "rounded";
    try {
        for (std::size_t index = 0; index < args.size(); ++index) {
            const std::string& arg = args[index];
            if (arg == "--help" || arg == "-h") { printTableHelp(); return 0; }
            if (arg == "--separator" || arg == "-s") {
                const std::string value = requireValue(args, index, arg);
                if (value.empty()) throw std::invalid_argument("separator cannot be empty");
                separator = value == "\\t" ? '\t' : value.front();
            } else if (arg == "--columns" || arg == "-c") {
                headers = splitComma(requireValue(args, index, arg));
            } else if (arg == "--widths" || arg == "-w") {
                for (const std::string& value : splitComma(requireValue(args, index, arg))) widths.push_back(std::stoi(value));
            } else if (arg == "--height") height = std::stoi(requireValue(args, index, arg));
            else if (arg == "--print" || arg == "-p") staticPrint = true;
            else if (arg == "--file" || arg == "-f") file = requireValue(args, index, arg);
            else if (arg == "--border" || arg == "-b") border = requireValue(args, index, arg);
            else if (arg == "--show-help") showHelp = true;
            else if (arg == "--no-show-help") showHelp = false;
            else if (arg == "--lazy-quotes") lazyQuotes = true;
            else if (arg == "--fields-per-record") fieldsPerRecord = std::stoi(requireValue(args, index, arg));
            else if (arg == "--return-column" || arg == "-r") returnColumn = std::stoi(requireValue(args, index, arg));
            else throw std::invalid_argument("unknown option: " + arg);
        }
        (void)bordersFor(border);
        if (height < 0 || returnColumn < 0 || fieldsPerRecord < 0) throw std::invalid_argument("numeric options cannot be negative");

        std::string input;
        if (!file.empty()) {
            std::ifstream stream(file);
            if (!stream) throw std::runtime_error("cannot open file: " + file);
            input.assign(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
        } else {
            input.assign(std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>());
        }
        std::vector<std::vector<std::string>> rows = parseDelimited(input, separator, lazyQuotes);
        if (rows.empty()) throw std::invalid_argument("no table rows supplied");
        if (headers.empty()) { headers = rows.front(); rows.erase(rows.begin()); }
        if (rows.empty()) throw std::invalid_argument("no data rows supplied");
        const std::size_t columnCount = std::max(headers.size(), std::accumulate(rows.begin(), rows.end(), std::size_t(0),
            [](std::size_t maximum, const std::vector<std::string>& row) {
//---------------------------------------------------------------------------------------------------------------------------------//

                return std::max(maximum, row.size());
            }));
        if (fieldsPerRecord > 0) {
            for (const auto& row : rows) {
                if (static_cast<int>(row.size()) != fieldsPerRecord) throw std::invalid_argument("row has unexpected field count");
            }
        }
        while (headers.size() < columnCount) headers.push_back("");
        if (widths.empty()) {
            widths.assign(columnCount, 1);
            for (std::size_t column = 0; column < columnCount; ++column) {
                widths[column] = displayWidth(headers[column]);
                for (const auto& row : rows) {
                    if (column < row.size()) widths[column] = std::max(widths[column], displayWidth(row[column]));
                }
                widths[column] = std::min(widths[column], 40);
            }
        }
        while (widths.size() < columnCount) widths.push_back(12);

        TerminalSession terminal;
        if (staticPrint || !terminal.interactive()) {
            for (const std::string& line : renderTable(headers, rows, widths, border, 0, 0, rows.size(), false)) {
                std::cout << line << '\n';
            }
            return 0;
        }
        if (!terminal.enableRawMode()) throw std::runtime_error("could not enter terminal raw mode");
        const int visibleHeight = height > 0 ? height : std::max(3, terminalDimensions().rows - 8);
        LiveRegion region(terminal);
        std::size_t selected = 0;
        while (true) {
            const std::size_t start = selected >= static_cast<std::size_t>(visibleHeight)
                ? selected - static_cast<std::size_t>(visibleHeight) + 1 : 0;
            std::vector<std::string> screen = renderTable(headers, rows, widths, border, selected, start,
                                                           static_cast<std::size_t>(visibleHeight), true);
            if (showHelp) screen.push_back("↑/↓ navigate • enter select • esc cancel");
            region.render(screen);
            const InputEvent event = terminal.readEvent();
            if (event.key == Key::Up || event.text == "k") selected = selected == 0 ? rows.size() - 1 : selected - 1;
            else if (event.key == Key::Down || event.text == "j") selected = (selected + 1) % rows.size();
            else if (event.key == Key::Enter) {
                region.clear();
                if (returnColumn > 0) {
                    const std::size_t column = static_cast<std::size_t>(returnColumn - 1);
                    if (column >= rows[selected].size()) return 1;
                    std::cout << rows[selected][column] << '\n';
                } else {
                    for (std::size_t column = 0; column < rows[selected].size(); ++column) {
                        if (column > 0) std::cout << separator;
                        std::cout << rows[selected][column];
                    }
                    std::cout << '\n';
                }
                return 0;
            } else if (event.key == Key::Escape || event.key == Key::Interrupt || event.key == Key::EndOfInput) {
                return 1;
            }
        }
    } catch (const std::exception& error) {
        std::cerr << "hum table: " << error.what() << '\n';
        return 2;
    }
}

void printTableHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum table [options] < data.csv\n\n"
        "Render delimited data and optionally select a row.\n\n"
        "Options:\n"
        "  -s, --separator CHAR       Field separator\n"
        "  -c, --columns A,B          Explicit column names\n"
        "  -w, --widths N,N           Column widths\n"
        "  -f, --file PATH            Read from a file\n"
        "  -p, --print                Print without interaction\n"
        "  -b, --border STYLE         rounded, normal, thick, double, hidden, none\n"
        "  -r, --return-column N      Return one 1-based column\n"
        "  --height N                 Visible row count\n"
        "  --lazy-quotes              Allow relaxed quoting\n"
        "  --fields-per-record N      Validate row field count\n";
}

}  // namespace hum

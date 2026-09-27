#include "hum/filter.hpp"

#include "hum/terminal.hpp"
#include "hum/text.hpp"
#include "hum/viewport.hpp"

#include <algorithm>
#include <cctype>
#include <iostream>
#include <iterator>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <unistd.h>

namespace hum {
namespace {

struct Match {
    std::size_t index = 0;
    int score = 0;
};

std::string lower(std::string value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
//---------------------------------------------------------------------------------------------------------------------------------//

        return static_cast<char>(std::tolower(character));
    });
    return value;
}

int fuzzyScore(const std::string& candidate, const std::string& query) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (query.empty()) return 0;
    const std::string text = lower(candidate);
    const std::string needle = lower(query);
    std::size_t position = 0;
    int score = 0;
    int previous = -2;
    for (char character : needle) {
        const std::size_t found = text.find(character, position);
        if (found == std::string::npos) return std::numeric_limits<int>::max();
        score += static_cast<int>(found);
        if (static_cast<int>(found) == previous + 1) score -= 3;
        previous = static_cast<int>(found);
        position = found + 1;
    }
    return score + static_cast<int>(text.size() - needle.size());
}

bool wordPrefixMatch(const std::string& candidate, const std::string& query) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (query.empty()) return true;
    const std::string text = lower(candidate);
    const std::string needle = lower(query);
    if (text.rfind(needle, 0) == 0) return true;
    for (std::size_t index = 1; index < text.size(); ++index) {
        if (!std::isalnum(static_cast<unsigned char>(text[index - 1])) && text.compare(index, needle.size(), needle) == 0) {
            return true;
        }
    }
    return false;
}

std::vector<Match> matchesFor(const std::vector<std::string>& options, const std::string& query,
                              bool fuzzy, bool sortResults) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::vector<Match> matches;
    for (std::size_t index = 0; index < options.size(); ++index) {
        const int score = fuzzy ? fuzzyScore(options[index], query)
                                : (wordPrefixMatch(options[index], query) ? 0 : std::numeric_limits<int>::max());
        if (score != std::numeric_limits<int>::max()) matches.push_back({index, score});
    }
    if (fuzzy && sortResults) {
        std::stable_sort(matches.begin(), matches.end(), [](const Match& left, const Match& right) {
//---------------------------------------------------------------------------------------------------------------------------------//

            return left.score < right.score;
        });
    }
    return matches;
}

std::string requireValue(const std::vector<std::string>& args, std::size_t& index,
                         const std::string& option) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (++index >= args.size()) throw std::invalid_argument(option + " requires a value");
    return args[index];
}

std::vector<std::string> splitInput(const std::string& input, const std::string& delimiter) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (delimiter.empty()) throw std::invalid_argument("input delimiter cannot be empty");
    std::vector<std::string> result;
    std::size_t start = 0;
    while (start < input.size()) {
        const std::size_t end = input.find(delimiter, start);
        if (end == std::string::npos) { result.push_back(input.substr(start)); break; }
        result.push_back(input.substr(start, end - start));
        start = end + delimiter.size();
    }
    return result;
}

std::string decodeDelimiter(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (value == "\\n") return "\n";
    if (value == "\\t") return "\t";
    if (value == "\\0") return std::string(1, '\0');
    return value;
}

void eraseLastCharacter(std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (value.empty()) return;
    std::size_t position = value.size() - 1;
    while (position > 0 && (static_cast<unsigned char>(value[position]) & 0xc0) == 0x80) --position;
    value.erase(position);
}

}  // namespace

int runFilter(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    int limit = 1;
    bool noLimit = false;
    bool selectIfOne = false;
    bool strict = true;
    bool showHelp = true;
    bool fuzzy = true;
    bool fuzzySort = true;
    int height = 10;
    std::string header;
    std::string placeholder = "Filter...";
    std::string prompt = "> ";
    std::string query;
    std::string inputDelimiter = "\n";
    std::string outputDelimiter = "\n";
    std::vector<std::string> options;

    try {
        for (std::size_t index = 0; index < args.size(); ++index) {
            const std::string& arg = args[index];
            if (arg == "--help" || arg == "-h") { printFilterHelp(); return 0; }
            if (arg == "--limit") limit = std::stoi(requireValue(args, index, arg));
            else if (arg == "--no-limit") noLimit = true;
            else if (arg == "--select-if-one") selectIfOne = true;
            else if (arg == "--strict") strict = true;
            else if (arg == "--no-strict") strict = false;
            else if (arg == "--show-help") showHelp = true;
            else if (arg == "--no-show-help") showHelp = false;
            else if (arg == "--fuzzy") fuzzy = true;
            else if (arg == "--no-fuzzy") fuzzy = false;
            else if (arg == "--fuzzy-sort") fuzzySort = true;
            else if (arg == "--no-fuzzy-sort") fuzzySort = false;
            else if (arg == "--height") height = std::stoi(requireValue(args, index, arg));
            else if (arg == "--header") header = requireValue(args, index, arg);
            else if (arg == "--placeholder") placeholder = requireValue(args, index, arg);
            else if (arg == "--prompt") prompt = requireValue(args, index, arg);
            else if (arg == "--value") query = requireValue(args, index, arg);
            else if (arg == "--input-delimiter") inputDelimiter = decodeDelimiter(requireValue(args, index, arg));
            else if (arg == "--output-delimiter") outputDelimiter = decodeDelimiter(requireValue(args, index, arg));
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option: " + arg);
            else options.push_back(arg);
        }
        if (limit < 1 && !noLimit) throw std::invalid_argument("limit must be at least one");
        if (height < 1) throw std::invalid_argument("height must be at least one");
        if (options.empty() && ::isatty(STDIN_FILENO) == 0) {
            std::string input{std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};
            options = splitInput(stripAnsi(input), inputDelimiter);
        }
        if (options.empty()) throw std::invalid_argument("no options supplied");
        const std::vector<Match> initialMatches = matchesFor(options, query, fuzzy, fuzzySort);
        if (selectIfOne && initialMatches.size() == 1) {
            std::cout << options[initialMatches.front().index] << '\n';
            return 0;
        }
    } catch (const std::exception& error) {
        std::cerr << "hum filter: " << error.what() << '\n';
        return 2;
    }

    TerminalSession terminal;
    if (!terminal.interactive() || !terminal.enableRawMode()) {
        const std::vector<Match> matches = matchesFor(options, query, fuzzy, fuzzySort);
        if (matches.size() == 1) {
            std::cout << options[matches.front().index] << '\n';
            return 0;
        }
        return strict ? 1 : (std::cout << query << '\n', 0);
    }

    LiveRegion region(terminal);
    std::size_t cursor = 0;
    std::vector<bool> selected(options.size(), false);
    while (true) {
        std::vector<Match> matches = matchesFor(options, query, fuzzy, fuzzySort);
        if (cursor >= matches.size()) cursor = matches.empty() ? 0 : matches.size() - 1;
        std::vector<std::string> screen;
        if (!header.empty()) screen.push_back(header);
        const std::string inputLine = query.empty() ? placeholder : query;
        screen.push_back(prompt + inputLine);
        const std::size_t start = cursor >= static_cast<std::size_t>(height)
            ? cursor - static_cast<std::size_t>(height) + 1 : 0;
        for (std::size_t row = start; row < matches.size() && row < start + static_cast<std::size_t>(height); ++row) {
            const std::size_t optionIndex = matches[row].index;
            std::string prefix = row == cursor ? "• " : "  ";
            if (limit != 1 || noLimit) prefix += selected[optionIndex] ? "◉ " : "○ ";
            std::string line = prefix + options[optionIndex];
            if (row == cursor && colorEnabled()) line = "\x1b[38;5;212m" + line + "\x1b[0m";
            screen.push_back(line);
        }
        if (matches.empty()) screen.push_back("  No matches");
        if (showHelp) screen.push_back(limit == 1 && !noLimit
            ? "type filter • ↑/↓ navigate • enter select"
            : "type filter • ↑/↓ navigate • tab select • enter submit");
        region.render(screen);

        const InputEvent event = terminal.readEvent();
        if (!event.text.empty() && event.text != " ") {
            query += event.text;
            cursor = 0;
        } else if (event.key == Key::Backspace) {
            eraseLastCharacter(query);
            cursor = 0;
        } else if (event.key == Key::Up) {
            if (!matches.empty()) cursor = cursor == 0 ? matches.size() - 1 : cursor - 1;
        } else if (event.key == Key::Down) {
            if (!matches.empty()) cursor = (cursor + 1) % matches.size();
        } else if ((event.key == Key::Tab || event.text == " ") && (limit != 1 || noLimit) && !matches.empty()) {
            const std::size_t optionIndex = matches[cursor].index;
            if (selected[optionIndex]) selected[optionIndex] = false;
            else if (noLimit || static_cast<int>(std::count(selected.begin(), selected.end(), true)) < limit) {
                selected[optionIndex] = true;
            }
        } else if (event.key == Key::Enter) {
            if (matches.empty()) {
                if (!strict) { region.clear(); std::cout << query << '\n'; return 0; }
                continue;
            }
            if (limit == 1 && !noLimit) selected[matches[cursor].index] = true;
            region.clear();
            bool first = true;
            for (std::size_t index = 0; index < selected.size(); ++index) {
                if (!selected[index]) continue;
                if (!first) std::cout << outputDelimiter;
                first = false;
                std::cout << options[index];
            }
            if (outputDelimiter.empty() || outputDelimiter.back() != '\0') std::cout << '\n';
            return 0;
        } else if (event.key == Key::Escape || event.key == Key::Interrupt || event.key == Key::EndOfInput) {
            return 1;
        }
    }
}

void printFilterHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum filter [options] [items ...]\n\n"
        "Fuzzy-filter and select items from arguments or stdin.\n\n"
        "Options:\n"
        "  --limit N, --no-limit       Selection limit\n"
        "  --select-if-one             Return a sole match immediately\n"
        "  --[no-]strict               Require a matched item\n"
        "  --[no-]fuzzy                Toggle subsequence matching\n"
        "  --[no-]fuzzy-sort           Toggle score ordering\n"
        "  --value TEXT                Initial query\n"
        "  --header TEXT               Header above the query\n"
        "  --placeholder TEXT          Empty-query placeholder\n"
        "  --prompt TEXT               Query prompt\n"
        "  --height N                  Visible result count\n"
        "  --input-delimiter TEXT      Stdin delimiter\n"
        "  --output-delimiter TEXT     Selection delimiter\n";
}

}  // namespace hum

#include "hum/format.hpp"

#include "hum/terminal.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#ifndef HUM_VERSION
#define HUM_VERSION "unknown"
#endif

namespace hum {
namespace {

std::string readInput() {
//---------------------------------------------------------------------------------------------------------------------------------//

    return {std::istreambuf_iterator<char>(std::cin), std::istreambuf_iterator<char>()};
}

std::string replaceAll(std::string value, const std::string& from, const std::string& to) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (from.empty()) return value;
    std::size_t at = 0;
    while ((at = value.find(from, at)) != std::string::npos) {
        value.replace(at, from.size(), to);
        at += to.size();
    }
    return value;
}

std::string colour(const std::string& value, const char* code) {
//---------------------------------------------------------------------------------------------------------------------------------//

    return colorEnabled() ? std::string("\x1b[") + code + "m" + value + "\x1b[0m" : value;
}

std::string markdownLine(std::string line) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (line.rfind("### ", 0) == 0) return colour(line.substr(4), "36;1");
    if (line.rfind("## ", 0) == 0) return colour(line.substr(3), "35;1");
    if (line.rfind("# ", 0) == 0) return colour(line.substr(2), "33;1");
    if (line.rfind("- ", 0) == 0 || line.rfind("* ", 0) == 0) line.replace(0, 1, "•");
    for (std::size_t at = 0; (at = line.find("**", at)) != std::string::npos;) {
        const std::size_t end = line.find("**", at + 2);
        if (end == std::string::npos) break;
        line.replace(end, 2, colorEnabled() ? "\x1b[0m" : "");
        line.replace(at, 2, colorEnabled() ? "\x1b[1m" : "");
        at = end + 7;
    }
    for (std::size_t at = 0; (at = line.find('`', at)) != std::string::npos;) {
        const std::size_t end = line.find('`', at + 1);
        if (end == std::string::npos) break;
        line.replace(end, 1, colorEnabled() ? "\x1b[0m" : "");
        line.replace(at, 1, colorEnabled() ? "\x1b[36m" : "");
        at = end + 5;
    }
    return line;
}

std::string highlightCode(const std::string& input) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (!colorEnabled()) return input;
    const std::vector<std::string> keywords = {"auto", "bool", "class", "const", "else", "for", "if", "int",
        "namespace", "return", "string", "struct", "void", "while", "def", "import", "print", "function",
        "let", "var", "export", "true", "false", "null", "nullptr"};
    std::string result;
    for (std::size_t i = 0; i < input.size();) {
        const unsigned char c = static_cast<unsigned char>(input[i]);
        if (std::isalpha(c) || c == '_') {
            const std::size_t start = i++;
            while (i < input.size() && (std::isalnum(static_cast<unsigned char>(input[i])) || input[i] == '_')) ++i;
            const std::string token = input.substr(start, i - start);
            result += std::find(keywords.begin(), keywords.end(), token) != keywords.end() ? colour(token, "35;1") : token;
        } else if (std::isdigit(c)) {
            const std::size_t start = i++;
            while (i < input.size() && (std::isalnum(static_cast<unsigned char>(input[i])) || input[i] == '.')) ++i;
            result += colour(input.substr(start, i - start), "33");
        } else if (c == '"' || c == '\'') {
            const char quote = static_cast<char>(c);
            const std::size_t start = i++;
            bool escaped = false;
            while (i < input.size()) {
                const char current = input[i++];
                if (current == quote && !escaped) break;
                if (current == '\\' && !escaped) escaped = true;
                else escaped = false;
            }
            result += colour(input.substr(start, i - start), "32");
        } else {
            result.push_back(input[i++]);
        }
    }
    return result;
}

int compareVersions(const std::string& left, const std::string& right) {
//---------------------------------------------------------------------------------------------------------------------------------//

    auto parts = [](const std::string& version) {
        std::vector<int> result;
        std::istringstream stream(version);
        std::string part;
        while (std::getline(stream, part, '.')) {
            if (part.empty() || !std::all_of(part.begin(), part.end(), [](unsigned char c) { return std::isdigit(c); })) {
                throw std::invalid_argument("version components must be numeric");
            }
            result.push_back(std::stoi(part));
        }
        return result;
    };
    const auto a = parts(left), b = parts(right);
    for (std::size_t index = 0; index < std::max(a.size(), b.size()); ++index) {
        const int av = index < a.size() ? a[index] : 0;
        const int bv = index < b.size() ? b[index] : 0;
        if (av != bv) return av < bv ? -1 : 1;
    }
    return 0;
}

}  // namespace

int runFormat(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string type = "markdown", language;
    std::vector<std::pair<std::string, std::string>> values;
    std::string input;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--help" || args[i] == "-h") {
                std::cout << "Usage: hum format [--type markdown|code|template|emoji] [options]\n"
                             "Reads text from stdin; --set NAME=VALUE supplies template values.\n";
                return 0;
            }
            if (args[i] == "--type" && i + 1 < args.size()) type = args[++i];
            else if (args[i] == "--language" && i + 1 < args.size()) language = args[++i];
            else if (args[i] == "--set" && i + 1 < args.size()) {
                const std::string item = args[++i];
                const auto equals = item.find('=');
                if (equals == std::string::npos) throw std::invalid_argument("--set expects NAME=VALUE");
                values.emplace_back(item.substr(0, equals), item.substr(equals + 1));
            } else if (args[i].rfind("--", 0) == 0) throw std::invalid_argument("unknown option: " + args[i]);
            else input += (input.empty() ? "" : " ") + args[i];
        }
        if (input.empty()) input = readInput();
        if (type == "markdown") {
            std::istringstream lines(input); std::string line; bool first = true;
            while (std::getline(lines, line)) { if (!first) std::cout << '\n'; first = false; std::cout << markdownLine(line); }
            if (!input.empty() && input.back() == '\n') std::cout << '\n';
        } else if (type == "code") {
            (void)language;
            std::cout << highlightCode(input);
        } else if (type == "template") {
            for (const auto& entry : values) input = replaceAll(input, "{{" + entry.first + "}}", entry.second);
            std::cout << input;
        } else if (type == "emoji") {
            const std::map<std::string, std::string> emoji{{":smile:", "😄"}, {":heart:", "❤️"},
                {":sparkles:", "✨"}, {":rocket:", "🚀"}, {":tada:", "🎉"}, {":warning:", "⚠️"},
                {":check:", "✓"}, {":cross:", "✗"}, {":thumbsup:", "👍"}};
            for (const auto& item : emoji) input = replaceAll(input, item.first, item.second);
            std::cout << input;
        } else throw std::invalid_argument("type must be markdown, code, template, or emoji");
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "hum format: " << error.what() << '\n';
        return 2;
    }
}

int runVersionCheck(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (args.size() != 1 || args[0] == "--help" || args[0] == "-h") {
        std::cout << "Usage: hum version-check <constraint>\n"
                     "Constraints: =1.2, >1.2, >=1.2, <1.2, <=1.2\n";
        return args.size() == 1 ? 0 : 2;
    }
    const std::string constraint = args[0];
    std::string operation;
    std::size_t offset = 0;
    for (const std::string op : {">=", "<=", ">", "<", "="}) {
        if (constraint.rfind(op, 0) == 0) { operation = op; offset = op.size(); break; }
    }
    if (operation.empty() || offset == constraint.size()) {
        std::cerr << "hum version-check: expected a version constraint such as '>=0.4'\n";
        return 2;
    }
    try {
        const int comparison = compareVersions(HUM_VERSION, constraint.substr(offset));
        const bool passes = operation == ">" ? comparison > 0 : operation == ">=" ? comparison >= 0 :
            operation == "<" ? comparison < 0 : operation == "<=" ? comparison <= 0 : comparison == 0;
        if (passes) return 0;
        std::cerr << "hum version-check: hum " << HUM_VERSION << " does not satisfy " << constraint << '\n';
        return 1;
    } catch (...) {
        std::cerr << "hum version-check: invalid numeric version constraint\n";
        return 2;
    }
}

}  // namespace hum

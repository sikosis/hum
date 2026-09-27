#include "hum/text.hpp"

#include <algorithm>
#include <cctype>
#include <clocale>
#include <cwchar>
#include <stdexcept>

namespace hum {

std::string stripAnsi(const std::string& input) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string result;
    for (std::size_t i = 0; i < input.size();) {
        if (input[i] == '\x1b' && i + 1 < input.size() && input[i + 1] == '[') {
            i += 2;
            while (i < input.size()) {
                const unsigned char c = static_cast<unsigned char>(input[i++]);
                if (c >= 0x40 && c <= 0x7e) {
                    break;
                }
            }
            continue;
        }
        result.push_back(input[i++]);
    }
    return result;
}

int displayWidth(const std::string& input) {
//---------------------------------------------------------------------------------------------------------------------------------//

    static const bool localeInitialized = [] {
//---------------------------------------------------------------------------------------------------------------------------------//

        std::setlocale(LC_CTYPE, "");
        return true;
    }();
    (void)localeInitialized;

    const std::string plain = stripAnsi(input);
    std::mbstate_t state{};
    const char* cursor = plain.data();
    std::size_t remaining = plain.size();
    int width = 0;

    while (remaining > 0) {
        wchar_t character = 0;
        const std::size_t used = std::mbrtowc(&character, cursor, remaining, &state);
        if (used == static_cast<std::size_t>(-1) || used == static_cast<std::size_t>(-2)) {
            ++width;
            ++cursor;
            --remaining;
            state = std::mbstate_t{};
            continue;
        }
        if (used == 0) {
            break;
        }
        const int characterWidth = ::wcwidth(character);
        width += characterWidth < 0 ? 1 : characterWidth;
        cursor += used;
        remaining -= used;
    }
    return width;
}

std::vector<std::string> splitLines(const std::string& input) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::vector<std::string> lines;
    std::size_t start = 0;
    while (start <= input.size()) {
        const std::size_t end = input.find('\n', start);
        if (end == std::string::npos) {
            lines.push_back(input.substr(start));
            break;
        }
        lines.push_back(input.substr(start, end - start));
        start = end + 1;
    }
    return lines.empty() ? std::vector<std::string>{""} : lines;
}

std::string trim(const std::string& input) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const auto first = std::find_if_not(input.begin(), input.end(), [](unsigned char c) {
//---------------------------------------------------------------------------------------------------------------------------------//

        return std::isspace(c) != 0;
    });
    const auto last = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char c) {
//---------------------------------------------------------------------------------------------------------------------------------//

        return std::isspace(c) != 0;
    }).base();
    return first < last ? std::string(first, last) : std::string();
}

std::array<int, 4> parseBox(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::vector<int> parts;
    std::size_t position = 0;
    while (position < value.size()) {
        while (position < value.size() && std::isspace(static_cast<unsigned char>(value[position]))) {
            ++position;
        }
        if (position >= value.size()) {
            break;
        }
        std::size_t consumed = 0;
        const int part = std::stoi(value.substr(position), &consumed);
        if (part < 0) {
            throw std::invalid_argument("spacing values cannot be negative");
        }
        parts.push_back(part);
        position += consumed;
        if (parts.size() > 4) {
            throw std::invalid_argument("spacing accepts at most four values");
        }
    }

    if (parts.empty()) {
        return {0, 0, 0, 0};
    }
    if (parts.size() == 1) {
        return {parts[0], parts[0], parts[0], parts[0]};
    }
    if (parts.size() == 2) {
        return {parts[0], parts[1], parts[0], parts[1]};
    }
    if (parts.size() == 3) {
        return {parts[0], parts[1], parts[2], parts[1]};
    }
    return {parts[0], parts[1], parts[2], parts[3]};
}

}  // namespace hum

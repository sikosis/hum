#include "hum/log.hpp"

#include "hum/terminal.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace hum {
namespace {

int levelRank(const std::string& level) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (level == "debug") return 0;
    if (level == "info" || level == "none") return 1;
    if (level == "warn") return 2;
    if (level == "error") return 3;
    if (level == "fatal") return 4;
    throw std::invalid_argument("unknown log level: " + level);
}

std::string upper(std::string value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char character) {
//---------------------------------------------------------------------------------------------------------------------------------//

        return static_cast<char>(std::toupper(character));
    });
    return value;
}

std::string jsonEscape(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::ostringstream output;
    for (unsigned char character : value) {
        switch (character) {
            case '"': output << "\\\""; break;
            case '\\': output << "\\\\"; break;
            case '\n': output << "\\n"; break;
            case '\r': output << "\\r"; break;
            case '\t': output << "\\t"; break;
            default:
                if (character < 0x20) {
                    output << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                           << static_cast<int>(character) << std::dec;
                } else {
                    output << static_cast<char>(character);
                }
        }
    }
    return output.str();
}

std::string logfmtQuote(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (value.find_first_of(" \t\n\r=\"") == std::string::npos) return value;
    return "\"" + jsonEscape(value) + "\"";
}

std::string currentTime(const std::string& format) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (format.empty()) return "";
    const std::time_t now = std::time(nullptr);
    struct tm localTime {};
    localtime_r(&now, &localTime);
    std::string layout;
    if (format == "kitchen") layout = "%I:%M%p";
    else if (format == "ansic") layout = "%a %b %e %H:%M:%S %Y";
    else if (format == "rfc822") layout = "%d %b %y %H:%M %Z";
    else if (format == "rfc3339") layout = "%Y-%m-%dT%H:%M:%S%z";
    else layout = format;
    char buffer[128] = {};
    if (std::strftime(buffer, sizeof(buffer), layout.c_str(), &localTime) == 0) {
        throw std::invalid_argument("time format produced no output");
    }
    return buffer;
}

std::string printfStyle(const std::vector<std::string>& text) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (text.empty()) return "";
    std::string result;
    std::size_t argument = 1;
    for (std::size_t i = 0; i < text[0].size(); ++i) {
        if (text[0][i] == '%' && i + 1 < text[0].size()) {
            if (text[0][i + 1] == '%') {
                result.push_back('%');
                ++i;
                continue;
            }
            if (text[0][i + 1] == 's' && argument < text.size()) {
                result += text[argument++];
                ++i;
                continue;
            }
        }
        result.push_back(text[0][i]);
    }
    return result;
}

std::string colourForLevel(const std::string& level) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (!colorEnabled()) return "";
    if (level == "debug") return "\x1b[38;5;63m";
    if (level == "info") return "\x1b[38;5;86m";
    if (level == "warn") return "\x1b[38;5;214m";
    if (level == "error" || level == "fatal") return "\x1b[38;5;196m";
    return "";
}

std::string requireValue(const std::vector<std::string>& args, std::size_t& index,
                         const std::string& option) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (++index >= args.size()) throw std::invalid_argument(option + " requires a value");
    return args[index];
}

}  // namespace

int runLog(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string file;
    std::string formatter = "text";
    std::string level = "none";
    std::string prefix;
    std::string timeFormat;
    std::string minimumLevel;
    bool structured = false;
    bool format = false;
    std::vector<std::string> text;

    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--help" || arg == "-h") { printLogHelp(); return 0; }
            if (arg == "--file" || arg == "-o") file = requireValue(args, i, arg);
            else if (arg == "--formatter") formatter = requireValue(args, i, arg);
            else if (arg == "--level" || arg == "-l") level = requireValue(args, i, arg);
            else if (arg == "--prefix") prefix = requireValue(args, i, arg);
            else if (arg == "--time" || arg == "-t") timeFormat = requireValue(args, i, arg);
            else if (arg == "--min-level") minimumLevel = requireValue(args, i, arg);
            else if (arg == "--structured" || arg == "-s") structured = true;
            else if (arg == "--format" || arg == "-f") format = true;
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option: " + arg);
            else text.push_back(arg);
        }
        if (structured && format) throw std::invalid_argument("--structured and --format cannot be combined");
        if (formatter != "text" && formatter != "json" && formatter != "logfmt") {
            throw std::invalid_argument("formatter must be text, json, or logfmt");
        }
        if (minimumLevel.empty()) {
            const char* environmentLevel = std::getenv("HUM_LOG_LEVEL");
            if (environmentLevel != nullptr) minimumLevel = environmentLevel;
        }
        if (!minimumLevel.empty() && levelRank(level) < levelRank(minimumLevel)) return 0;

        const std::string message = format ? printfStyle(text) : (text.empty() ? "" : text.front());
        std::vector<std::pair<std::string, std::string>> fields;
        if (structured) {
            for (std::size_t i = 1; i < text.size(); i += 2) {
                fields.emplace_back(text[i], i + 1 < text.size() ? text[i + 1] : "");
            }
        } else if (!format && text.size() > 1) {
            std::string joined = message;
            for (std::size_t i = 1; i < text.size(); ++i) joined += " " + text[i];
            fields.emplace_back("_message", joined);
        }
        const std::string finalMessage = !fields.empty() && fields.front().first == "_message"
            ? fields.front().second : message;
        if (!fields.empty() && fields.front().first == "_message") fields.erase(fields.begin());
        const std::string timestamp = currentTime(timeFormat);

        std::ostringstream line;
        if (formatter == "json") {
            line << '{';
            bool comma = false;
            const auto addJson = [&](const std::string& key, const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

                if (value.empty()) return;
                if (comma) line << ',';
                comma = true;
                line << '"' << jsonEscape(key) << "\":\"" << jsonEscape(value) << '"';
            };
            addJson("time", timestamp);
            addJson("level", level == "none" ? "" : level);
            addJson("prefix", prefix);
            addJson("message", finalMessage);
            for (const auto& field : fields) addJson(field.first, field.second);
            line << '}';
        } else if (formatter == "logfmt") {
            if (!timestamp.empty()) line << "time=" << logfmtQuote(timestamp) << ' ';
            if (level != "none") line << "level=" << level << ' ';
            if (!prefix.empty()) line << "prefix=" << logfmtQuote(prefix) << ' ';
            line << "message=" << logfmtQuote(finalMessage);
            for (const auto& field : fields) line << ' ' << field.first << '=' << logfmtQuote(field.second);
        } else {
            if (!timestamp.empty()) line << timestamp << ' ';
            if (!prefix.empty()) line << prefix << ' ';
            if (level != "none") {
                const std::string colour = colourForLevel(level);
                line << colour << upper(level) << (colour.empty() ? "" : "\x1b[0m") << ' ';
            }
            line << finalMessage;
            for (const auto& field : fields) line << ' ' << field.first << '=' << field.second;
        }

        if (!file.empty()) {
            std::ofstream output(file, std::ios::app);
            if (!output) throw std::runtime_error("cannot open log file: " + file);
            output << line.str() << '\n';
        } else {
            std::cerr << line.str() << '\n';
        }
        return level == "fatal" ? 1 : 0;
    } catch (const std::exception& error) {
        std::cerr << "hum log: " << error.what() << '\n';
        return 2;
    }
}

void printLogHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum log [options] text [key value ...]\n\n"
        "Print styled or structured log messages. Logs go to stderr by default.\n\n"
        "Options:\n"
        "  -l, --level LEVEL       none, debug, info, warn, error, fatal\n"
        "  -s, --structured        Treat trailing arguments as key/value pairs\n"
        "  -f, --format            Replace %s in the first argument\n"
        "  --formatter FORMAT      text, logfmt, or json\n"
        "  --prefix TEXT           Prefix the message\n"
        "  -t, --time FORMAT       kitchen, ansic, rfc822, rfc3339, or strftime\n"
        "  --min-level LEVEL       Suppress messages below this level\n"
        "  -o, --file PATH         Append the log line to a file\n";
}

}  // namespace hum

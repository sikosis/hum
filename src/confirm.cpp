#include "hum/confirm.hpp"

#include "hum/terminal.hpp"
#include "hum/text.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <iostream>
#include <stdexcept>

namespace hum {
namespace {

std::string requireValue(const std::vector<std::string>& args, std::size_t& index,
                         const std::string& option) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (++index >= args.size()) throw std::invalid_argument(option + " requires a value");
    return args[index];
}

bool parseDefault(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string lower = value;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) {
//---------------------------------------------------------------------------------------------------------------------------------//

        return static_cast<char>(std::tolower(c));
    });
    if (lower == "yes" || lower == "true" || lower == "y" || lower == "1") return true;
    if (lower == "no" || lower == "false" || lower == "n" || lower == "0") return false;
    throw std::invalid_argument("--default must be yes or no");
}

int parseDuration(const std::string& value) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::size_t consumed = 0;
    const double amount = std::stod(value, &consumed);
    const std::string unit = value.substr(consumed);
    double multiplier = 1000.0;
    if (unit == "ms") multiplier = 1.0;
    else if (unit.empty() || unit == "s") multiplier = 1000.0;
    else throw std::invalid_argument("timeout unit must be ms or s");
    if (amount < 0) throw std::invalid_argument("timeout cannot be negative");
    return static_cast<int>(amount * multiplier);
}

std::string renderPrompt(const ConfirmOptions& options, bool selectedAffirmative) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const bool color = colorEnabled();
    const std::string selected = color ? "\x1b[48;5;212m\x1b[38;5;230m" : "[";
    const std::string unselected = color ? "\x1b[48;5;235m\x1b[38;5;254m" : " ";
    const std::string end = color ? "\x1b[0m" : "]";
    const auto action = [&](const std::string& label, bool active) {
//---------------------------------------------------------------------------------------------------------------------------------//

        return std::string(active ? selected : unselected) + " " + label + " " + end;
    };
    std::string result = options.prompt + "  " + action(options.affirmative, selectedAffirmative) +
                         " " + action(options.negative, !selectedAffirmative);
    if (options.showHelp) result += "  ←/→ select • enter confirm";
    return result;
}

}  // namespace

int runConfirm(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    ConfirmOptions options;
    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (arg == "--help" || arg == "-h") { printConfirmHelp(); return 0; }
            if (arg == "--affirmative") options.affirmative = requireValue(args, i, arg);
            else if (arg == "--negative") options.negative = requireValue(args, i, arg);
            else if (arg == "--default") options.defaultAffirmative = parseDefault(requireValue(args, i, arg));
            else if (arg.rfind("--default=", 0) == 0) options.defaultAffirmative = parseDefault(arg.substr(10));
            else if (arg == "--show-output") options.showOutput = true;
            else if (arg == "--show-help") options.showHelp = true;
            else if (arg == "--no-show-help") options.showHelp = false;
            else if (arg == "--timeout") options.timeoutMilliseconds = parseDuration(requireValue(args, i, arg));
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option: " + arg);
            else if (options.prompt == "Are you sure?") options.prompt = arg;
            else options.prompt += " " + arg;
        }
    } catch (const std::exception& error) {
        std::cerr << "hum confirm: " << error.what() << '\n';
        return 2;
    }

    bool choice = options.defaultAffirmative;
    TerminalSession terminal;
    if (!terminal.interactive() || !terminal.enableRawMode()) {
        std::cerr << options.prompt << " [" << (choice ? "Y/n" : "y/N") << "] ";
        std::string answer;
        std::getline(std::cin, answer);
        answer = trim(answer);
        if (!answer.empty()) {
            const char initial = static_cast<char>(std::tolower(static_cast<unsigned char>(answer[0])));
            if (initial == 'y') choice = true;
            else if (initial == 'n') choice = false;
            else return 1;
        }
    } else {
        while (true) {
            terminal.write("\r\x1b[2K" + renderPrompt(options, choice));
            const Key key = terminal.readKey(options.timeoutMilliseconds);
            if (key == Key::Left || key == Key::Right || key == Key::Tab) choice = !choice;
            else if (key == Key::Yes) { choice = true; break; }
            else if (key == Key::No) { choice = false; break; }
            else if (key == Key::Enter || key == Key::Timeout) break;
            else if (key == Key::Escape || key == Key::Interrupt || key == Key::EndOfInput) {
                terminal.write("\r\x1b[2K");
                return 1;
            }
        }
        terminal.write("\r\x1b[2K" + options.prompt + " " +
                       (choice ? options.affirmative : options.negative) + "\n");
    }

    if (options.showOutput) {
        std::cout << options.prompt << " " << (choice ? options.affirmative : options.negative) << '\n';
    }
    return choice ? 0 : 1;
}

void printConfirmHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum confirm [options] [prompt]\n\n"
        "Ask for confirmation. Exit 0 for yes and 1 for no.\n\n"
        "Options:\n"
        "  --affirmative TEXT    Affirmative action label (default: Yes)\n"
        "  --negative TEXT       Negative action label (default: No)\n"
        "  --default yes|no      Action selected initially (default: yes)\n"
        "  --timeout DURATION    Select the default after e.g. 500ms or 5s\n"
        "  --show-output         Print the prompt and chosen action to stdout\n"
        "  --[no-]show-help      Show keyboard hints (default: on)\n";
}

}  // namespace hum

#include "hum/spin.hpp"

#include "hum/terminal.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <stdexcept>
#include <sys/wait.h>
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

std::vector<std::string> spinnerFrames(const std::string& name) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (name == "line") return {"-", "\\", "|", "/"};
    if (name == "minidot") return {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    if (name == "jump") return {"⢄", "⢂", "⢁", "⡁", "⡈", "⡐", "⡠"};
    if (name == "pulse") return {"█", "▓", "▒", "░"};
    if (name == "points") return {"∙∙∙", "●∙∙", "∙●∙", "∙∙●"};
    if (name == "globe") return {"🌍", "🌎", "🌏"};
    if (name == "moon") return {"🌑", "🌒", "🌓", "🌔", "🌕", "🌖", "🌗", "🌘"};
    if (name == "monkey") return {"🙈", "🙉", "🙊"};
    if (name == "meter") return {"▱▱▱", "▰▱▱", "▰▰▱", "▰▰▰"};
    if (name == "hamburger") return {"☱", "☲", "☴"};
    if (name == "dot") return {"⣾", "⣽", "⣻", "⢿", "⡿", "⣟", "⣯", "⣷"};
    throw std::invalid_argument("unknown spinner: " + name);
}

void drainPipe(int fd, std::string& captured, bool show, std::ostream& output) {
//---------------------------------------------------------------------------------------------------------------------------------//

    char buffer[4096];
    while (true) {
        const ssize_t count = ::read(fd, buffer, sizeof(buffer));
        if (count > 0) {
            captured.append(buffer, static_cast<std::size_t>(count));
            if (show) {
                output.write(buffer, count);
                output.flush();
            }
        } else if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK)) {
            break;
        } else {
            break;
        }
    }
}

}  // namespace

int runSpin(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    bool showOutput = false;
    bool showError = false;
    bool showStdout = false;
    bool showStderr = false;
    std::string spinner = "dot";
    std::string title = "Loading...";
    std::string align = "left";
    int timeoutMilliseconds = 0;
    std::vector<std::string> command;
    bool commandStarted = false;

    try {
        for (std::size_t i = 0; i < args.size(); ++i) {
            const std::string& arg = args[i];
            if (commandStarted) { command.push_back(arg); continue; }
            if (arg == "--") { commandStarted = true; continue; }
            if (arg == "--help" || arg == "-h") { printSpinHelp(); return 0; }
            if (arg == "--show-output") showOutput = true;
            else if (arg == "--show-error") showError = true;
            else if (arg == "--show-stdout") showStdout = true;
            else if (arg == "--show-stderr") showStderr = true;
            else if (arg == "--spinner" || arg == "-s") spinner = requireValue(args, i, arg);
            else if (arg == "--title") title = requireValue(args, i, arg);
            else if (arg == "--align" || arg == "-a") align = requireValue(args, i, arg);
            else if (arg == "--timeout") timeoutMilliseconds = parseDuration(requireValue(args, i, arg));
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option: " + arg);
            else { commandStarted = true; command.push_back(arg); }
        }
        if (command.empty()) throw std::invalid_argument("provide a command after --");
        if (align != "left" && align != "right") throw std::invalid_argument("alignment must be left or right");
        (void)spinnerFrames(spinner);
    } catch (const std::exception& error) {
        std::cerr << "hum spin: " << error.what() << '\n';
        return 2;
    }

    int standardOutputPipe[2] = {-1, -1};
    int standardErrorPipe[2] = {-1, -1};
    if (::pipe(standardOutputPipe) != 0 || ::pipe(standardErrorPipe) != 0) {
        std::cerr << "hum spin: could not create output pipes\n";
        return 2;
    }

    const pid_t child = ::fork();
    if (child < 0) {
        std::cerr << "hum spin: could not start command\n";
        return 2;
    }
    if (child == 0) {
        ::close(standardOutputPipe[0]);
        ::close(standardErrorPipe[0]);
        ::dup2(standardOutputPipe[1], STDOUT_FILENO);
        ::dup2(standardErrorPipe[1], STDERR_FILENO);
        ::close(standardOutputPipe[1]);
        ::close(standardErrorPipe[1]);
        std::vector<char*> arguments;
        for (std::string& part : command) arguments.push_back(const_cast<char*>(part.c_str()));
        arguments.push_back(nullptr);
        ::execvp(arguments[0], arguments.data());
        const std::string message = "hum spin: command not found: " + command.front() + "\n";
        ::write(STDERR_FILENO, message.data(), message.size());
        _exit(127);
    }

    ::close(standardOutputPipe[1]);
    ::close(standardErrorPipe[1]);
    ::fcntl(standardOutputPipe[0], F_SETFL, O_NONBLOCK);
    ::fcntl(standardErrorPipe[0], F_SETFL, O_NONBLOCK);

    TerminalSession terminal;
    const bool animate = terminal.interactive();
    const std::vector<std::string> frames = spinnerFrames(spinner);
    std::size_t frame = 0;
    std::string capturedOutput;
    std::string capturedError;
    int waitStatus = 0;
    bool timedOut = false;
    bool hasWaitStatus = false;
    const auto start = std::chrono::steady_clock::now();

    while (true) {
        drainPipe(standardOutputPipe[0], capturedOutput, showOutput || showStdout, std::cout);
        drainPipe(standardErrorPipe[0], capturedError, showOutput || showStderr, std::cerr);
        const pid_t result = ::waitpid(child, &waitStatus, WNOHANG);
        if (result == child) { hasWaitStatus = true; break; }
        if (result < 0) break;

        if (timeoutMilliseconds > 0) {
            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - start).count();
            if (elapsed >= timeoutMilliseconds) {
                ::kill(child, SIGTERM);
                ::waitpid(child, &waitStatus, 0);
                timedOut = true;
                hasWaitStatus = true;
                break;
            }
        }
        if (animate) {
            const std::string indicator = colorEnabled()
                ? "\x1b[38;5;212m" + frames[frame % frames.size()] + "\x1b[0m"
                : frames[frame % frames.size()];
            terminal.write("\r\x1b[2K" + (align == "left" ? indicator + " " + title : title + " " + indicator));
            ++frame;
        }
        ::usleep(80000);
    }

    drainPipe(standardOutputPipe[0], capturedOutput, showOutput || showStdout, std::cout);
    drainPipe(standardErrorPipe[0], capturedError, showOutput || showStderr, std::cerr);
    ::close(standardOutputPipe[0]);
    ::close(standardErrorPipe[0]);
    if (animate) terminal.write("\r\x1b[2K");

    int exitStatus = timedOut ? 124 : 1;
    if (!timedOut && hasWaitStatus && WIFEXITED(waitStatus)) exitStatus = WEXITSTATUS(waitStatus);
    else if (!timedOut && hasWaitStatus && WIFSIGNALED(waitStatus)) exitStatus = 128 + WTERMSIG(waitStatus);
    if (showError && exitStatus != 0) {
        if (!showOutput && !showStdout) std::cout << capturedOutput;
        if (!showOutput && !showStderr) std::cerr << capturedError;
    }
    return exitStatus;
}

void printSpinHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum spin [options] -- command [arguments ...]\n\n"
        "Display a spinner while running a command and return its exit status.\n\n"
        "Options:\n"
        "  -s, --spinner NAME    line, dot, minidot, jump, pulse, points, globe,\n"
        "                         moon, monkey, meter, or hamburger\n"
        "  --title TEXT          Spinner title\n"
        "  -a, --align SIDE      left or right\n"
        "  --show-output         Stream stdout and stderr\n"
        "  --show-stdout         Stream stdout\n"
        "  --show-stderr         Stream stderr\n"
        "  --show-error          Show captured output only on failure\n"
        "  --timeout DURATION    Stop the command after a duration\n";
}

}  // namespace hum

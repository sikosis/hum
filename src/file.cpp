#include "hum/file.hpp"

#include "hum/terminal.hpp"
#include "hum/viewport.hpp"

#include <algorithm>
#include <cstdlib>
#include <dirent.h>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <sstream>
#include <stdexcept>
#include <sys/stat.h>
#include <unistd.h>

namespace hum {
namespace {

struct FileEntry {
    std::string name;
    std::string path;
    bool directory = false;
    bool symlink = false;
    mode_t mode = 0;
    off_t size = 0;
};

std::string joinPath(const std::string& directory, const std::string& name) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (directory.empty() || directory == ".") return directory == "." ? "./" + name : name;
    return directory.back() == '/' ? directory + name : directory + "/" + name;
}

std::string parentPath(std::string path) {
//---------------------------------------------------------------------------------------------------------------------------------//

    while (path.size() > 1 && path.back() == '/') path.pop_back();
    const std::size_t separator = path.find_last_of('/');
    if (separator == std::string::npos) return ".";
    if (separator == 0) return "/";
    return path.substr(0, separator);
}

std::string permissions(mode_t mode) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string result = S_ISDIR(mode) ? "d" : (S_ISLNK(mode) ? "l" : "-");
    const mode_t bits[] = {S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP,
                           S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH};
    const char letters[] = {'r', 'w', 'x', 'r', 'w', 'x', 'r', 'w', 'x'};
    for (int index = 0; index < 9; ++index) result.push_back((mode & bits[index]) ? letters[index] : '-');
    return result;
}

std::string fileSize(off_t size) {
//---------------------------------------------------------------------------------------------------------------------------------//

    const char* units[] = {"B", "K", "M", "G", "T"};
    double value = static_cast<double>(size);
    int unit = 0;
    while (value >= 1024.0 && unit < 4) { value /= 1024.0; ++unit; }
    std::ostringstream output;
    if (unit == 0) output << static_cast<long long>(value);
    else output << std::fixed << std::setprecision(value < 10 ? 1 : 0) << value;
    return output.str() + units[unit];
}

std::vector<FileEntry> readDirectory(const std::string& path, bool showAll) {
//---------------------------------------------------------------------------------------------------------------------------------//

    DIR* directory = ::opendir(path.c_str());
    if (directory == nullptr) throw std::runtime_error("cannot open directory: " + path);
    std::vector<FileEntry> entries;
    if (path != "/") {
        const std::string parent = parentPath(path);
        struct stat parentInformation {};
        ::stat(parent.c_str(), &parentInformation);
        entries.push_back({"..", parent, true, false, parentInformation.st_mode, parentInformation.st_size});
    }
    while (struct dirent* item = ::readdir(directory)) {
        const std::string name = item->d_name;
        if (name == "." || name == "..") continue;
        if (!showAll && !name.empty() && name.front() == '.') continue;
        const std::string itemPath = joinPath(path, name);
        struct stat information {};
        if (::lstat(itemPath.c_str(), &information) != 0) continue;
        FileEntry entry{name, itemPath, S_ISDIR(information.st_mode), S_ISLNK(information.st_mode),
                        information.st_mode, information.st_size};
        if (entry.symlink) {
            struct stat target {};
            if (::stat(itemPath.c_str(), &target) == 0) entry.directory = S_ISDIR(target.st_mode);
        }
        entries.push_back(entry);
    }
    ::closedir(directory);
    std::sort(entries.begin() + (path == "/" ? 0 : 1), entries.end(),
        [](const FileEntry& left, const FileEntry& right) {
//---------------------------------------------------------------------------------------------------------------------------------//

            if (left.directory != right.directory) return left.directory > right.directory;
            return left.name < right.name;
        });
    return entries;
}

std::string requireValue(const std::vector<std::string>& args, std::size_t& index,
                         const std::string& option) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (++index >= args.size()) throw std::invalid_argument(option + " requires a value");
    return args[index];
}

}  // namespace

int runFile(const std::vector<std::string>& args) {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::string path = ".";
    std::string cursorText = ">";
    std::string header;
    bool showAll = false;
    bool showPermissions = true;
    bool showSize = true;
    bool allowFile = true;
    bool allowDirectory = false;
    bool showHelp = true;
    int height = 10;
    try {
        for (std::size_t index = 0; index < args.size(); ++index) {
            const std::string& arg = args[index];
            if (arg == "--help" || arg == "-h") { printFileHelp(); return 0; }
            if (arg == "--cursor" || arg == "-c") cursorText = requireValue(args, index, arg);
            else if (arg == "--all" || arg == "-a") showAll = true;
            else if (arg == "--permissions" || arg == "-p") showPermissions = true;
            else if (arg == "--no-permissions") showPermissions = false;
            else if (arg == "--size" || arg == "-s") showSize = true;
            else if (arg == "--no-size") showSize = false;
            else if (arg == "--file") allowFile = true;
            else if (arg == "--no-file") allowFile = false;
            else if (arg == "--directory") allowDirectory = true;
            else if (arg == "--no-directory") allowDirectory = false;
            else if (arg == "--show-help") showHelp = true;
            else if (arg == "--no-show-help") showHelp = false;
            else if (arg == "--header") header = requireValue(args, index, arg);
            else if (arg == "--height") height = std::stoi(requireValue(args, index, arg));
            else if (!arg.empty() && arg[0] == '-') throw std::invalid_argument("unknown option: " + arg);
            else path = arg;
        }
        if (height < 1) throw std::invalid_argument("height must be at least one");
        if (!allowFile && !allowDirectory) throw std::invalid_argument("enable file or directory selection");
        char resolvedPath[PATH_MAX] = {};
        if (::realpath(path.c_str(), resolvedPath) == nullptr) {
            throw std::invalid_argument("cannot resolve directory: " + path);
        }
        path = resolvedPath;
    } catch (const std::exception& error) {
        std::cerr << "hum file: " << error.what() << '\n';
        return 2;
    }

    TerminalSession terminal;
    if (!terminal.interactive() || !terminal.enableRawMode()) {
        std::cerr << "hum file: an interactive terminal is required\n";
        return 2;
    }
    LiveRegion region(terminal);
    std::size_t cursor = 0;
    while (true) {
        std::vector<FileEntry> entries;
        try {
            entries = readDirectory(path, showAll);
        } catch (const std::exception& error) {
            std::cerr << error.what() << '\n';
            return 2;
        }
        if (entries.empty()) cursor = 0;
        else if (cursor >= entries.size()) cursor = entries.size() - 1;
        std::vector<std::string> screen;
        screen.push_back(header.empty() ? path : header + "  " + path);
        const std::size_t start = cursor >= static_cast<std::size_t>(height)
            ? cursor - static_cast<std::size_t>(height) + 1 : 0;
        for (std::size_t row = start; row < entries.size() && row < start + static_cast<std::size_t>(height); ++row) {
            const FileEntry& entry = entries[row];
            std::ostringstream line;
            line << (row == cursor ? cursorText : std::string(cursorText.size(), ' ')) << ' ';
            if (showPermissions) line << permissions(entry.mode) << ' ';
            if (showSize) line << std::setw(8) << (entry.directory ? "-" : fileSize(entry.size)) << ' ';
            line << entry.name << (entry.directory ? "/" : (entry.symlink ? "@" : ""));
            std::string value = line.str();
            if (row == cursor && colorEnabled()) value = "\x1b[38;5;212m" + value + "\x1b[0m";
            screen.push_back(value);
        }
        if (showHelp) screen.push_back("↑/↓ navigate • → open • ← parent • enter select/open • esc cancel");
        region.render(screen);

        const InputEvent event = terminal.readEvent();
        if (event.key == Key::Up || event.text == "k") {
            if (!entries.empty()) cursor = cursor == 0 ? entries.size() - 1 : cursor - 1;
        } else if (event.key == Key::Down || event.text == "j") {
            if (!entries.empty()) cursor = (cursor + 1) % entries.size();
        } else if (event.key == Key::Left || event.key == Key::Backspace || event.text == "h") {
            path = parentPath(path);
            cursor = 0;
        } else if ((event.key == Key::Right || event.text == "l") && !entries.empty() && entries[cursor].directory) {
            path = entries[cursor].path;
            cursor = 0;
        } else if (event.key == Key::Enter && !entries.empty()) {
            const FileEntry& entry = entries[cursor];
            if ((entry.directory && allowDirectory) || (!entry.directory && allowFile)) {
                region.clear();
                std::cout << entry.path << '\n';
                return 0;
            }
            if (entry.directory) { path = entry.path; cursor = 0; }
        } else if (event.key == Key::Escape || event.key == Key::Interrupt || event.key == Key::EndOfInput) {
            return 1;
        }
    }
}

void printFileHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "Usage: hum file [options] [path]\n\n"
        "Browse a directory and select a file or directory.\n\n"
        "Options:\n"
        "  -a, --all                Show hidden entries\n"
        "  -c, --cursor TEXT        Cursor marker\n"
        "  --[no-]permissions      Toggle permissions\n"
        "  --[no-]size             Toggle file sizes\n"
        "  --[no-]file             Toggle file selection\n"
        "  --[no-]directory        Toggle directory selection\n"
        "  --header TEXT           Header above the listing\n"
        "  --height N              Visible entry count\n";
}

}  // namespace hum

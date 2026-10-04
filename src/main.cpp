#include "hum/choose.hpp"
#include "hum/confirm.hpp"
#include "hum/file.hpp"
#include "hum/filter.hpp"
#include "hum/input.hpp"
#include "hum/join.hpp"
#include "hum/log.hpp"
#include "hum/pager.hpp"
#include "hum/spin.hpp"
#include "hum/style.hpp"
#include "hum/table.hpp"
#include "hum/write.hpp"

#include <iostream>
#include <string>
#include <vector>

#ifndef HUM_VERSION
#define HUM_VERSION "unknown"
#endif

namespace {

void printHelp() {
//---------------------------------------------------------------------------------------------------------------------------------//

    std::cout <<
        "hum v" << HUM_VERSION << " ... it makes your Terminal sing.\n\n"
        "Usage: hum <command> [options]\n\n"
        "Commands:\n"
        "  style      Apply colour, borders and spacing to text\n"
        "  confirm    Ask for confirmation using a shell-friendly exit status\n"
        "  join       Join multi-line text horizontally or vertically\n"
        "  log        Print styled or structured log messages\n"
        "  input      Prompt for a single line of input\n"
        "  choose     Select one or more items from a list\n"
        "  spin       Display progress while running a command\n"
        "  pager      Scroll through long text\n"
        "  filter     Fuzzy-filter and select list items\n"
        "  file       Browse and select files or directories\n"
        "  table      Render and select delimited table rows\n"
        "  write      Edit multi-line text\n"
        "  help       Show this help\n"
        "  version    Print the hum version\n\n"
        "Run 'hum <command> --help' for command-specific help.\n";
}

}  // namespace

int main(int argc, char** argv) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (argc < 2) {
        printHelp();
        return 0;
    }

    const std::string command = argv[1];
    std::vector<std::string> args;
    for (int i = 2; i < argc; ++i) args.emplace_back(argv[i]);

    if (command == "style") return hum::runStyle(args);
    if (command == "confirm") return hum::runConfirm(args);
    if (command == "join") return hum::runJoin(args);
    if (command == "log") return hum::runLog(args);
    if (command == "input") return hum::runInput(args);
    if (command == "choose") return hum::runChoose(args);
    if (command == "spin") return hum::runSpin(args);
    if (command == "pager") return hum::runPager(args);
    if (command == "filter") return hum::runFilter(args);
    if (command == "file") return hum::runFile(args);
    if (command == "table") return hum::runTable(args);
    if (command == "write") return hum::runWrite(args);
    if (command == "help" || command == "--help" || command == "-h") {
        printHelp();
        return 0;
    }
    if (command == "version" || command == "--version" || command == "-v") {
        std::cout << "hum " << HUM_VERSION << '\n';
        return 0;
    }

    std::cerr << "hum: unknown command '" << command << "'\n"
              << "Run 'hum help' to see available commands.\n";
    return 2;
}

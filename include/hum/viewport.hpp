#pragma once

#include "hum/terminal.hpp"

#include <string>
#include <vector>

namespace hum {

struct TerminalDimensions {
    int columns = 80;
    int rows = 24;
};

TerminalDimensions terminalDimensions();

class LiveRegion {
public:
    explicit LiveRegion(TerminalSession& terminal);
    ~LiveRegion();

    LiveRegion(const LiveRegion&) = delete;
    LiveRegion& operator=(const LiveRegion&) = delete;

    void render(const std::vector<std::string>& lines);
    void clear();

private:
    TerminalSession& terminal_;
    int renderedLines_ = 0;
};

std::string fitText(const std::string& value, int width);

}  // namespace hum

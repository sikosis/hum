#include "hum/viewport.hpp"

#include "hum/text.hpp"

#include <algorithm>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace hum {

TerminalDimensions terminalDimensions() {
//---------------------------------------------------------------------------------------------------------------------------------//

    TerminalDimensions dimensions;
    int descriptor = ::open("/dev/tty", O_RDONLY);
    if (descriptor < 0) descriptor = STDIN_FILENO;
    struct winsize size {};
    if (::ioctl(descriptor, TIOCGWINSZ, &size) == 0) {
        if (size.ws_col > 0) dimensions.columns = size.ws_col;
        if (size.ws_row > 0) dimensions.rows = size.ws_row;
    }
    if (descriptor != STDIN_FILENO) ::close(descriptor);
    return dimensions;
}

LiveRegion::LiveRegion(TerminalSession& terminal)
    : terminal_(terminal) {
//---------------------------------------------------------------------------------------------------------------------------------//

    terminal_.write("\x1b[?25l");
}

LiveRegion::~LiveRegion() {
//---------------------------------------------------------------------------------------------------------------------------------//

    clear();
    terminal_.write("\x1b[?25h");
}

void LiveRegion::render(const std::vector<std::string>& lines) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (renderedLines_ > 0) terminal_.write("\x1b[" + std::to_string(renderedLines_) + "A");
    const int lineCount = static_cast<int>(lines.size());
    const int totalLines = std::max(renderedLines_, lineCount);
    for (int index = 0; index < totalLines; ++index) {
        terminal_.write("\r\x1b[2K");
        if (index < lineCount) terminal_.write(lines[static_cast<std::size_t>(index)]);
        terminal_.write("\n");
    }
    renderedLines_ = totalLines;
}

void LiveRegion::clear() {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (renderedLines_ <= 0) return;
    terminal_.write("\x1b[" + std::to_string(renderedLines_) + "A");
    for (int index = 0; index < renderedLines_; ++index) {
        terminal_.write("\r\x1b[2K");
        if (index + 1 < renderedLines_) terminal_.write("\x1b[1B");
    }
    terminal_.write("\r");
    renderedLines_ = 0;
}

std::string fitText(const std::string& value, int width) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (width <= 0) return "";
    if (displayWidth(value) <= width) return value + std::string(static_cast<std::size_t>(width - displayWidth(value)), ' ');
    std::string result;
    std::size_t position = 0;
    const int available = std::max(0, width - 1);
    while (position < value.size() && displayWidth(result) < available) {
        std::size_t next = position + 1;
        while (next < value.size() && (static_cast<unsigned char>(value[next]) & 0xc0) == 0x80) ++next;
        const std::string character = value.substr(position, next - position);
        if (displayWidth(result) + displayWidth(character) > available) break;
        result += character;
        position = next;
    }
    return width == 1 ? "…" : result + "…" +
        std::string(static_cast<std::size_t>(std::max(0, available - displayWidth(result))), ' ');
}

}  // namespace hum

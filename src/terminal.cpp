#include "hum/terminal.hpp"

#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

namespace hum {
namespace {

bool readByte(int fd, unsigned char& value, int timeoutMilliseconds) {
//---------------------------------------------------------------------------------------------------------------------------------//

    struct pollfd descriptor {fd, POLLIN, 0};
    int result = 0;
    do {
        result = ::poll(&descriptor, 1, timeoutMilliseconds);
    } while (result < 0 && errno == EINTR);
    return result > 0 && ::read(fd, &value, 1) == 1;
}

}  // namespace

TerminalSession::TerminalSession() {
//---------------------------------------------------------------------------------------------------------------------------------//

    fd_ = ::open("/dev/tty", O_RDWR);
    if (fd_ >= 0) {
        ownsFd_ = true;
    } else {
        fd_ = STDIN_FILENO;
    }
}

TerminalSession::~TerminalSession() {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (raw_) {
        ::tcsetattr(fd_, TCSAFLUSH, &original_);
    }
    if (ownsFd_) {
        ::close(fd_);
    }
}

bool TerminalSession::interactive() const {
//---------------------------------------------------------------------------------------------------------------------------------//

    return fd_ >= 0 && ::isatty(fd_) != 0;
}

bool TerminalSession::enableRawMode() {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (!interactive() || ::tcgetattr(fd_, &original_) != 0) {
        return false;
    }
    struct termios raw = original_;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG));
    raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    if (::tcsetattr(fd_, TCSAFLUSH, &raw) != 0) {
        return false;
    }
    raw_ = true;
    return true;
}

Key TerminalSession::readKey(int timeoutMilliseconds) {
//---------------------------------------------------------------------------------------------------------------------------------//

    return readEvent(timeoutMilliseconds).key;
}

InputEvent TerminalSession::readEvent(int timeoutMilliseconds) {
//---------------------------------------------------------------------------------------------------------------------------------//

    struct pollfd descriptor {fd_, POLLIN, 0};
    const int result = ::poll(&descriptor, 1, timeoutMilliseconds > 0 ? timeoutMilliseconds : -1);
    if (result == 0) {
        return {Key::Timeout, {}};
    }
    if (result < 0) {
        return {errno == EINTR ? Key::Interrupt : Key::EndOfInput, {}};
    }

    unsigned char c = 0;
    if (::read(fd_, &c, 1) != 1) {
        return {Key::EndOfInput, {}};
    }
    if (c == 3) {
        return {Key::Interrupt, {}};
    }
    if (c == 4) {
        return {Key::Submit, {}};
    }
    if (c == '\r' || c == '\n') {
        return {Key::Enter, {}};
    }
    if (c == '\t') {
        return {Key::Tab, {}};
    }
    if (c == 0x7f || c == 0x08) {
        return {Key::Backspace, {}};
    }
    if (c == 'y' || c == 'Y') {
        return {Key::Yes, "y"};
    }
    if (c == 'n' || c == 'N') {
        return {Key::No, "n"};
    }
    if (c != 0x1b) {
        std::string text(1, static_cast<char>(c));
        int continuationBytes = 0;
        if ((c & 0xe0) == 0xc0) continuationBytes = 1;
        else if ((c & 0xf0) == 0xe0) continuationBytes = 2;
        else if ((c & 0xf8) == 0xf0) continuationBytes = 3;
        for (int i = 0; i < continuationBytes; ++i) {
            unsigned char continuation = 0;
            if (!readByte(fd_, continuation, 150)) break;
            text.push_back(static_cast<char>(continuation));
        }
        return {Key::Unknown, text};
    }

    // Some terminals, including Haiku Terminal, can deliver the bytes in an
    // escape sequence separately. Allow enough time for the complete key
    // sequence instead of mistaking its first byte for the Escape key.
    unsigned char next = 0;
    if (!readByte(fd_, next, 150)) {
        return {Key::Escape, {}};
    }

    std::string sequence(1, static_cast<char>(next));
    if (next == '[' || next == 'O') {
        while (sequence.size() < 16) {
            if (!readByte(fd_, next, 150)) {
                return {Key::Escape, {}};
            }
            sequence.push_back(static_cast<char>(next));
            if (next >= 0x40 && next <= 0x7e) {
                break;
            }
        }
    }

    return {decodeEscapeSequence(sequence), {}};
}

Key decodeEscapeSequence(const std::string& sequence) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (sequence.empty()) {
        return Key::Escape;
    }
    if ((sequence.front() == '[' || sequence.front() == 'O') && sequence.back() == 'C') {
        return Key::Right;
    }
    if ((sequence.front() == '[' || sequence.front() == 'O') && sequence.back() == 'D') {
        return Key::Left;
    }
    if ((sequence.front() == '[' || sequence.front() == 'O') && sequence.back() == 'A') {
        return Key::Up;
    }
    if ((sequence.front() == '[' || sequence.front() == 'O') && sequence.back() == 'B') {
        return Key::Down;
    }
    if (sequence == "[H" || sequence == "OH" || sequence == "[1~" || sequence == "[7~") {
        return Key::Home;
    }
    if (sequence == "[F" || sequence == "OF" || sequence == "[4~" || sequence == "[8~") {
        return Key::End;
    }
    if (sequence == "[3~") {
        return Key::Delete;
    }
    if (sequence == "[5~") {
        return Key::PageUp;
    }
    if (sequence == "[6~") {
        return Key::PageDown;
    }
    return Key::Unknown;
}

void TerminalSession::write(const std::string& value) const {
//---------------------------------------------------------------------------------------------------------------------------------//

    const char* data = value.data();
    std::size_t remaining = value.size();
    while (remaining > 0) {
        const ssize_t count = ::write(fd_, data, remaining);
        if (count <= 0) {
            break;
        }
        data += count;
        remaining -= static_cast<std::size_t>(count);
    }
}

bool colorEnabled() {
//---------------------------------------------------------------------------------------------------------------------------------//

    return std::getenv("NO_COLOR") == nullptr;
}

}  // namespace hum

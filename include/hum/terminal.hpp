#pragma once

#include <string>
#include <termios.h>

namespace hum {

enum class Key {
    Unknown,
    Enter,
    Escape,
    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    PageUp,
    PageDown,
    Backspace,
    Delete,
    Submit,
    Tab,
    Yes,
    No,
    Interrupt,
    EndOfInput,
    Timeout
};

struct InputEvent {
    Key key = Key::Unknown;
    std::string text;
};

class TerminalSession {
public:
    TerminalSession();
    ~TerminalSession();

    TerminalSession(const TerminalSession&) = delete;
    TerminalSession& operator=(const TerminalSession&) = delete;

    bool interactive() const;
    bool enableRawMode();
    InputEvent readEvent(int timeoutMilliseconds = 0);
    Key readKey(int timeoutMilliseconds = 0);
    void write(const std::string& value) const;

private:
    int fd_ = -1;
    bool ownsFd_ = false;
    bool raw_ = false;
    struct termios original_ {};
};

bool colorEnabled();
Key decodeEscapeSequence(const std::string& sequence);

}  // namespace hum

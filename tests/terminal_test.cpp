#include "hum/terminal.hpp"

#include <iostream>
#include <string>

namespace {

bool expect(const std::string& description, hum::Key actual, hum::Key expected) {
//---------------------------------------------------------------------------------------------------------------------------------//

    if (actual == expected) {
        return true;
    }
    std::cerr << "FAIL: " << description << '\n';
    return false;
}

}  // namespace

int main() {
//---------------------------------------------------------------------------------------------------------------------------------//

    bool passed = true;
    passed &= expect("standalone escape", hum::decodeEscapeSequence(""), hum::Key::Escape);
    passed &= expect("CSI right", hum::decodeEscapeSequence("[C"), hum::Key::Right);
    passed &= expect("CSI left", hum::decodeEscapeSequence("[D"), hum::Key::Left);
    passed &= expect("SS3 right", hum::decodeEscapeSequence("OC"), hum::Key::Right);
    passed &= expect("SS3 left", hum::decodeEscapeSequence("OD"), hum::Key::Left);
    passed &= expect("modified right", hum::decodeEscapeSequence("[1;5C"), hum::Key::Right);
    passed &= expect("modified left", hum::decodeEscapeSequence("[1;5D"), hum::Key::Left);
    passed &= expect("CSI up", hum::decodeEscapeSequence("[A"), hum::Key::Up);
    passed &= expect("CSI down", hum::decodeEscapeSequence("[B"), hum::Key::Down);
    passed &= expect("home", hum::decodeEscapeSequence("[H"), hum::Key::Home);
    passed &= expect("end", hum::decodeEscapeSequence("OF"), hum::Key::End);
    passed &= expect("delete", hum::decodeEscapeSequence("[3~"), hum::Key::Delete);
    passed &= expect("page up", hum::decodeEscapeSequence("[5~"), hum::Key::PageUp);
    passed &= expect("page down", hum::decodeEscapeSequence("[6~"), hum::Key::PageDown);

    if (!passed) {
        return 1;
    }
    std::cout << "All terminal tests passed\n";
    return 0;
}

#ifndef INPUT_H
#define INPUT_H

// Minimal cross-platform non-blocking console input helper for demo purposes.
// Uses termios on POSIX.

class Input {
public:
    static void Init();
    static void Shutdown();
    // Returns -1 if no key, otherwise ASCII code of key.
    static int PollKey();
};

#endif // INPUT_H

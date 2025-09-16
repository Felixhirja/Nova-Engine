#ifdef __linux__
#include "Input.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

static struct termios oldt;

void Input::Init() {
    struct termios newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

void Input::Shutdown() {
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
}

int Input::PollKey() {
    unsigned char ch;
    ssize_t n = read(STDIN_FILENO, &ch, 1);
    if (n == 1) return ch;
    return -1;
}

#else
#include "Input.h"
#include <iostream>

void Input::Init() { }
void Input::Shutdown() { }
int Input::PollKey() { return -1; }
#endif

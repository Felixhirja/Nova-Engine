#ifdef __linux__
#include "Input.h"
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#ifdef USE_SDL
#include <SDL2/SDL.h>
#endif

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
#ifdef USE_SDL
    // Poll SDL events first (if SDL is used for windowing/input)
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) return 'q';
        if (e.type == SDL_KEYDOWN) {
            SDL_Keycode kc = e.key.keysym.sym;
            if (kc == SDLK_a) return 'a';
            if (kc == SDLK_d) return 'd';
            if (kc == SDLK_SPACE) return ' ';
            if (kc == SDLK_q) return 'q';
            if (kc == SDLK_z) return 'z';
            if (kc == SDLK_x) return 'x';
        }
    }
#endif

    unsigned char ch;
    ssize_t n = read(STDIN_FILENO, &ch, 1);
    if (n == 1) return ch;
    return -1;
}

#else
#include "Input.h"

void Input::Init() {}
void Input::Shutdown() {}
int Input::PollKey() { return -1; }
#endif

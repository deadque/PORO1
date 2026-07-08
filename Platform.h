#pragma once

#ifdef _WIN32
    #include <windows.h>
    #include <conio.h>
    inline void setup_console() {
        SetConsoleOutputCP(1251);
        SetConsoleCP(1251);
    }
    inline void clear_screen() { system("cls"); }
    inline int read_key() { return _getch(); }
    #else
    #include <termios.h>
    #include <unistd.h>
    inline void setup_console() {}
    inline void clear_screen() { system("clear"); }
    inline int read_key() {
        termios oldt{}, newt{};
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        int ch = getchar();
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        return ch;
}
#endif

#include <stdio.h>
#include <stdlib.h>

static char* console_buffer = NULL;
static size_t console_buffer_size = 0;

static size_t WIDTH = 0; 
static size_t HEIGHT = 0;

#ifdef _WIN32
    /* Windows */
    #include <windows.h>

    void enableVtTerminal(void) { 
        // enable virtual terminal for ANSI escape codes
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD dwOutMode = 0;
        GetConsoleMode(hOut, &dwOutMode);
        SetConsoleMode(hOut, dwOutMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING); 

        // disable processed input so "Ctrl+{key}" goes to input stream
        HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);
        DWORD dwInMode = 0;
        GetConsoleMode(hIn, &dwInMode);
        
        // remove ENABLE_PROCESSED_INPUT, ENABLE_LINE_INPUT and ENABLE_ECHO_INPUT
        dwInMode &= ~(ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
        SetConsoleMode(hIn, dwInMode);
    }

    int getTerminalHeight(void) { 
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

        return (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    }

    int getTerminalWidth(void) { 
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);

        return (csbi.srWindow.Right - csbi.srWindow.Left + 1);
    }

    long long getTimeMs(void) {
        FILETIME ft;
        GetSystemTimeAsFileTime(&ft);

        ULARGE_INTEGER uli;
        uli.LowPart = ft.dwLowDateTime;
        uli.HighPart = ft.dwHighDateTime;
        return uli.QuadPart / 10000;
    }

#else
    /* Linux / POSIX */
    #include <termios.h>
    #include <time.h>
    #include <unistd.h>
    #include <sys/ioctl.h>
    #include <sys/select.h>

    void enableVtTerminal(void) {}

    int getTerminalHeight(void) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        return w.ws_row;
    }

    int getTerminalWidth(void) {
        struct winsize w;
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);

        return w.ws_col;  
    }

    static void enableRawMode(struct termios *orig) {
        tcgetattr(STDIN_FILENO, orig);

        struct termios raw = *orig;

        raw.c_lflag &= ~(ECHO | ICANON);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    }

    static void disableRawMode(struct termios *orig) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, orig);
    }

    char getch(void) {
        char c = 0;
        struct termios orig;
        disableRawMode(&orig);
        
        if (read(STDIN_FILENO, &c, 1) < 0) {
            c = 0;
        }
        
        disable_raw_mode(&orig);

        return c;
    }

    int kbhit(void) {
        struct termios orig;
        enableRawMode(&orig);

        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);

        struct timeval timeout = {0, 0}; 
        int result = select(STDIN_FILENO + 1, &readfds, NULL, NULL, &timeout);

        disable_raw_mode(&orig);

        return result > 0;
    }

    long long getTimeMs(void) {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }

#endif

void initTerminal(void) {
    enableVtTerminal();
    
    //  alt screen (\x1b[?1049h), hide cursor (\x1b[?25l), disable line wrap (\x1b[?7l)
    fputs("\x1b[?1049h\x1b[?25l\x1b[?7l", stdout); 
    fflush(stdout);
    
    WIDTH = (size_t)getTerminalWidth();
    HEIGHT = (size_t)getTerminalHeight();

    console_buffer_size = (WIDTH + 1) * HEIGHT + 16;    
    console_buffer = (char*)malloc(console_buffer_size);
}

int fillConsoleBuffer(char** buffer, size_t b_rows, size_t b_cols) {
    size_t needed_size = (b_cols + 1) * b_rows + 16;
    if (needed_size > console_buffer_size) {
        console_buffer_size = needed_size;
        console_buffer = (char*)realloc(console_buffer, console_buffer_size);
    }

    int offset = 0;
    
    
    offset += sprintf(console_buffer + offset, "\x1b[H");   // goto(0, 0)

    for (size_t i = 0; i < b_rows; i++) {
        for (size_t j = 0; j < b_cols; j++) {
            console_buffer[offset++] = buffer[i][j];
        }
        
        if (i < b_rows - 1) {       // dont print '\n' on the last row to not trigger terminal autoscroll
            console_buffer[offset++] = '\n';
        }
    }
    
    return offset;
}

void refreshBuffer(char** buffer, size_t b_rows, size_t b_cols) {   // prints current buffer to the screen
    int offset = fillConsoleBuffer(buffer, b_rows, b_cols);
    fwrite(console_buffer, sizeof(char), (size_t)offset, stdout);
    fflush(stdout);
}

void cleanUpTerminal(void) {        
    // Show cursor (\x1b[?25h) and restore main screen buffer (\x1b[?1049l)
    fputs("\x1b[?25h\x1b[?1049l", stdout);
    fflush(stdout);

    if (console_buffer != NULL) {
        free(console_buffer);
        console_buffer = NULL;
    }
}

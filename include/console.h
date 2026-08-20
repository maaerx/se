#ifndef EDITOR_CONSOLE_H
#define EDITOR_CONSOLE_H

#include <stdio.h>

static char* console_buffer;
static size_t console_buffer_size;

static size_t WIDTH; 
static size_t HEIGHT;

#ifdef _WIN32
    /* Windows */

    #include <windows.h>
    #include <conio.h>

    int getTerminalHeight(void);

    int getTerminalWidth(void);

    long long getTimeMs(void);

#else
    /* Linux */

    #include <sys/ioctl.h>
    #include <stdio.h>

    int getTerminalHeight(void);

    int getTerminalWidth(void);

    long long getTimeMs(void);

    char getch(void);

    int kbhit(void);
    
#endif


void initTerminal(void);

int refreshBuffer(char** buffer, size_t b_rows, size_t b_cols);

void cleanUpTerminal(void);


#endif

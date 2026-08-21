#include <stdio.h>
#include <stdlib.h>

#ifndef _WIN32
#include <string.h>
#include <unistd.h>
#endif

#include "console.h"

#define    ASCII_NEWLINE    10   

#define    K_UP_ARROW       72
#define    K_DOWN_ARROW     80
#define    K_LEFT_ARROW     75
#define    K_RIGHT_ARROW    77
#define    K_ENTER          13
#define    K_BACKSPACE       8
#define    K_DEL            83        
#define    K_TAB             9        

#define    K_CTRL_X         24      // ctrl+x keycode
#define    K_CTRL_S         19      // ctrl+s keycode

#define    INTV_BLINK      600      // Blink intervall
#define    INTV_UPDATE      20      // Input reading intervall

// Single Line
typedef struct {
    char* data;             // array of characters
    size_t text_length;     // length of actual characters in the array
    size_t mem_length;      // memory size of the array
} Line; 

// Editor
typedef struct {
    Line** lines;           // array of Lines*
    size_t line_length;     // size of array -> depends on file size
} Editor;

// Cursor
typedef struct {
    char c;                 // console character
    int visible;            // is visible?
    int line;               // current line
    int column;             // current column
    int offset_y;           // offset vor viewing files larger than terminal height
    int offset_x;
} Cursor;

// File Buffer
typedef struct {
    int* data;              // array of ints (file content)
    size_t size;            // size of file content
} FileBuffer;

// Screen
typedef struct {
    char** data;            // array of chars
    size_t height;          // terminal height
    size_t width;           // terminal width
} Screen;

/* Struct creation */

Line* createLine(size_t size) { // creates line
    Line* line = malloc(sizeof(Line));  // allocate memory for line

    if(line == NULL) {                  // check
        return NULL;
    }

    line->data = calloc(size + 20, sizeof(char));    // create array of chars
    line->text_length = size;                        // set the text length
    line->mem_length = size + 20;                    // set the memory length 
    
    return line;
}

void freeLine(Line* line) { // frees a Line
    if(line != NULL) {
        free(line->data);
        free(line);
    }
}   

Editor* createEditor(FileBuffer* filebuffer) { // creates editor
    Editor* editor = malloc(sizeof(Editor)); // allocate Memory for Editor

    if(editor == NULL) {                            // check
        return NULL;
    }

    size_t row_cnt = 1;                             // get the amount of rows in the file Buffer
    for (size_t i = 0; i < filebuffer->size; i++)
    {
        if(filebuffer->data[i] == ASCII_NEWLINE) {
            row_cnt++;
        }
    }

    editor->lines = malloc(row_cnt * sizeof(Line*)); // allocate Memory for Lines

    if(editor->lines == NULL) {                      // check
        free(editor);
        return NULL;
    }

    editor->line_length = row_cnt;                          // set editor->line_length

    size_t buffer_index = 0;                         // index for file buffer
    for (size_t i = 0; i < row_cnt; i++)             // go through every row
    {
        size_t line_length = 0;                      // length of each line
        size_t temp_index = buffer_index;
        while(temp_index < filebuffer->size && filebuffer->data[temp_index] != ASCII_NEWLINE) {
            line_length++;
            temp_index++;
        }

        editor->lines[i] = createLine(line_length);
        if(editor->lines[i] == NULL) {               // check
            for (size_t k = 0; k < i; k++)
            {
                free(editor->lines[k]);
            }

            free(editor->lines);
            free(editor);

            return NULL;
        }

        for (size_t j = 0; j < line_length; j++)    // fill the line with filebuffer
        {
            editor->lines[i]->data[j] = (char)filebuffer->data[buffer_index++];
        }

        if(buffer_index < filebuffer->size && filebuffer->data[buffer_index] == ASCII_NEWLINE) { // skip over '\n' character
            buffer_index++;
        }
        
    }


    return editor;
}

void freeEditor(Editor* editor) { // frees editor
    if(editor != NULL) {
        for (size_t i = 0; i < editor->line_length; i++)
        {
            freeLine(editor->lines[i]);
        }

        free(editor->lines);
        free(editor);
    }
}

FileBuffer* loadFile(char* filename) { // creates file buffer
    FILE* file;

    FileBuffer* filebuffer = malloc(sizeof(FileBuffer));
    if(filebuffer == NULL) {
        return NULL;
    }

    file = fopen(filename, "r");    // Open file
    if(file == NULL) {              // check if file is not NULL
        free(filebuffer);
        return NULL;
    }

    size_t size = 0;                    // Get the length of the file

    while(fgetc(file) != EOF) {
        size++;
    }

    filebuffer->size = size;        // set filebuffer size to length of file
    filebuffer->data = malloc(size * sizeof(int));

    if(filebuffer->data == NULL) {
        free(filebuffer);
        fclose(file);
        return NULL;
    }

    rewind(file);                   // rewind file pointer

    for (size_t i = 0; i < size; i++)
    {
        filebuffer->data[i] = fgetc(file);
    }

    fclose(file);    

    return filebuffer;
}

void freeFileBuffer(FileBuffer* filebuffer) {
    if(filebuffer != NULL) {
        free(filebuffer->data);
        free(filebuffer);
    }
}

Screen* createScreen(size_t height, size_t width) { // creates screen
    Screen* screen = malloc(sizeof(Screen));

    if(screen == NULL) {
        return NULL;
    }

    screen->data = calloc(height, sizeof(char*));

    if(screen->data == NULL) {
        free(screen);
        return NULL;
    }

    for (size_t i = 0; i < height; i++)
    {
        screen->data[i] = calloc(width, sizeof(char));
        
        if(screen->data[i] == NULL) {
            free(screen->data);
            free(screen);
            return NULL;
        }
    }

    screen->height = height;
    screen->width = width;
    
    return screen;
}

void freeScreen(Screen* screen) {
    if(screen != NULL) {
        for (size_t i = 0; i < screen->height; i++)
        {
            free(screen->data[i]);
        }

        free(screen->data);
        free(screen);
    }
}

/* Cursor Functions */

void moveCursorDown(Editor* editor, Cursor* cursor, size_t screen_height, size_t screen_width) { // moves cursor down
    int new_line = cursor->line + 1;
    int new_column = 0;

    if(new_line >= editor->line_length) {
        return;
    }

    while(new_column < editor->lines[new_line]->text_length && new_column < cursor->column) {
        new_column++;
    }

    cursor->line = new_line;
    cursor->column = new_column;

    if(cursor->line - cursor->offset_y >= screen_height - 1) {
        cursor->offset_y++;
    }

    if(cursor->column < cursor->offset_x) {
        cursor->offset_x = cursor->column;
    } else if(cursor->column - cursor->offset_x >= screen_width - 1) {
        cursor->offset_x = cursor->column - (screen_width - 2);
    }
}

void moveCursorUp(Editor* editor, Cursor* cursor) { // moves cursor up
    int new_line = cursor->line - 1;
    int new_column = 0;

    if(new_line < 0) {
        return;
    }

    cursor->line = new_line;

    if(cursor->line - cursor->offset_y < 0) {
        cursor->offset_y--;
    }

    while(new_column < editor->lines[new_line]->text_length && new_column < cursor->column) {
        new_column++;
    }

    cursor->column = new_column;

    if(cursor->column < cursor->offset_x) {
        cursor->offset_x = cursor->column;
    }
}

void moveCursorRight(Editor* editor, Cursor* cursor, size_t screen_height, size_t screen_width) { // moves cursor right
    int new_line = cursor->line;
    int new_column = cursor->column;

    if(new_column >= editor->lines[new_line]->text_length) {
        new_line++;
        new_column = 0;
    } else {
        new_column++;
    }

    if((size_t)new_line >= editor->line_length) {
        return;
    }

    cursor->line = new_line;
    cursor->column = new_column;

    if(cursor->line - cursor->offset_y >= screen_height - 1) {
        cursor->offset_y++;
    }

    if(cursor->column < cursor->offset_x) {
        cursor->offset_x = 0;
    } else if(cursor->column - cursor->offset_x >= screen_width - 1) {
        cursor->offset_x++;
    }
}

void moveCursorLeft(Editor* editor, Cursor* cursor, size_t screen_width) { // moves cursor left
    int new_line = cursor->line;
    int new_column = cursor->column;

    if(new_line == 0 && new_column == 0) {
        return;
    }

    if(new_column <= 0) {
        new_line--;
        new_column = editor->lines[new_line]->text_length;
    } else {
        new_column--;
    }

    cursor->line = new_line;
    cursor->column = new_column;

    if(cursor->line - cursor->offset_y < 0) {
        cursor->offset_y--;
    }

    if(cursor->column < cursor->offset_x) {
        cursor->offset_x = cursor->column;
    } else if(cursor->column - cursor->offset_x >= screen_width - 1) {
        cursor->offset_x = cursor->column - (screen_width - 2);
    }
}

/* Character inserting */

void resizeLine(Line* line, int add) { // resizes line data array
    if(add == 0) {
        return;
    } 

    size_t new_mem = (size_t)((int)line->mem_length + add);         // find out new memory size

    if (new_mem < line->text_length + 1) {                          // check
        return; 
    } 

    char* buffer = realloc(line->data, new_mem * sizeof(char));     // reallocate buffer

    if (buffer == NULL) {                                           // check
        return;
    } 

    line->data = buffer;           
    line->mem_length = new_mem;
}

void insertChar(Line* line, char c, size_t pos_col) {
    if(line->text_length + 2 > line->mem_length) {
        resizeLine(line, 20);
    }

    for (size_t i = line->text_length; i > pos_col; i--)
    {
        line->data[i] = line->data[i - 1];
    }
    
    line->data[pos_col] = c;
    line->text_length++;
}

void deleteChar(Line* line, size_t pos_col) {
    for (size_t i = pos_col; i < line->text_length - 1; i++)
    {
        line->data[i] = line->data[i + 1];
    }

    line->data[line->text_length] = 0;
    line->text_length--;
    
    if(line->mem_length - 20 > line->text_length) {
        resizeLine(line, -20);
    }
}

void resizeEditor(Editor* editor, int add) {
    if(add == 0) {
        return;
    }

    size_t new_mem = (size_t)((int)editor->line_length + add);      // find out new memory size

    Line** buffer = realloc(editor->lines, new_mem * sizeof(Line*));

    if(buffer == NULL) {
        return;
    }

    editor->lines = buffer;
    editor->line_length = new_mem;
}

void insertEnter(Editor* editor, size_t pos_line, size_t pos_col) {
    Line* current_line = editor->lines[pos_line];

    size_t split_len = current_line->text_length - pos_col;

    Line* split_line = createLine(split_len);

    if(split_line == NULL) {
        return;
    }


    for (size_t i = 0; i < split_len; i++)
    {
        split_line->data[i] = current_line->data[pos_col + i]; 
    }
    
    for (size_t i = pos_col; i < current_line->text_length; i++)
    {
        current_line->data[i] = 0;
    }

    current_line->text_length = pos_col;

    resizeEditor(editor, 1);

    for (size_t i = editor->line_length - 1; i > pos_line + 1; i--)
    {
        editor->lines[i] = editor->lines[i - 1];
    }



    editor->lines[pos_line + 1] = split_line;
}

void deleteCharLineBegin(Editor* editor, Cursor* cursor) {
    size_t pos_line = cursor->line;

    if(pos_line == 0) {
        return;
    }
   
    Line* current_line = editor->lines[pos_line];
    Line* upper_line = editor->lines[pos_line - 1];

    cursor->column = upper_line->text_length;
    cursor->line--;

    if(cursor->line - cursor->offset_y < 0) {
        cursor->offset_y--;
    }


    if(upper_line->mem_length <= upper_line->text_length + current_line->text_length + 2) {
        resizeLine(upper_line, current_line->text_length);
    }

    for (size_t i = 0; i < current_line->text_length; i++)
    {
        upper_line->data[upper_line->text_length + i] = current_line->data[i];
    }

    upper_line->text_length += current_line->text_length;

    freeLine(current_line);

    for (size_t i = pos_line; i < editor->line_length - 1; i++)
    {
        editor->lines[i] = editor->lines[i + 1];
    }
    
    resizeEditor(editor, -1);
}
/* Render functions */

void renderScreen(Screen* screen, Editor* editor, Cursor* cursor) {
    // clear screen
    for (size_t i = 0; i < screen->height; i++)
    {
        memset(screen->data[i], ' ', screen->width);
        
    }

    size_t line_idx_y = cursor->offset_y;
    size_t screen_idx_y = 0;

    size_t line_idx_x;
    size_t screen_idx_x; 
    

    while (line_idx_y < editor->line_length && screen_idx_y < screen->height) {  // editor printing
        line_idx_x = cursor->offset_x;
        screen_idx_x = 0;

        while (line_idx_x < editor->lines[line_idx_y]->text_length && screen_idx_x < screen->width) {
            screen->data[screen_idx_y][screen_idx_x] = editor->lines[line_idx_y]->data[line_idx_x];

            line_idx_x++;
            screen_idx_x++;
        }
        
        screen_idx_y++;
        line_idx_y++;
    }
    
    if (cursor->visible && cursor->line >= cursor->offset_y) {      // cursor printing
        int screen_y = cursor->line - cursor->offset_y;
        int screen_x = cursor->column - cursor->offset_x;

        if (screen_y >= 0 && screen_y < (int)screen->height - 1 && screen_x >= 0 && screen_x < (int)screen->width - 1) {

            screen->data[screen_y][screen_x] = cursor->c;
        }
    }
    

    refreshBuffer(screen->data, screen->height, screen->width);
}

/* File functions */

void saveFile(Editor* editor, char* filename) {
    FILE* file;

    file = fopen(filename, "w");

    if(file == NULL) {
        return;
    }

    for (size_t i = 0; i < editor->line_length; i++)
    {
        for (size_t j = 0; j < editor->lines[i]->text_length; j++)
        {
            fputc((int)(editor->lines[i]->data[j]), file);
        }
        
        if(i != editor->line_length - 1) {
            fputc('\n', file);
        }
    }

    fclose(file);
}

/* Editor Main Functions */

int readKey() {
    int c = getch();
    if(c == 0) return 0;

#ifdef _WIN32
    if(c == 224 || c == 0) {
        c = getch();

        switch (c)
        {
            case K_UP_ARROW: return K_UP_ARROW;
            case K_DOWN_ARROW: return K_DOWN_ARROW;
            case K_LEFT_ARROW: return K_LEFT_ARROW;
            case K_RIGHT_ARROW: return K_RIGHT_ARROW;
            case K_DEL: return K_DEL;
        }
        return c;
    }
#else
    // linux anis escape sequences
    if (c == 27) { // escape character
        char seq[3];

        if (read(STDIN_FILENO, &seq[0], 1) <= 0) return 27;
        if (read(STDIN_FILENO, &seq[1], 1) <= 0) return 27;

        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return K_UP_ARROW;
                case 'B': return K_DOWN_ARROW;
                case 'C': return K_RIGHT_ARROW;
                case 'D': return K_LEFT_ARROW;
                case '3': 
                    if (read(STDIN_FILENO, &seq[2], 1) > 0 && seq[2] == '~') {
                        return K_DEL;
                    }
                    break;
            }
        }
        return 0; 
    }
    
    if (c == 127) { 
        return K_BACKSPACE;
    }

#endif

    return c;

}

void editorMain(char* filename) {
    FileBuffer* filebuffer = loadFile(filename);
    Editor* editor = createEditor(filebuffer);
    Screen* screen = createScreen(getTerminalHeight(), getTerminalWidth());

    Cursor cursor = { '_', 1, 0, 0, 0, 0};

    int is_running = 1;

    long long timeUpdate = getTimeMs();
    long long timeBlink = getTimeMs();

    renderScreen(screen, editor, &cursor);

    while(is_running) {
        
        if(getTimeMs() - timeUpdate >= INTV_UPDATE)
        {
            if(kbhit()) {
                cursor.visible = 1;
                timeBlink = getTimeMs();

                int c = readKey();

                switch (c)
                {
                case K_DOWN_ARROW: moveCursorDown(editor, &cursor, screen->height, screen->width); break;
                case K_UP_ARROW: moveCursorUp(editor, &cursor); break;
                case K_LEFT_ARROW: moveCursorLeft(editor, &cursor, screen->width); break;
                case K_RIGHT_ARROW: moveCursorRight(editor, &cursor, screen->height, screen->width); break;
                
                case K_BACKSPACE:
                    if(cursor.column == 0){
                        deleteCharLineBegin(editor, &cursor);
                    } else {
                        moveCursorLeft(editor, &cursor, screen->width);
                        deleteChar(editor->lines[cursor.line], cursor.column);
                    }
                    break;

                case K_ENTER:
                    insertEnter(editor, cursor.line, cursor.column);
                    moveCursorRight(editor, &cursor, screen->height, screen->width);
                    break;

                case K_TAB: break;

                case K_CTRL_X: is_running = 0; break;
                case K_CTRL_S: saveFile(editor, filename); break;

                default:
                    insertChar(editor->lines[cursor.line], c, cursor.column);

                    cursor.column++;
                    if (cursor.column - cursor.offset_x >= screen->width - 1) {
                        cursor.offset_x++;
                    }
                }
            
                renderScreen(screen, editor, &cursor);

            }

            timeUpdate = getTimeMs();
        
        }
        
        if(getTimeMs() - timeBlink >= INTV_BLINK)
        {
            if(cursor.visible == 1) {
                cursor.visible = 0;
            } else {
                cursor.visible = 1;
            }

            renderScreen(screen, editor, &cursor);

            timeBlink = getTimeMs();
        }
        
        consoleSleep(10);
    }


    freeScreen(screen);
    freeEditor(editor);
    freeFileBuffer(filebuffer);
}

int main(int argc, char** argv) {
    
    if(argc != 2) {
        printf("Please Enter a filename");
        return -1;
    }

    initTerminal();
    
    editorMain(argv[1]);

    cleanUpTerminal();

    return 0;
} 

#include "ui.h"
#include "editor.h"
#include "ui_constants.h"

#include <ncurses.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "ui_constants.h"

char status_message[STATUS_MESSAGE_MAX_LEN];
time_t status_message_time;

void editor_draw_rows()
{
    EditorConfig *E = get_editor_config();
    int y;
    for (y = 0; y < E->screen_rows; y++)
    {
        int filerow = y + E->row_offset;

        if (filerow >= E->lines.size)
        {
        }
        else
        {
            EditorLine *line = &E->lines.elements[filerow];
            int current_color_pair = HL_NORMAL;
            int display_col = 0;

            for (size_t i = 0; i < line->len; i++)
            {
                int char_display_width = 1;
                if (line->text[i] == '\t')
                {
                    char_display_width = TAB_STOP - (display_col % TAB_STOP);
                }

                if (display_col < E->col_offset)
                {
                    display_col += char_display_width;
                    continue;
                }

                if ((display_col - E->col_offset) >= E->screen_cols)
                    break;

                if (E_syntax && has_colors())
                {
                    int hl_type = line->hl[i];
                    if (hl_type != current_color_pair)
                    {
                        attroff(COLOR_PAIR(current_color_pair));
                        current_color_pair = hl_type;
                        attron(COLOR_PAIR(current_color_pair));
                    }
                }

                if (line->text[i] == '\t')
                {
                    for (int k = 0; k < char_display_width; k++)
                    {
                        mvaddch(y, (display_col - E->col_offset) + k, ' ');
                    }
                }
                else
                {
                    mvaddch(y, (display_col - E->col_offset), line->text[i]);
                }
                display_col += char_display_width;
            }
            if (E_syntax && has_colors())
            {
                attroff(COLOR_PAIR(current_color_pair));
            }
        }
        clrtoeol();
    }
}

void editor_draw_status_bar()
{
    EditorConfig *E = get_editor_config();
    attron(A_REVERSE);

    mvprintw(E->screen_rows, 0, "%.20s - %d lines %s", E->filename ? E->filename : "[No Name]",
             E->lines.size, E->dirty ? "(modified)" : "");

    char rstatus[80];
    snprintf(rstatus, sizeof(rstatus), "%d/%d", E->cy + 1, E->lines.size);
    mvprintw(E->screen_rows, E->screen_cols - strlen(rstatus), "%s", rstatus);

    attroff(A_REVERSE);
}

void editor_set_status_message(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(status_message, sizeof(status_message), fmt, ap);
    va_end(ap);
    status_message_time = time(NULL);
}

void editor_draw_message_bar()
{
    EditorConfig *E = get_editor_config();
    move(E->screen_rows + 1, 0);
    clrtoeol();

    int msglen = strlen(status_message);
    if (msglen > E->screen_cols)
        msglen = E->screen_cols;
    if (time(NULL) - status_message_time < STATUS_MESSAGE_TIMEOUT_SECONDS)
    {
        mvprintw(E->screen_rows + 1, 0, "%.*s", msglen, status_message);
    }
}

void editor_draw_clock()
{
    EditorConfig *E = get_editor_config();
    time_t rawtime;
    struct tm *info;
    char time_str[6];

    time(&rawtime);
    info = localtime(&rawtime);
    strftime(time_str, sizeof(time_str), "%H:%M", info);

    int clock_len = strlen(time_str);
    if (E->screen_cols >= clock_len)
    {
        mvprintw(0, E->screen_cols - clock_len, "%s", time_str);
    }
}

void editor_scroll()
{
    EditorConfig *E = get_editor_config();
    if (E->cy < E->row_offset)
    {
        E->row_offset = E->cy;
    }
    if (E->cy >= E->row_offset + E->screen_rows)
    {
        E->row_offset = E->cy - E->screen_rows + 1;
    }

    int current_line_len = (E->cy < E->lines.size) ? E->lines.elements[E->cy].len : 0;
    if (E->cx > current_line_len)
    {
        E->cx = current_line_len;
    }

    int current_cx_display = get_cx_display();

    if (current_cx_display < E->col_offset)
    {
        E->col_offset = current_cx_display;
    }
    if (current_cx_display >= E->col_offset + E->screen_cols)
    {
        E->col_offset = current_cx_display - E->screen_cols + 1;
    }
}

void editor_refresh_screen()
{
    EditorConfig *E = get_editor_config();
    editor_scroll();

    clear();

    editor_draw_rows();
    editor_draw_status_bar();
    editor_draw_message_bar();
    editor_draw_clock();

    move(E->cy - E->row_offset, get_cx_display() - E->col_offset);
    refresh();
}

int get_cx_display()
{
    EditorConfig *E = get_editor_config();
    int display_cx = 0;
    if (E->cy >= E->lines.size)
        return 0;

    EditorLine *line = &E->lines.elements[E->cy];
    for (int i = 0; i < E->cx; i++)
    {
        if ((size_t)i >= line->len)
            break;
        if (line->text[i] == '\t')
        {
            display_cx += (TAB_STOP - (display_cx % TAB_STOP));
        }
        else
        {
            display_cx++;
        }
    }
    return display_cx;
}

char *editor_prompt(const char *prompt_fmt, ...)
{
    size_t capacity = 256;
    char *buffer = malloc(capacity);
    int buflen = 0;

    if (buffer == NULL)
    {
        return NULL;
    }
    buffer[0] = '\0';

    while (1)
    {
        editor_set_status_message(prompt_fmt, buffer);
        editor_refresh_screen();

        int c = getch();
        if (c == '\r' || c == '\n')
        {
            if (buflen > 0)
            {
                /* Caller frees; return the growable buffer directly. */
                return buffer;
            }
            free(buffer);
            editor_set_status_message("");
            return NULL;
        }
        else if (c == CTRL('c') || c == CTRL('q') || c == 27)
        {
            free(buffer);
            editor_set_status_message("");
            return NULL;
        }
        else if (c == KEY_BACKSPACE || c == 127 || c == KEY_DC)
        {
            if (buflen > 0)
            {
                buflen--;
                buffer[buflen] = '\0';
            }
        }
        else if (c >= 32 && c <= 126)
        {
            if ((size_t)buflen + 1 >= capacity)
            {
                size_t new_capacity = capacity * 2;
                char *grown = realloc(buffer, new_capacity);
                if (grown == NULL)
                {
                    free(buffer);
                    editor_set_status_message("");
                    return NULL;
                }
                buffer = grown;
                capacity = new_capacity;
            }
            buffer[buflen++] = (char)c;
            buffer[buflen] = '\0';
        }
    }
}

void handle_winch(int sig)
{
    EditorConfig *E = get_editor_config();
    (void)sig;
    endwin();
    refresh();
    getmaxyx(stdscr, E->screen_rows, E->screen_cols);
    E->screen_rows -= 2;
    editor_refresh_screen();
}
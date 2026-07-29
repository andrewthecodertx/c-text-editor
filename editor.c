#include "editor.h"
#include "editor_lines_array.h"
#include "error_handler.h"
#include "file.h"
#include "syntax.h"
#include "ui.h"

#include <ncurses.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

void handle_winch(int sig);

extern time_t status_message_time;

static EditorConfig E;

EditorConfig* get_editor_config()
{
    return &E;
}

void init_editor()
{
    E.cx = 0;
    E.cy = 0;
    init_editor_lines_array(&E.lines);
    E.row_offset = 0;
    E.col_offset = 0;
    E.filename = NULL;
    E.dirty = 0;
    E.select_all_active = 0;

    E.select_active = 0;
    E.sel_start_row = 0;
    E.sel_start_col = 0;

    E.undo_history_len = 0;
    E.undo_history_idx = 0;
    for (int i = 0; i < MAX_UNDO_STATES; ++i)
    {
        // Initialize each EditorAction in the history to a default state if
        // necessary For now, we'll just ensure its internal pointers are NULL or
        // safe.
        E.undo_history[i].line_content = NULL; // Ensure this is NULL initially
    }

    E.search_query = NULL;
    E.search_direction = 1;
    E.last_match_row = -1;
    E.last_match_col = -1;
    E.find_active = false;
    E.recording_actions = true;
    E.critical_error = 0;

    initscr();
    raw();
    noecho();
    keypad(stdscr, TRUE);

    signal(SIGWINCH, handle_winch);

    getmaxyx(stdscr, E.screen_rows, E.screen_cols);
    E.screen_rows -= 2;

    if (has_colors())
    {
        start_color();
        use_default_colors();
        init_pair(HL_NORMAL, COLOR_WHITE, COLOR_BLACK);
        init_pair(HL_COMMENT, COLOR_CYAN, COLOR_BLACK);
        init_pair(HL_KEYWORD1, COLOR_YELLOW, COLOR_BLACK);
        init_pair(HL_KEYWORD2, COLOR_GREEN, COLOR_BLACK);
        init_pair(HL_STRING, COLOR_MAGENTA, COLOR_BLACK);
        init_pair(HL_NUMBER, COLOR_RED, COLOR_BLACK);
        init_pair(HL_MATCH, COLOR_BLACK, COLOR_YELLOW);
        init_pair(HL_PREPROC, COLOR_BLUE, COLOR_BLACK);
    }

    mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, NULL);
}

void cleanup_editor()
{
    endwin();

    free_editor_lines_array(&E.lines);
    if (E.filename)
    {
        free(E.filename);
    }
    if (E.search_query)
    {
        free(E.search_query);
    }

    for (int i = 0; i < E.undo_history_len; ++i)
    {
        editor_action_free(&E.undo_history[i]);
    }
}

static void editor_clear_selection()
{
    E.select_active = 0;
    E.sel_start_row = 0;
    E.sel_start_col = 0;
}

void editor_move_cursor(int key)
{
    EditorLine* line = (E.cy >= E.lines.size) ? NULL : &E.lines.elements[E.cy];
    int is_selection_text =
        (key == KEY_SLEFT || key == KEY_SRIGHT || key == KEY_SF || key == KEY_SR);

    /* text selection logic */
    if (is_selection_text)
    {
        if (!E.select_active)
        {
            E.select_active = 1;
            E.sel_start_row = E.cy;
            E.sel_start_col = E.cx;
        }

        switch (key)
        {
        case KEY_SRIGHT:
            key = KEY_RIGHT;
            break;
        case KEY_SLEFT:
            key = KEY_LEFT;
            break;
        case KEY_SR:
            key = KEY_UP;
            break;
        case KEY_SF:
            key = KEY_DOWN;
            break;
        }
    }
    else
    {
        editor_clear_selection();
    }
    /* text selection logic */

    switch (key)
    {

    case KEY_LEFT:
        if (E.cx > 0)
        {
            E.cx--;
        }
        else if (E.cy > 0)
        {
            E.cy--;
            E.cx = E.lines.elements[E.cy].len;
        }
        break;
    case KEY_RIGHT:
        if (line && (size_t) E.cx < line->len)
        {
            E.cx++;
        }
        else if (line && (size_t) E.cx == line->len && E.cy < E.lines.size - 1)
        {
            if (line && (size_t) E.cx == line->len && E.cy < E.lines.size - 1)
            {
                E.cy++;
                E.cx = 0;
            }
        }
        break;
    case KEY_UP:
        if (E.cy > 0)
        {
            E.cy--;
        }
        break;
    case KEY_DOWN:
        if (E.cy < E.lines.size - 1)
        {
            E.cy++;
        }
        break;
    case KEY_HOME:
        E.cx = 0;
        break;
    case KEY_END:
        if (line)
            E.cx = line->len;
        break;
    case KEY_PPAGE:
    case KEY_NPAGE:
    {
        int times = E.screen_rows;
        while (times--)
        {
            if (key == KEY_PPAGE)
            {
                if (E.cy > 0)
                    E.cy--;
            }
            else
            {
                if (E.cy < E.lines.size - 1)
                    E.cy++;
            }
        }
    }
    break;
    }
    line = (E.cy >= E.lines.size) ? NULL : &E.lines.elements[E.cy];
    int line_len = line ? line->len : 0;
    if (E.cx > line_len)
    {
        E.cx = line_len;
    }
}

EditorSelectionRange editor_resolve_selection()
{
    EditorSelectionRange range;

    if (E.sel_start_row < E.cy || (E.sel_start_row == E.cy && E.sel_start_col <= E.cx))
    {
        range.start_row = E.sel_start_row;
        range.start_col = E.sel_start_col;
        range.end_row = E.cy;
        range.end_col = E.cx;
    }
    else
    {
        range.start_row = E.cy;
        range.start_col = E.cx;
        range.end_row = E.sel_start_row;
        range.end_col = E.sel_start_col;
    }

    return range;
}

void editor_process_keypress()
{
    int c = getch();
    bool cursor_moved = false;
    int original_cx = E.cx;
    int original_cy = E.cy;

    if (E.critical_error)
    {
        if (c == CTRL('s'))
        {
            editor_save_file();
        }
        else if (c == CTRL('q') || c == CTRL('c'))
        {
            cleanup_editor();
            exit(0);
        }
        editor_refresh_screen();
        return;
    }

    if (E.find_active && c != KEY_UP && c != KEY_DOWN && c != CTRL('f'))
    {
        E.find_active = false;
        editor_set_status_message("");
        for (int i = 0; i < E.lines.size; i++)
        {
            editor_update_syntax(i);
        }
        editor_refresh_screen();
    }

    if (E.select_all_active && c != KEY_BACKSPACE && c != 127 && c != KEY_DC)
    {
        E.select_all_active = 0;
        editor_set_status_message("");
    }

    if (E.select_active && c != KEY_SLEFT && c != KEY_SRIGHT && c != KEY_SF && c != KEY_SR &&
        c != KEY_LEFT && c != KEY_RIGHT && c != KEY_UP && c != KEY_DOWN && c != KEY_HOME &&
        c != KEY_END && c != KEY_PPAGE && c != KEY_NPAGE)
    {
        editor_clear_selection();
    }

    switch (c)
    {
    case CTRL('q'):
    case CTRL('c'):
        if (E.dirty)
        {
            editor_set_status_message("WARNING! File has unsaved changes. Press "
                                      "Ctrl+Q/C again to force quit.");
            editor_refresh_screen();
            int c2 = getch();
            if (c2 != CTRL('q') && c2 != CTRL('c'))
                return;
        }
        cleanup_editor();
        exit(0);
        break;

    case CTRL('s'):
        editor_save_file();
        break;

    case CTRL('a'):
        E.select_all_active = 1;
        E.cx = 0;
        E.cy = 0;
        editor_set_status_message("All text selected. Press Backspace to delete.");
        cursor_moved = true;
        break;

    case CTRL('v'):
        paste_from_clipboard();
        break;

    case CTRL('z'):
        editor_undo();
        break;

    case CTRL('f'):
        editor_find();
        break;

    case KEY_BACKSPACE:
    case KEY_DC:
    case 127:
        editor_del_char();
        break;

    case '\t':
        editor_insert_char('\t');
        break;

    case '\r':
    case '\n':
        editor_insert_newline();
        break;

    case KEY_SLEFT:
    case KEY_SRIGHT:
    case KEY_SF:
    case KEY_SR:
    case KEY_HOME:
    case KEY_END:
    case KEY_PPAGE:
    case KEY_NPAGE:
        editor_move_cursor(c);
        cursor_moved = true;
        break;

    case KEY_UP:
        if (E.find_active)
        {
            editor_find_next(-1);
        }
        else
        {
            editor_move_cursor(c);
            cursor_moved = true;
        }
        break;
    case KEY_DOWN:
        if (E.find_active)
        {
            editor_find_next(1);
        }
        else
        {
            editor_move_cursor(c);
            cursor_moved = true;
        }
        break;
    case KEY_LEFT:
    case KEY_RIGHT:
        editor_move_cursor(c);
        cursor_moved = true;
        break;

    case KEY_MOUSE:
    {
        MEVENT event;
        if (getmouse(&event) == OK)
        {
            if (event.bstate & BUTTON1_CLICKED)
            {
                E.cy = event.y + E.row_offset;

                int target_display_cx = event.x + E.col_offset;
                int actual_cx = 0;
                if (E.cy < E.lines.size)
                {
                    EditorLine* line = &E.lines.elements[E.cy];
                    int current_display_cx = 0;
                    for (size_t char_idx = 0; char_idx < line->len; char_idx++)
                    {
                        int char_display_width = 1;
                        if (line->text[char_idx] == '\t')
                        {
                            char_display_width = 4 - (current_display_cx % 4);
                        }
                        if (current_display_cx + char_display_width > target_display_cx)
                        {
                            break;
                        }
                        current_display_cx += char_display_width;
                        actual_cx = char_idx + 1;
                    }
                }
                E.cx = actual_cx;

                if (E.cy >= E.lines.size)
                {
                    E.cy = E.lines.size > 0 ? E.lines.size - 1 : 0;
                }
                EditorLine* line = (E.cy < E.lines.size) ? &E.lines.elements[E.cy] : NULL;
                int line_len = line ? line->len : 0;
                if (E.cx > line_len)
                {
                    E.cx = line_len;
                }
                cursor_moved = true;
            }
            else if (event.bstate & BUTTON4_PRESSED)
            {
                for (int i = 0; i < 3; ++i)
                {
                    editor_move_cursor(KEY_UP);
                }
                cursor_moved = true;
            }
            else if (event.bstate & BUTTON5_PRESSED)
            {
                for (int i = 0; i < 3; ++i)
                {
                    editor_move_cursor(KEY_DOWN);
                }
                cursor_moved = true;
            }
        }
    }
    break;
    default:
        if (c >= 32 && c <= 126)
        {
            editor_insert_char(c);
        }
        break;
    }

    if (E.dirty || cursor_moved || original_cx != E.cx || original_cy != E.cy ||
        time(NULL) - status_message_time < 5)
    {
        editor_refresh_screen();
    }
}

void editor_insert_char(int c)
{
    EditorAction action = {
        .type = ACTION_INSERT_CHAR, .row = E.cy, .col = E.cx, .character = (char) c};
    editor_record_action(action);
    if (E.cy == E.lines.size)
    {
        EditorLine new_line = {.text = strdup(""), .len = 0, .hl = NULL, .hl_open_comment = 0};
        if (new_line.text == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY,
                                "Failed to prepare new line for character insertion.");
            return;
        }
        editor_lines_array_append(&E.lines, new_line);
    }

    EditorLine* line = &E.lines.elements[E.cy];
    line->text = realloc(line->text, line->len + 2);
    if (line->text == NULL)
    {
        editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory for line %d.", E.cy);
        return;
    }
    memmove(&line->text[E.cx + 1], &line->text[E.cx], line->len - E.cx + 1);
    line->text[E.cx] = c;
    line->len++;
    E.cx++;
    E.dirty = 1;

    editor_update_syntax(E.cy);
}

int editor_insert_newline()
{
    EditorAction action = {.type = ACTION_INSERT_NEWLINE, .row = E.cy, .col = E.cx};
    editor_record_action(action);
    if (E.lines.size == 0)
    {
        EditorLine new_line = {.text = strdup(""), .len = 0, .hl = NULL, .hl_open_comment = 0};
        if (new_line.text == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory for initial line text.");
            return -1;
        }
        editor_lines_array_append(&E.lines, new_line);
        E.cy = 0;
        E.cx = 0;
        E.dirty = 1;
        editor_update_syntax(0);
        return 0;
    }

    EditorLine new_line = {.text = NULL, .len = 0, .hl = NULL, .hl_open_comment = 0};
    editor_lines_array_insert(&E.lines, E.cy + 1, new_line);

    E.lines.elements[E.cy].hl = NULL;
    E.lines.elements[E.cy].hl_open_comment = 0;

    if (E.cx == 0)
    {
        E.lines.elements[E.cy].text = strdup("");
        if (E.lines.elements[E.cy].text == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory for new empty line text.");
            return -1;
        }
        E.lines.elements[E.cy].len = 0;
    }
    else
    {
        EditorLine* current_line = &E.lines.elements[E.cy];
        E.lines.elements[E.cy + 1].len = current_line->len - E.cx;
        E.lines.elements[E.cy + 1].text = strdup(&current_line->text[E.cx]);
        if (E.lines.elements[E.cy + 1].text == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory for split line text.");
            return -1;
        }
        E.lines.elements[E.cy + 1].hl = NULL;
        E.lines.elements[E.cy + 1].hl_open_comment = 0;

        current_line->text = realloc(current_line->text, E.cx + 1);
        if (current_line->text == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory for truncated line.");
            return -1;
        }
        current_line->text[E.cx] = '\0';
        current_line->len = E.cx;
    }

    E.cy++;
    E.cx = 0;
    E.dirty = 1;

    editor_update_syntax(E.cy - 1);
    editor_update_syntax(E.cy);

    return 0;
}

void editor_del_char()
{
    EditorAction action = {.type = ACTION_DELETE_CHAR, .row = E.cy, .col = E.cx};
    if (E.cx > 0)
    {
        action.character = E.lines.elements[E.cy].text[E.cx - 1];
    }
    else
    {
        action.type = ACTION_DELETE_LINE;
        action.line_content = strdup(E.lines.elements[E.cy].text);
        action.line_len = E.lines.elements[E.cy].len;
    }
    editor_record_action(action);
    if (E.select_all_active)
    {
        free_editor_lines_array(&E.lines);
        init_editor_lines_array(&E.lines);
        EditorLine new_line = {.text = strdup(""), .len = 0, .hl = NULL, .hl_open_comment = 0};
        if (new_line.text == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory (clear all text).");
        }
        editor_lines_array_append(&E.lines, new_line);
        E.cx = 0;
        E.cy = 0;
        E.dirty = 1;
        E.select_all_active = 0;
        editor_update_syntax(0);
        editor_set_status_message("All text deleted.");
        return;
    }

    if (E.cy == E.lines.size || E.lines.size == 0)
        return;
    if (E.cx == 0 && E.cy == 0 && E.lines.elements[0].len == 0)
        return;

    EditorLine* line = &E.lines.elements[E.cy];
    if (E.cx > 0)
    {
        memmove(&line->text[E.cx - 1], &line->text[E.cx], line->len - E.cx + 1);
        line->len--;
        line->text = realloc(line->text, line->len + 1);
        if (line->text == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory (del char realloc).");
            return;
        }
        E.cx--;
        E.dirty = 1;
        editor_update_syntax(E.cy);
    }
    else
    {
        if (E.cy > 0)
        {
            EditorLine* prev_line = &E.lines.elements[E.cy - 1];
            prev_line->text = realloc(prev_line->text, prev_line->len + line->len + 1);
            if (prev_line->text == NULL)
            {
                editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory (merge line realloc).");
                return;
            }
            memcpy(&prev_line->text[prev_line->len], line->text, line->len);
            prev_line->len += line->len;
            prev_line->text[prev_line->len] = '\0';

            editor_lines_array_delete(&E.lines, E.cy);

            if (E.lines.size == 0)
            {
                EditorLine new_line = {
                    .text = strdup(""), .len = 0, .hl = NULL, .hl_open_comment = 0};
                if (new_line.text == NULL)
                {
                    editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory (empty file text).");
                }
                editor_lines_array_append(&E.lines, new_line);
                E.cx = 0;
                E.cy = 0;
                editor_update_syntax(0);
            }
            else
            {
                E.cx = prev_line->len;
                E.cy--;
                editor_update_syntax(E.cy);
            }
            E.dirty = 1;
        }
    }
}

void editor_undo()
{
    if (E.undo_history_idx <= 0)
    {
        editor_set_status_message("Nothing to undo.");
        return;
    }

    E.undo_history_idx--;
    EditorAction last_action = E.undo_history[E.undo_history_idx];

    E.recording_actions = false; // Temporarily disable recording

    switch (last_action.type)
    {
    case ACTION_INSERT_CHAR:
        // Undo insert char: delete char at recorded position
        // Need to adjust cursor to recorded position first
        E.cy = last_action.row;
        E.cx = last_action.col;
        // Perform the deletion without recording it
        EditorLine* line_to_delete_from = &E.lines.elements[E.cy];
        memmove(&line_to_delete_from->text[E.cx], &line_to_delete_from->text[E.cx + 1],
                line_to_delete_from->len - E.cx);
        line_to_delete_from->len--;
        line_to_delete_from->text =
            realloc(line_to_delete_from->text, line_to_delete_from->len + 1);
        E.dirty = 1;
        editor_update_syntax(E.cy);
        break;
    case ACTION_DELETE_CHAR:
        // Undo delete char: insert char at recorded position
        // Need to adjust cursor to recorded position first
        E.cy = last_action.row;
        E.cx = last_action.col;
        // Perform the insertion without recording it
        EditorLine* line_to_insert_into = &E.lines.elements[E.cy];
        line_to_insert_into->text =
            realloc(line_to_insert_into->text, line_to_insert_into->len + 2);
        memmove(&line_to_insert_into->text[E.cx + 1], &line_to_insert_into->text[E.cx],
                line_to_insert_into->len - E.cx + 1);
        line_to_insert_into->text[E.cx] = last_action.character;
        line_to_insert_into->len++;
        E.dirty = 1;
        editor_update_syntax(E.cy);
        break;
    case ACTION_INSERT_NEWLINE:
        // Undo insert newline: delete the newline at the recorded position
        E.cy = last_action.row;
        E.cx = last_action.col;
        // Perform the deletion without recording it
        if (E.cy < E.lines.size - 1)
        { // If not the last line
            EditorLine* current_line = &E.lines.elements[E.cy];
            EditorLine* next_line = &E.lines.elements[E.cy + 1];

            current_line->text =
                realloc(current_line->text, current_line->len + next_line->len + 1);
            memcpy(&current_line->text[current_line->len], next_line->text, next_line->len);
            current_line->len += next_line->len;
            current_line->text[current_line->len] = '\0';

            editor_lines_array_delete(&E.lines, E.cy + 1);
            E.dirty = 1;
            editor_update_syntax(E.cy);
        }
        break;
    case ACTION_DELETE_LINE:
        // Undo delete line: insert line with recorded content
        {
            EditorLine new_line = {.text = last_action.line_content,
                                   .len = last_action.line_len,
                                   .hl = NULL,
                                   .hl_open_comment = 0};
            editor_lines_array_insert(&E.lines, last_action.row, new_line);
            // Transfer ownership: clear the action's line_content to avoid double-free
            E.undo_history[E.undo_history_idx].line_content = NULL;
            E.cy = last_action.row;
            E.cx = last_action.col;
            E.dirty = 1;
            editor_update_syntax(E.cy);
        }
        break;
    default:
        editor_set_status_message("Undo: Unknown action type.");
        break;
    }

    editor_set_status_message("Undo successful.");
    editor_refresh_screen();

    E.recording_actions = true; // Re-enable recording
}

void editor_find()
{
    char* query = editor_prompt("Search (Use arrows to navigate, ESC to cancel): %s",
                                E.search_query ? E.search_query : "");

    if (query == NULL)
    {
        editor_set_status_message("");
        E.find_active = false;
        for (int i = 0; i < E.lines.size; i++)
        {
            editor_update_syntax(i);
        }
        editor_refresh_screen();
        return;
    }

    if (E.search_query)
    {
        if (strcmp(E.search_query, query) != 0)
        {
            free(E.search_query);
            E.search_query = query;
            E.last_match_row = -1;
            E.last_match_col = -1;
        }
        else
        {
            free(query);
        }
    }
    else
    {
        E.search_query = query;
        E.last_match_row = -1;
        E.last_match_col = -1;
    }

    E.find_active = true;
    editor_find_next(1);
}

void editor_find_next(int direction)
{
    if (E.search_query == NULL)
        return;

    int current_row = E.last_match_row;
    int current_col = E.last_match_col;

    if (current_row == -1)
    {
        current_row = E.cy;
        current_col = E.cx;
        E.search_direction = direction;
    }
    else
    {
        current_col += direction;
    }

    int query_len = strlen(E.search_query);
    int original_row = current_row;
    int original_col = current_col;

    while (1)
    {
        if (current_row < 0 || current_row >= E.lines.size)
            break;

        EditorLine* line = &E.lines.elements[current_row];
        char* match = NULL;

        if (direction == 1)
        {
            if ((size_t) current_col >= line->len)
            {
                current_row++;
                current_col = 0;
                continue;
            }
            match = strstr(line->text + current_col, E.search_query);
        }
        else
        {
            if (current_col < 0)
            {
                current_row--;
                if (current_row < 0)
                    break;
                current_col = E.lines.elements[current_row].len - 1;
                for (int i = current_col; i >= 0; i--)
                {
                    if ((size_t) i + query_len <= line->len &&
                        strncmp(line->text + i, E.search_query, query_len) == 0)
                    {
                        match = line->text + i;
                        break;
                    }
                }
            }
        }

        if (match)
        {
            E.cy = current_row;
            E.cx = match - line->text;
            E.last_match_row = E.cy;
            E.last_match_col = E.cx;
            editor_set_status_message("Found '%s' at %d:%d", E.search_query, E.cy + 1, E.cx + 1);
            editor_refresh_screen();
            return;
        }

        if (direction == 1)
        {
            current_row++;
            current_col = 0;
        }
        else
        {
            current_row--;
            current_col = E.lines.elements[current_row].len - 1;
        }

        if (current_row >= E.lines.size)
        {
            current_row = 0;
            current_col = 0;
        }
        else if (current_row < 0)
        {
            current_row = E.lines.size - 1;
            current_col = E.lines.elements[current_row].len - 1;
        }

        if (current_row == original_row && current_col == original_col)
        {
            break;
        }
    }
    editor_set_status_message("No more matches for '%s'", E.search_query);
    E.last_match_row = -1;
    E.last_match_col = -1;
    editor_refresh_screen();
}

void paste_from_clipboard()
{
    int pipefd[2];
    pid_t pid;
    char buffer[1024];
    ssize_t bytes_read;
    char* clipboard_tool = NULL;
    char* argv[3];
    int tool_found = 0;

    if (getenv("XDG_SESSION_TYPE") != NULL && strcmp(getenv("XDG_SESSION_TYPE"), "wayland") == 0)
    {
        if (access("/usr/bin/wl-paste", X_OK) == 0 || access("/bin/wl-paste", X_OK) == 0)
        {
            clipboard_tool = "wl-paste";
            argv[0] = "wl-paste";
            argv[1] = NULL;
            tool_found = 1;
        }
    }

    if (!tool_found)
    {
        if (access("/usr/bin/xclip", X_OK) == 0 || access("/bin/xclip", X_OK) == 0)
        {
            clipboard_tool = "xclip";
            argv[0] = "xclip";
            argv[1] = "-o";
            argv[2] = NULL;
            tool_found = 1;
        }
    }

    if (!tool_found)
    {
        editor_handle_error(ERR_CLIPBOARD_TOOL,
                            "Paste error: Neither wl-paste nor xclip found. Please install one.");
        return;
    }

    editor_set_status_message("Attempting to paste using %s...", clipboard_tool);
    editor_refresh_screen();

    fflush(stdout);

    if (pipe(pipefd) == -1)
    {
        editor_handle_error(ERR_CLIPBOARD_TOOL, "Paste error: Failed to create pipe.");
        return;
    }

    pid = fork();
    if (pid == -1)
    {
        editor_handle_error(ERR_CLIPBOARD_TOOL, "Paste error: Failed to fork process.");
        close(pipefd[0]);
        close(pipefd[1]);
        return;
    }

    if (pid == 0)
    {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execvp(clipboard_tool, argv);

        _exit(1);
    }
    else
    {
        close(pipefd[1]);

        while ((bytes_read = read(pipefd[0], buffer, sizeof(buffer) - 1)) > 0)
        {
            buffer[bytes_read] = '\0';
            for (int i = 0; i < bytes_read; ++i)
            {
                if (buffer[i] == '\n' || buffer[i] == '\r')
                {
                    editor_insert_newline();
                }
                else if (buffer[i] >= 32 && buffer[i] <= 126)
                {
                    editor_insert_char(buffer[i]);
                }
            }
        }
        close(pipefd[0]);

        int status;
        waitpid(pid, &status, 0);

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        {
            editor_set_status_message("Pasted from clipboard using %s.", clipboard_tool);
        }
        else
        {
            editor_set_status_message("Paste error: %s failed or returned an error.",
                                      clipboard_tool);
        }
    }
}

void editor_record_action(EditorAction action)
{
    if (!E.recording_actions)
    {
        return;
    }
    if (E.undo_history_idx < E.undo_history_len)
    {
        for (int i = E.undo_history_idx; i < E.undo_history_len; ++i)
        {
            editor_action_free(&E.undo_history[i]);
        }
        E.undo_history_len = E.undo_history_idx;
    }

    if (E.undo_history_len == MAX_UNDO_STATES)
    {
        editor_action_free(&E.undo_history[0]);
        memmove(&E.undo_history[0], &E.undo_history[1],
                (MAX_UNDO_STATES - 1) * sizeof(EditorAction));
        E.undo_history_len--;
        E.undo_history_idx--;
    }

    E.undo_history[E.undo_history_idx] = action;
    E.undo_history_len++;
    E.undo_history_idx++;
}

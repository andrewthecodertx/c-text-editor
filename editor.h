#ifndef EDITOR_H
#define EDITOR_H

#include "editor_lines_array.h"
#include "syntax.h"
#include <ncurses.h>
#include <stdbool.h>
#include <stddef.h>

#include "editor_actions.h"

#define MAX_UNDO_STATES 1000
#define CTRL(k) ((k) & 0x1f)

typedef struct
{
    EditorLinesArray lines;
    int cx, cy;
    int dirty;
} EditorStateSnapshot;

typedef struct
{
    int start_row;
    int start_col;
    int end_row;
    int end_col;
} EditorSelectionRange;

typedef struct
{
    EditorLinesArray lines;
    int cx, cy;
    int row_offset;
    int col_offset;
    int screen_rows, screen_cols;
    char* filename;
    int dirty;
    int select_all_active;

    int select_active;
    int sel_start_row;
    int sel_start_col;

    EditorAction undo_history[MAX_UNDO_STATES];
    int undo_history_len;
    int undo_history_idx;

    char* search_query;
    int search_direction; // 1 for forward, -1 for backward
    int last_match_row;
    int last_match_col;
    bool find_active;
    bool recording_actions;
    int critical_error;
} EditorConfig;

EditorConfig* get_editor_config();
extern EditorSyntax* E_syntax;

void init_editor();
void cleanup_editor();
void editor_move_cursor(int key);
EditorSelectionRange editor_resolve_selection();
void editor_process_keypress();
void editor_insert_char(int c);
int editor_insert_newline();
void editor_del_char();
void editor_undo();
void editor_find();
void editor_find_next(int direction);
void paste_from_clipboard();
void editor_record_action(EditorAction action);
void editor_free_snapshot(EditorStateSnapshot* snapshot);

#endif // EDITOR_H

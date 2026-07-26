#include "editor_actions.h"

#include "editor.h"
#include "editor_lines_array.h"
#include "syntax.h"
#include "ui.h"

#include <stdlib.h>
#include <string.h>

// Free heap memory owned by an action
void editor_action_free(EditorAction *action) {
  if (action->type == ACTION_DELETE_LINE && action->line_content) {
    free(action->line_content);
    action->line_content = NULL;
  }
}

void editor_record_action(EditorAction action) {
  EditorConfig *E = get_editor_config();

  if (!E->recording_actions) {
    return;
  }
  if (E->undo_history_idx < E->undo_history_len) {
    for (int i = E->undo_history_idx; i < E->undo_history_len; ++i) {
      editor_action_free(&E->undo_history[i]); // Free any memory associated with discarded actions
    }
    E->undo_history_len = E->undo_history_idx;
  }

  if (E->undo_history_len == MAX_UNDO_STATES) {
    editor_action_free(&E->undo_history[0]); // Free the oldest action before shifting
    memmove(&E->undo_history[0], &E->undo_history[1],
            (MAX_UNDO_STATES - 1) * sizeof(EditorAction));
    E->undo_history_len--;
    E->undo_history_idx--;
  }

  E->undo_history[E->undo_history_idx] = action;
  E->undo_history_len++;
  E->undo_history_idx++;
}

void editor_undo() {
  EditorConfig *E = get_editor_config();

  if (E->undo_history_idx <= 0) {
    editor_set_status_message("Nothing to undo.");
    return;
  }

  E->undo_history_idx--;
  EditorAction last_action = E->undo_history[E->undo_history_idx];

  E->recording_actions = false; // Temporarily disable recording

  switch (last_action.type) {
  case ACTION_INSERT_CHAR:
    // Undo insert char: delete char at recorded position
    E->cy = last_action.row;
    E->cx = last_action.col;
    EditorLine *line_to_delete_from = &E->lines.elements[E->cy];
    memmove(&line_to_delete_from->text[E->cx],
            &line_to_delete_from->text[E->cx + 1],
            line_to_delete_from->len - E->cx);
    line_to_delete_from->len--;
    line_to_delete_from->text =
        realloc(line_to_delete_from->text, line_to_delete_from->len + 1);
    E->dirty = 1;
    editor_update_syntax(E->cy);
    break;
  case ACTION_DELETE_CHAR:
    // Undo delete char: insert char at recorded position
    E->cy = last_action.row;
    E->cx = last_action.col;
    EditorLine *line_to_insert_into = &E->lines.elements[E->cy];
    line_to_insert_into->text =
        realloc(line_to_insert_into->text, line_to_insert_into->len + 2);
    memmove(&line_to_insert_into->text[E->cx + 1],
            &line_to_insert_into->text[E->cx],
            line_to_insert_into->len - E->cx + 1);
    line_to_insert_into->text[E->cx] = last_action.character;
    line_to_insert_into->len++;
    E->dirty = 1;
    editor_update_syntax(E->cy);
    break;
  case ACTION_INSERT_NEWLINE:
    // Undo insert newline: delete the newline at the recorded position
    E->cy = last_action.row;
    E->cx = last_action.col;
    if (E->cy < E->lines.size - 1) { // If not the last line
      EditorLine *current_line = &E->lines.elements[E->cy];
      EditorLine *next_line = &E->lines.elements[E->cy + 1];

      current_line->text =
          realloc(current_line->text, current_line->len + next_line->len + 1);
      memcpy(&current_line->text[current_line->len], next_line->text,
             next_line->len);
      current_line->len += next_line->len;
      current_line->text[current_line->len] = '\0';

      editor_lines_array_delete(&E->lines, E->cy + 1);
      E->dirty = 1;
      editor_update_syntax(E->cy);
    }
    break;
  case ACTION_DELETE_LINE:
    // Undo delete line: insert line with recorded content
    {
      EditorLine new_line = {.text = last_action.line_content,
                             .len = last_action.line_len,
                             .hl = NULL,
                             .hl_open_comment = 0};
      editor_lines_array_insert(&E->lines, last_action.row, new_line);
      E->undo_history[E->undo_history_idx].line_content = NULL; // Clear to avoid double-free after ownership transfer
      E->cy = last_action.row;
      E->cx = last_action.col;
      E->dirty = 1;
      editor_update_syntax(E->cy);
    }
    break;
  default:
    editor_set_status_message("Undo: Unknown action type.");
    break;
  }

  editor_set_status_message("Undo successful.");
  editor_refresh_screen();

  E->recording_actions = true; // Re-enable recording
}

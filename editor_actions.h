#ifndef EDITOR_ACTIONS_H
#define EDITOR_ACTIONS_H

#include <stddef.h> // For size_t

// Enum for different types of editor actions
typedef enum {
  ACTION_INSERT_CHAR,
  ACTION_DELETE_CHAR,
  ACTION_INSERT_NEWLINE,
  ACTION_DELETE_LINE,
  // Add more action types as needed
} EditorActionType;

// Structure to represent a single editor action
typedef struct {
  EditorActionType type;
  int row;
  int col;
  char character;     // For insert/delete char
  char *line_content; // For delete line (stores content of deleted line)
  size_t line_len;    // For delete line (stores length of deleted line)
} EditorAction;

#endif // EDITOR_ACTIONS_H

// Action management functions
void editor_record_action(EditorAction action);
void editor_undo();
void editor_action_free(EditorAction *action);

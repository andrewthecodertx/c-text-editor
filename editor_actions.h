#ifndef EDITOR_ACTIONS_H
#define EDITOR_ACTIONS_H

#include <stddef.h> // For size_t

// Enum for different types of editor actions
typedef enum
{
    ACTION_INSERT_CHAR,
    ACTION_DELETE_CHAR,
    ACTION_INSERT_NEWLINE,
    ACTION_DELETE_LINE,
    ACTION_DELETE_RANGE,
    // Add more action types as needed
} EditorActionType;

// Structure to represent a single editor action
typedef struct
{
    EditorActionType type;
    int row;
    int col;
    char character;     // For insert/delete char
    char* line_content; // For delete line/range (stores content of deleted text;
                        // for ACTION_DELETE_RANGE, '\n' marks line breaks within the range)
    size_t line_len;    // For delete line/range (stores length of deleted text)
} EditorAction;

void editor_action_free(EditorAction* action);

#endif // EDITOR_ACTIONS_H

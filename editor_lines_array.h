#ifndef EDITOR_LINES_ARRAY_H
#define EDITOR_LINES_ARRAY_H

#include <stddef.h> // For size_t

typedef struct {
  char *text;
  size_t len;
  char *hl;
  int hl_open_comment;
} EditorLine;

typedef struct {
  EditorLine *elements;
  int size;
  int capacity;
} EditorLinesArray;

void init_editor_lines_array(EditorLinesArray *array);
void free_editor_lines_array(EditorLinesArray *array);
void editor_lines_array_append(EditorLinesArray *array, EditorLine line);
void editor_lines_array_insert(EditorLinesArray *array, int index,
                               EditorLine line);
void editor_lines_array_delete(EditorLinesArray *array, int index);

#endif // EDITOR_LINES_ARRAY_H

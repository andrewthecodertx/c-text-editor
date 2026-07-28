#include "editor_lines_array.h"
#include "error_handler.h"

#include <stdlib.h>
#include <string.h>

#define EDITOR_LINES_ARRAY_INIT_CAPACITY 8

void init_editor_lines_array(EditorLinesArray* array)
{
    array->elements = malloc(sizeof(EditorLine) * EDITOR_LINES_ARRAY_INIT_CAPACITY);
    if (array->elements == NULL)
    {
        editor_handle_error(ERR_OUT_OF_MEMORY,
                            "Failed to allocate initial memory for EditorLinesArray.");
    }
    array->size = 0;
    array->capacity = EDITOR_LINES_ARRAY_INIT_CAPACITY;
}

void free_editor_lines_array(EditorLinesArray* array)
{
    if (array->elements)
    {
        for (int i = 0; i < array->size; ++i)
        {
            free(array->elements[i].text);
            free(array->elements[i].hl);
        }
        free(array->elements);
        array->elements = NULL;
    }
    array->size = 0;
    array->capacity = 0;
}

void editor_lines_array_grow(EditorLinesArray* array)
{
    int new_capacity = array->capacity * 2;
    EditorLine* new_elements = realloc(array->elements, sizeof(EditorLine) * new_capacity);
    if (new_elements == NULL)
    {
        editor_handle_error(ERR_OUT_OF_MEMORY, "Failed to grow EditorLinesArray capacity.");
    }
    array->elements = new_elements;
    array->capacity = new_capacity;
}

void editor_lines_array_append(EditorLinesArray* array, EditorLine line)
{
    if (array->size == array->capacity)
    {
        editor_lines_array_grow(array);
    }
    array->elements[array->size++] = line;
}

void editor_lines_array_insert(EditorLinesArray* array, int index, EditorLine line)
{
    if (index < 0 || index > array->size)
    {
        editor_handle_error(ERR_NONE, "Invalid index for EditorLinesArray insertion.");
        return;
    }
    if (array->size == array->capacity)
    {
        editor_lines_array_grow(array);
    }
    memmove(&array->elements[index + 1], &array->elements[index],
            (array->size - index) * sizeof(EditorLine));
    array->elements[index] = line;
    array->size++;
}

void editor_lines_array_delete(EditorLinesArray* array, int index)
{
    if (index < 0 || index >= array->size)
    {
        editor_handle_error(ERR_NONE, "Invalid index for EditorLinesArray deletion.");
        return;
    }
    free(array->elements[index].text);
    free(array->elements[index].hl);
    memmove(&array->elements[index], &array->elements[index + 1],
            (array->size - index - 1) * sizeof(EditorLine));
    array->size--;

    // Shrink array if significantly underutilized
    if (array->capacity > EDITOR_LINES_ARRAY_INIT_CAPACITY && array->size < array->capacity / 4)
    {
        int new_capacity = array->capacity / 2;
        EditorLine* new_elements = realloc(array->elements, sizeof(EditorLine) * new_capacity);
        if (new_elements == NULL)
        {
            editor_handle_error(ERR_OUT_OF_MEMORY, "Failed to shrink EditorLinesArray capacity.");
        }
        array->elements = new_elements;
        array->capacity = new_capacity;
    }
}

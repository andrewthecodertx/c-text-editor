#include "file.h"
#include "editor.h"
#include "syntax.h"
#include "ui.h"

#include "editor_lines_array.h"
#include "error_handler.h"
#include <errno.h>
#include <ncurses.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

void editor_read_file(const char* filename)
{
    EditorConfig* E = get_editor_config();
    if (E->filename)
        free(E->filename);
    E->filename = strdup(filename);
    if (E->filename == NULL)
    {
        editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory (filename).");
    }

    editor_select_syntax_highlight();

    FILE* fp = fopen(filename, "r");
    if (!fp)
    {
        if (errno == ENOENT)
        {
            init_editor_lines_array(&E->lines);
            EditorLine new_line = {.text = strdup(""), .len = 0, .hl = NULL, .hl_open_comment = 0};
            if (new_line.text == NULL)
            {
                editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory (initial line text).");
            }
            editor_lines_array_append(&E->lines, new_line);
            editor_set_status_message("New file: %s", filename);
        }
        else
        {
            editor_handle_error(ERR_FILE_OPERATION, "Error opening file '%s': %s", filename,
                                strerror(errno));
        }
        return;
    }

    char* line_buffer = NULL;
    size_t linecap = 0;
    ssize_t linelen;

    while ((linelen = getline(&line_buffer, &linecap, fp)) != -1)
    {
        while (linelen > 0 &&
               (line_buffer[linelen - 1] == '\n' || line_buffer[linelen - 1] == '\r'))
        {
            linelen--;
        }

        EditorLine new_line = {
            .text = malloc(linelen + 1), .len = linelen, .hl = NULL, .hl_open_comment = 0};
        if (new_line.text == NULL)
        {
            free(line_buffer);
            editor_handle_error(ERR_OUT_OF_MEMORY, "Out of memory (line text).");
        }
        memcpy(new_line.text, line_buffer, linelen);
        new_line.text[linelen] = '\0';
        editor_lines_array_append(&E->lines, new_line);
    }
    free(line_buffer);
    fclose(fp);

    for (int i = 0; i < E->lines.size; i++)
    {
        editor_update_syntax(i);
    }

    E->dirty = 0;
    editor_set_status_message("Opened file: %s (%d lines)", filename, E->lines.size);
}

void editor_save_file(void)
{
    EditorConfig* E = get_editor_config();
    if (!E->filename)
    {
        char* new_filename = editor_prompt("Save as: %s (ESC to cancel)", "");
        if (new_filename == NULL)
        {
            editor_set_status_message("Save cancelled.");
            return;
        }
        if (E->filename)
            free(E->filename);
        E->filename = new_filename;
        editor_select_syntax_highlight();
    }

    FILE* fp = fopen(E->filename, "w");
    if (!fp)
    {
        editor_set_status_message("Error saving file: %s", strerror(errno));
        return;
    }

    for (int i = 0; i < E->lines.size; ++i)
    {
        fprintf(fp, "%s\n", E->lines.elements[i].text);
    }
    fclose(fp);
    E->dirty = 0;
    editor_set_status_message("File saved: %s", E->filename);
}

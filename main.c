#include "editor.h"
#include "editor_lines_array.h"
#include "error_handler.h"
#include "file.h"
#include "syntax.h"
#include "ui.h"

#include <string.h>

EditorConfig *E = NULL;

int main(int argc, char *argv[]) {
  E = get_editor_config();
  init_editor();

  if (argc >= 2) {
    editor_read_file(argv[1]);
  } else {
    init_editor_lines_array(&E->lines);
    EditorLine empty_line = {
        .text = strdup(""), .len = 0, .hl = NULL, .hl_open_comment = 0};
    if (empty_line.text == NULL) {
      editor_handle_error(ERR_OUT_OF_MEMORY,
                          "Out of memory (main empty line text).");
    }
    editor_lines_array_append(&E->lines, empty_line);
    editor_update_syntax(0);
    editor_set_status_message(
        "ErwinText: Press Ctrl+Q to quit. Ctrl+S to save. Ctrl+F to find.");
  }

  editor_refresh_screen();

  while (1) {
    editor_process_keypress();
  }

  return 0;
}

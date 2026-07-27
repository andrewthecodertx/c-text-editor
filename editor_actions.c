#include "editor_actions.h"

#include <stdlib.h>

void editor_action_free(EditorAction *action) {
  if (action == NULL) {
    return;
  }
  if (action->type == ACTION_DELETE_LINE && action->line_content != NULL) {
    free(action->line_content);
    action->line_content = NULL;
    action->line_len = 0;
  }
}

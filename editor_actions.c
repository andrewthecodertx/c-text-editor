#include "editor_actions.h"
#include "editor.h"
#include "editor_lines_array.h"
#include "syntax.h"
#include "ui.h"

#include <stdlib.h>
#include <string.h>

// Free heap memory owned by an action
void editor_action_free(EditorAction* action)
{
    if (action == NULL)
    {
        return;
    }
    if (action->type == ACTION_DELETE_LINE && action->line_content != NULL)
    {
        free(action->line_content);
        action->line_content = NULL;
        action->line_len = 0;
    }
}

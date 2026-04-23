#include "error_handler.h"
#include "editor.h" // For cleanup_editor
#include "ui.h"     // For editor_set_status_message
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void editor_handle_error(EditorErrorCode code, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  // 1. Print to stderr (for developers/debugging)
  fprintf(stderr, "Error [%d]: ", code);
  vfprintf(stderr, fmt, ap);
  fprintf(stderr, "\n");

  va_end(ap);

  // 2. Set editor status message (for user feedback)
  // Re-initialize va_list for vsnprintf
  va_start(ap, fmt);
  char user_message[256]; // Or a suitable size
  vsnprintf(user_message, sizeof(user_message), fmt, ap);
  editor_set_status_message("ERROR: %s", user_message);
  va_end(ap);

  // 3. Decide on termination based on error code or severity
  if (code == ERR_OUT_OF_MEMORY || code == ERR_FILE_OPERATION) { // Fatal errors
    cleanup_editor(); // Ensure resources are freed
    exit(1);
  }
  // For non-fatal errors, simply return and let the calling function handle
  // recovery
}

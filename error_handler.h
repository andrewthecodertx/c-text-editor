#ifndef ERROR_HANDLER_H

#define ERROR_HANDLER_H

#include "error_codes.h"

void editor_handle_error(EditorErrorCode code, const char* fmt, ...);

#endif // ERROR_HANDLER_H

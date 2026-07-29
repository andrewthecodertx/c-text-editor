#ifndef UI_H

#define UI_H

void editor_draw_rows(void);
void editor_refresh_screen(void);
void editor_draw_status_bar(void);
void editor_set_status_message(const char* fmt, ...);
void editor_draw_message_bar(void);
void editor_draw_clock(void);
void editor_scroll(void);
int get_cx_display(void);
char* editor_prompt(const char* prompt_fmt, ...);
void editor_handle_resize(void);

#include <time.h>

extern time_t status_message_time;

#endif // UI_H

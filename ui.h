#ifndef UI_H

#define UI_H

void editor_draw_rows();
void editor_refresh_screen();
void editor_draw_status_bar();
void editor_set_status_message(const char* fmt, ...);
void editor_draw_message_bar();
void editor_draw_clock();
void editor_scroll();
int get_cx_display();
char* editor_prompt(const char* prompt_fmt, ...);

#include <time.h>

extern time_t status_message_time;

#endif // UI_H

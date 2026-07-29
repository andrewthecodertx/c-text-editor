#ifndef SYNTAX_H

#define SYNTAX_H

enum EditorHighlight
{
    HL_NORMAL = 0,
    HL_COMMENT,
    HL_KEYWORD1,
    HL_KEYWORD2,
    HL_STRING,
    HL_NUMBER,
    HL_MATCH,
    HL_PREPROC
};

typedef struct
{
    char** filetype_extensions;
    char** keywords1;
    char** keywords2;
    char* singleline_comment_start;
    char* multiline_comment_start;
    char* multiline_comment_end;
} EditorSyntax;

void editor_select_syntax_highlight(void);
void editor_update_syntax(int filerow);
int is_separator(int c);

#endif // SYNTAX_H

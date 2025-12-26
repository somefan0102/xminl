#ifndef XMINL_H
#define XMINL_H

#include <stddef.h>

struct XMINL_Lex {
    enum {
        XMINL_LEX_NONE,
        XMINL_LEX_TAG_START,
        XMINL_LEX_TAG_END,
        XMINL_LEX_TAG_EMPTY,
        XMINL_LEX_ATTR_NAME,
        XMINL_LEX_ATTR_VALUE,
        XMINL_LEX_CDATA,
        XMINL_LEX_PI
    } type;
    char *value;
};

struct XMINL_Handler {
    char *data_buf, *data_bufp;
    struct XMINL_Lex *lex_buf, *lex_bufp;
    size_t data_len, lex_len;
    char *error_message, *error_location;
};

struct XMINL_Handler xminl_set(char *data, size_t data_len, struct XMINL_Lex *lex, size_t lex_len);
int xminl_lex(struct XMINL_Handler *handle, char *document);

#endif
#ifndef XMINL_H
#define XMINL_H

#include <stddef.h>

struct XMINL_Token {
    enum {
        XMINL_TOKEN_NONE,
        XMINL_TOKEN_TAG_START,
        XMINL_TOKEN_TAG_END,
        XMINL_TOKEN_TAG_EMPTY,
        XMINL_TOKEN_ATTR_NAME,
        XMINL_TOKEN_ATTR_VALUE,
        XMINL_TOKEN_CDATA,
        XMINL_TOKEN_PI
    } type;
    char *value;
};

struct XMINL_Handler {
    char *data_buf, *data_bufp;
    struct XMINL_Token *token_buf, *token_bufp;
    size_t data_len, token_len;
    char *error_message, *error_location;
};

struct XMINL_Handler xminl_set(char *data, size_t data_len, struct XMINL_Token *token_buf, size_t token_len);
int xminl_lex(struct XMINL_Handler *handle, char *document);

#endif
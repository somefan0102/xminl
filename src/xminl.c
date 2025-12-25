#include <ctype.h>
#include <string.h>
#include "xminl.h"

#define xminl_is_name_start(c) (strchr(":_", (c)) || isalpha(c) || (c) > 0x7f)
#define xminl_error(x, m, l) (x)->error_message = (m); (x)->error_location = (l);

int xminl_push(struct XMINL_Handler *x, int type, char *s, size_t len) {
    char *value = x->data_bufp;
    struct XMINL_Lex *lex = x->lex_bufp;

    if (value + len + 1 > x->data_buf + x->data_len) {
        xminl_error(x, "String data exhausted", NULL);
        return 1;
    } else if (lex + 1 > x->lex_buf + x->lex_len) {
        xminl_error(x, "Lexeme data exhausted", NULL);
        return 1;
    }

    memcpy(value, s, sizeof(char)*(len + 1));
    value[len] = '\0';
    x->data_bufp += len + 1;

    memset(lex, 0, sizeof(struct XMINL_Lex));
    lex->type = type;
    lex->value = value;
    x->lex_bufp++;

    return 0;
}

static size_t xminl_lex_space(struct XMINL_Handler *x, char *s) {
    char *t = s;
    while (strchr(" \t\r\n", *s))
        s++;
    return s-t;
}

static size_t xminl_lex_name(struct XMINL_Handler *x, char *s) {
    char *t = s;

    if (!xminl_is_name_start(*s)) {
        xminl_error(x, "Invalid name", t);
        return 0;
    }

    while (xminl_is_name_start(*s) || isdigit(*s) || strchr(".-", *s))
        s++;
    
    return s-t;
}

static size_t xminl_lex_group(struct XMINL_Handler *x, char *s, char *start, char *end, char *start_error, char *end_error) {
    char *t = s;

    if (!start || !end) {
        xminl_error(x, "Invalid start and end group delimiters", NULL);
        return 0;
    }

    if (strncmp(s, start, strlen(start))) {
        xminl_error(x, start_error, t);
        return 0;
    }
    s += strlen(start);

    while (strncmp(s, end, strlen(end)) && *s)
        s++;
    
    if (!*s) {
        xminl_error(x, end_error, t);
        return 0;
    }
    s += strlen(end);

    return s-t;
}

static size_t xminl_lex_pi(struct XMINL_Handler *x, char *s) {
    int res = xminl_lex_group(x, s, "<?", "?>",
        "Invalid start PI declaration", "Invalid end PI declaration"
    );

    if (!res || xminl_push(x, XMINL_LEX_PI, s, res))
        return 0;

    return res;
}

static size_t xminl_lex_comment(struct XMINL_Handler *x, char *s) {
    return xminl_lex_group(x, s, "<!--", "-->",
        "Invalid start comment", "Invalid end comment"
    );
}

static size_t xminl_lex_cdata(struct XMINL_Handler *x, char *s) {
    int res = xminl_lex_group(x, s, "<![CDATA[", "]]>",
        "Invalid start CDATA section", "Invalid end CDATA section"
    );

    if (!res || xminl_push(x, XMINL_LEX_CDATA, s+strlen("<![CDATA["), res-strlen("<![CDATA[")-strlen("]]>")))
        return 0;

    return res;
}

static size_t xminl_lex_cdata_raw(struct XMINL_Handler *x, char *s) {
    char *t = s;

    while (*s != '<' && *s != '&')
        s++;

    if (s == t || xminl_push(x, XMINL_LEX_CDATA, t, s-t))
        return 0;

    return s-t;
}

static size_t xminl_lex_attributes(struct XMINL_Handler *x, char *s) {
    char *t = s;
    int done = 0;

    while (!done && *s) {
        char *name, *value, quote;
        s += xminl_lex_space(x, s);

        if (xminl_is_name_start(*s)) {
            name = s;
            s += xminl_lex_name(x, s);
            if (xminl_push(x, XMINL_LEX_ATTR_NAME, name, s-name))
                return 0;
        } else {
            done = 1;
            continue;
        }

        s += xminl_lex_space(x, s);
        if (*s != '=') {
            xminl_error(x, "Missing equal", t);
            return 0;
        }
        s++;
        s += xminl_lex_space(x, s);

        if (*s != '"' && *s != '\'') {
            xminl_error(x, "Missing start quote", t);
            return 0;
        }
        quote = *s;
        s++;

        value = s;
        while (*s != quote && *s)
            s++;

        if (!*s) {
            xminl_error(x, "Missing end quote", t);
            return 0;
        }

        if (xminl_push(x, XMINL_LEX_ATTR_VALUE, value, s-value))
            return 0;
        s++;
    }

    return s-t;
}

/* TODO */
static size_t xminl_lex_content(struct XMINL_Handler *x, char *s) {
    return 0;
}

/* TODO */
static size_t xminl_lex_element(struct XMINL_Handler *x, char *s) {
    return 0;
}

struct XMINL_Handler xminl_set(char *data, size_t data_len, struct XMINL_Lex *lex, size_t lex_len) {
    struct XMINL_Handler x;
    memset(&x, 0, sizeof(x));
    memset(lex, 0, sizeof(struct XMINL_Lex)*lex_len);
    memset(data, 0, sizeof(char)*data_len);
    x.data_buf = x.data_bufp = data;
    x.lex_buf = x.lex_bufp = lex;
    x.data_len = data_len;
    x.lex_len = lex_len;
    return x;
}

/* TODO */
int xminl_lex(struct XMINL_Handler *x, char *s) {
    xminl_error(x, "You haven't implemented the function", NULL);
    return 1;
}
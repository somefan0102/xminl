#include <ctype.h>
#include <string.h>
#include "xminl.h"

#define xminl_is_name_start(c) (strchr(":_", (c)) || isalpha(c) || (c) > 0x7f)
#define xminl_error(x, m, l) (x)->error_message = (m); (x)->error_location = (l);

static int xminl_push(struct XMINL_Handler *x, int type, char *s, size_t len) {
    char *value = x->data_bufp;
    struct XMINL_Token *token = x->token_bufp;

    if (value + len + 1 > x->data_buf + x->data_len) {
        xminl_error(x, "String data exhausted", NULL);
        return 1;
    } else if (token + 1 > x->token_buf + x->token_len) {
        xminl_error(x, "Token data exhausted", NULL);
        return 1;
    }

    memcpy(value, s, sizeof(char)*(len + 1));
    value[len] = '\0';
    x->data_bufp += len + 1;

    memset(token, 0, sizeof(struct XMINL_Token));
    token->type = type;
    token->value = value;
    x->token_bufp++;

    return 0;
}

static size_t xminl_lex_space(struct XMINL_Handler *x, char *s) {
    char *t = s;
    while (isspace(*s))
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
        xminl_error(x, "No start and end group delimiters", NULL);
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
    int res = xminl_lex_group(x, s, "<?", "?>", "Invalid start PI declaration", "Invalid end PI declaration");

    if (!res || xminl_push(x, XMINL_TOKEN_PI, s, res))
        return 0;

    return res;
}

static size_t xminl_lex_comment(struct XMINL_Handler *x, char *s) {
    return xminl_lex_group(x, s, "<!--", "-->", "Invalid start comment", "Invalid end comment");
}

static size_t xminl_lex_cdata(struct XMINL_Handler *x, char *s) {
    int res = xminl_lex_group(x, s, "<![CDATA[", "]]>", "Invalid start CDATA section", "Invalid end CDATA section");

    if (!res || xminl_push(x, XMINL_TOKEN_CDATA, s+strlen("<![CDATA["), res-strlen("<![CDATA[")-strlen("]]>")))
        return 0;

    return res;
}

static size_t xminl_lex_cdata_raw(struct XMINL_Handler *x, char *s) {
    char *t = s;

    while (*s != '<')
        s++;

    if (s == t || xminl_push(x, XMINL_TOKEN_CDATA, t, s-t))
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
            if (xminl_push(x, XMINL_TOKEN_ATTR_NAME, name, s-name))
                return -1;
        } else {
            done = 1;
            continue;
        }

        s += xminl_lex_space(x, s);
        if (*s != '=') {
            xminl_error(x, "Missing equal for attribute", s);
            return -1;
        }
        s++;
        s += xminl_lex_space(x, s);

        if (*s != '"' && *s != '\'') {
            xminl_error(x, "Missing start quote for attribute value", s);
            return -1;
        }
        quote = *s;
        s++;

        value = s;
        while (*s != quote && *s)
            s++;

        if (!*s) {
            xminl_error(x, "Missing end quote for attribute value", s);
            return -1;
        }

        if (xminl_push(x, XMINL_TOKEN_ATTR_VALUE, value, s-value))
            return -1;
        s++;
    }

    return s-t;
}

static size_t xminl_lex_stag(struct XMINL_Handler *x, char *s) {
    char *t = s;
    int res;
    char *name;
    size_t name_len;

    if (*s != '<') {
        xminl_error(x, "No start bracket for tag", s);
        return 0;
    }
    s++;

    if (!(res = xminl_lex_name(x, s)))
        return 0;
    if (xminl_push(x, XMINL_TOKEN_TAG_START, s, res))
        return 0;
    name = s;
    name_len = res;
    s += res;

    if ((res = xminl_lex_attributes(x, s)) == -1)
        return 0;
    s += res;

    if (!strncmp(s, "/>", strlen("/>"))) {
        s += strlen("/>");
        if (xminl_push(x, XMINL_TOKEN_TAG_EMPTY, name, name_len))
            return 0;
        return s-t;
    } else if (*s != '>') {
        xminl_error(x, "No closing bracket for start tag", s);
        return 0;
    }
    s++;

    return s-t;
}

static size_t xminl_lex_etag(struct XMINL_Handler *x, char *s) {
    char *t = s;
    int res;
    char *name;
    size_t name_len;

    if (strncmp(s, "</", strlen("</"))) {
        xminl_error(x, "No starting bracket for closing tag", s);
        return 0;
    }
    s += strlen("</");

    if (!(res = xminl_lex_name(x, s)))
        return 0;
    name = s;
    name_len = res;
    s += res;
    s += xminl_lex_space(x, s);

    if (xminl_push(x, XMINL_TOKEN_TAG_END, name, name_len))
        return 0;

    if (*s != '>') {
        xminl_error(x, "No closing bracket for end tag", s);
        return 0;
    }
    s++;

    return s-t;
}

struct XMINL_Handler xminl_set(char *data, size_t data_len, struct XMINL_Token *token, size_t token_len) {
    struct XMINL_Handler x;
    memset(&x, 0, sizeof(x));
    memset(token, 0, sizeof(struct XMINL_Token)*token_len);
    memset(data, 0, sizeof(char)*data_len);
    x.data_buf = x.data_bufp = data;
    x.token_buf = x.token_bufp = token;
    x.data_len = data_len;
    x.token_len = token_len;
    return x;
}

int xminl_lex(struct XMINL_Handler *x, char *s) {
    int done = 0;

    while (!done && *s) {
        int res = 0;

        if (!strncmp(s, "<?", strlen("<?"))) {
            if (!(res = xminl_lex_pi(x, s)))
                return 1;
        } else if (!strncmp(s, "<!--", strlen("<!--"))) {
            if (!(res = xminl_lex_comment(x, s)))
                return 1;
        } else if (!strncmp(s, "<![CDATA[", strlen("<![CDATA["))) {
            if (!(res = xminl_lex_cdata(x, s)))
                return 1;
        } else if (*s == '<' && xminl_is_name_start(s[1])) {
            if (!(res = xminl_lex_stag(x, s)))
                return 1;
        } else if (!strncmp(s, "</", strlen("</"))) {
            if (!(res = xminl_lex_etag(x, s)))
                return 1;
        } else if (isspace(*s)) {
            if (!(res = xminl_lex_space(x, s)))
                return -1;
        } else {
            res = xminl_lex_cdata_raw(x, s);
        }

        if (res) {
            s += res;
        } else {
            done = 1;
        }
    }

    return 0;
}

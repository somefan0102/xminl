/* Basic demo of xminl */
#include <stdio.h>
#include <string.h>
#include "xminl.h"

#define DATA_SIZE 0xfff
#define TOKEN_SIZE 0xfff
#define DOCUMENT_SIZE 0xfff
#define FILENAME "test.xml"

char *type_to_str(int type) {
    static char *names[] = {
        "NONE", "TAG_START", "TAG_END", "TAG_EMPTY",
        "ATTR_NAME", "ATTR_VALUE", "CDATA", "PI"
    };
    return names[type];
}

int main(int argc, char *argv[]) {
    /*
        XMINL_Handler needs a big string buffer and token (struct) buffer to work.
        The string buffer is used to copy and store CDATA, tag and attribute names, etc. etc.. from the doc.
        The token buffer is used to store these so-called structs called tokens, which each has a type and value.
        The value of the token buffer is a pointer to a string in the string buffer that corresponds to the type.
    */
    char data[DATA_SIZE];
    struct XMINL_Token tokens[TOKEN_SIZE];

    /*
        xminl_set() is used to handle setting the buffers and their sizes and returns a XMINL_Handler.
        You can init the struct set the members of it yourself, but the func is for ease of use.
    */
    struct XMINL_Handler x = xminl_set(data, DATA_SIZE, tokens, TOKEN_SIZE);

    int i, count;

    /* file setup stuff. move along */
    char document[DOCUMENT_SIZE];
    FILE *xml_file = fopen(FILENAME, "r");

    if (!xml_file) {
        fprintf(stderr, "Error: Failed to open file.\n");
        return 1;
    }

    count = fread(document, sizeof(char), DOCUMENT_SIZE, xml_file);
    document[count] = '\0';

    /*
        xminl_lex begins the lexing process. This is the only time where you use the doc string arg.
        If it fails, it returns 1, else 0 if success.
        Use XMINL_Handler's members error_message and
        error_location (pointer to the causing char in the doc) to find the issue.
    */
    if (xminl_lex(&x, document)) {
        fprintf(stderr, "Error: %s\n", x.error_message);
        return 1;
    }

    /* Print all the tokens, both its type and its value */
    for (i = 0; i < TOKEN_SIZE && tokens[i].type; i++) {
        printf("%12s | %s\n", type_to_str(x.token_buf[i].type), x.token_buf[i].value);
    }

    return 0;
}
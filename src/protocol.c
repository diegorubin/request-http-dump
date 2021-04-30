#include "protocol.h"

#define separator "\n"

char* reasons[] = {
    "OK",
    "NOT FOUND",
};

unsigned statuses[] = {
    200,
    404,
};

/**
 * Response = Status-Line
 *            *(( general-header
 *             | response-header
 *             | entity-header ) CRLF)
 *            CRLF
 *            [ message-body ]
 * Status-Line = HTTP-Version SP Status-Code SP Reason-Phrase CRLF
 */
void send_response(int clientfd, response_t* response) {

    char status_line[256];
    bzero(status_line, 256);
    sprintf(status_line, "%s %u %s\n", response->protocol_version,
            response->status, response->reason);
    write(clientfd, status_line, strlen(status_line));

    char content_length[20];
    if (response->body != NULL) {
        sprintf(content_length, "Content-Length: %lu\n", strlen(response->body));
        write(clientfd, content_length, strlen(content_length));
    } else {
        sprintf(content_length, "Content-Length: 0\n");
        write(clientfd, content_length, strlen(content_length));
    }

    write(clientfd, separator, strlen(separator));

    if (response->body != NULL) {
        write(clientfd, response->body, strlen(response->body));
    }
}

char read_from_buffer(FILE* input) {
    char c = getc(input);
    printf("%c", c);
    return c;
}

void unread_from_buffer(char c, FILE* input) {
    ungetc(c, input);
    printf("\b");
}

void read_reason(unsigned status, char* dest) {
    for (unsigned i = 0; i < (sizeof(statuses) / sizeof(unsigned)); i++) {
        if (statuses[i] == status) {
            strcpy(dest, reasons[i]);
        }
    }
}

int read_method(FILE* input) {
    int lexeme_position = 0;
    char method_str[10];
    char c;

    while (!isspace(c = read_from_buffer(input))) {
        method_str[lexeme_position++] = c;
    }
    method_str[lexeme_position] = '\0';
    unread_from_buffer(c, input);

    if (strcmp("GET", method_str) == 0) {
        return GET;
    }

    if (strcmp("POST", method_str) == 0) {
        return POST;
    }

    return NOT_IMPLEMENTED;
}

void read_path(FILE* input, char* dest) {
    int lexeme_position = 0;
    char lexeme[1024];
    char c;

    while (!isspace(c = read_from_buffer(input))) {
        lexeme[lexeme_position++] = c;
    }
    lexeme[lexeme_position] = '\0';
    strcpy(dest, lexeme);
}

void read_version(FILE* input, char* dest) {
    int lexeme_position = 0;
    char lexeme[50];
    char c;
    while (!isspace(c = read_from_buffer(input))) {
        lexeme[lexeme_position++] = c;
    }
    lexeme[lexeme_position] = '\0';
    strcpy(dest, lexeme);
}

void skip_whitespace(FILE* input) {
    char c;
    while ((isspace(c = read_from_buffer(input))))
        ;
    unread_from_buffer(c, input);
}

void read_header(request_t* request, FILE* input) {

    char c;
    char key[255];
    char value[255];
    int position = 0;

    while ((c = read_from_buffer(input)) != ':') {
        key[position++] = c;
    }
    key[position] = '\0';

    skip_whitespace(input);

    position = 0;
    while ((c = read_from_buffer(input)) != '\n') {
        value[position++] = c;
    }
    value[position] = '\0';

    if (strcmp(key, "Content-Length") == 0) {
        request->content_length = atoi(value);
    }

}

void read_headers(request_t* request, FILE* input) {

    char c;
    while ((c = read_from_buffer(input)) != 13) {
        unread_from_buffer(c, input);
        read_header(request, input);
    }
}

void read_body(request_t* request, FILE* input) {
    for (unsigned i = 0; i <= request->content_length; i++) {
        read_from_buffer(input);
    }
}

request_t* parse(FILE* input) {
    request_t* request = (request_t*)malloc(sizeof(request_t));
    request->content_length = 0;

    request->method = read_method(input);
    skip_whitespace(input);
    read_path(input, request->path);
    skip_whitespace(input);
    read_version(input, request->protocol_version);

    read_headers(request, input);

    read_body(request, input);
    printf("\n\n");

    return request;
}

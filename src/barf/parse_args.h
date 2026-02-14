#pragma once

#include "platform/platform.h"
#include "libc/string.h"

static void parse_input(const char* input, int size, const char* path, int* argc, char*** argv) {

    char** args = mem__alloc(8*50, NULL);
    char* text = mem__alloc(50*50, NULL);
    int argi = 0;
    int text_len = 0;

    {
        int len = strlen(path);
        args[argi] = text + text_len;
        memcpy(text + text_len, path, len);
        text[text_len + len] = '\0';
        text_len += len + 1;
        argi++;
    }

    int start_head = 0;
    int head = 0;
    while (head < size) {
        if (input[head] == ' ' || head+1 == size) {
            if (head+1 == size) {
                head++;
            }
            int len = head - start_head;
            if (len > 0) {
                args[argi] = text + text_len;
                memcpy(text + text_len, input + start_head, len);
                text[text_len + len] = '\0';
                text_len += len + 1;
                argi++;
            }
            start_head = head + 1;
        }
        // TODO: Parse quoted string, handle escapes
        head++;
    }

    *argv = args;
    *argc = argi;
}

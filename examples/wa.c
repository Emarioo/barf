
// extern void prints(const char* text);

// #include <stdarg.h>
// #include <stdio.h>


// extern void prints(const char* msg);

// extern void vprintf(const char* format, va_list va) {
    

#include "platform/platform.h"
#include "libc/string.h"

#include <immintrin.h>
// #include <stdlib.h>

// float okay;
// float first[64] = {0.5};
// static float second[64] = {0.7};

// int uninit;

extern int counter;
int update(int value);

void parse_input(const char* input, int size, int* argc, char*** argv) {

    char** args = mem__alloc(8*50, NULL);
    *argv = mem__alloc(8*50, NULL);
    char* text = mem__alloc(50*50, NULL);
    int argi = 0;
    int text_len = 0;

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


int ba_entry(const char* path, const char* data, int size) {

    log__printf("entry from %s\n", path);

    int argc;
    char** argv;
    parse_input(data, size, &argc, &argv);

    for (int i=0;i<argc;i++) {
        log__printf("%d %s\n", i, argv[i]);
    }

    FSHandle file = fs__open("README.md", FS_READ);

    char text[24];
    text[23] = 0;
    fs__read(file, 0, text, 23);
    log__printf("%s\n", text);

    fs__close(file);

    int val = update(9);
    log__printf("vals %d %d\n", counter, val);


    // FILE* file = fopen("README.md", "rb");
    // fclose(file);

    // prints("hello");
    // prints("again");

    // __m256 v = _mm256_load_ps(first);
    // printf("%f\n", ((float*)&v)[4]);
    log__printf("end entry\n");
    return 0;
}


// void main(int argc, char** argv) {
//     entry("temp", argv[0], 1);
// }
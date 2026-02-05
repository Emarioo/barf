
// extern void prints(const char* text);

// #include <stdarg.h>
// #include <stdio.h>


// extern void prints(const char* msg);

// extern void vprintf(const char* format, va_list va) {
    
// int vsnprintf(char* buffer, int size, const char* format, va_list va) {
//     if(!buffer || !size)
//         return 0;

//     int format_len = strlen(format);
//     int head = 0;
//     int i = 0;
    
//     #define CHECK if (head-1 >= size) { buffer[head] = '\0'; return head; }
    
//     while (i < format_len) {
//         if (format[i] != '%') {
//             buffer[head] = format[i];
//             head++;
//             CHECK

//             i++;
//             continue;
//         }
//         i++;
//         if (i >= format_len)
//             break;        

//         int width = 0;

//         if (format[i] >= '0' && format[i] <= '9') {
//             width = format[i] - '0';
//             i++;
//         }

//         if (i >= format_len)
//             break;

//         if (format[i] == 'd') {
//             i++;

//             int value = va_arg(va, int);
//             int len = output_int(buffer + head, size - head, value);
//             head += len;
//             CHECK
//         } else if (format[i] == 'c') {
//             i++;

//             char value = va_arg(va, int);
//             buffer[head] = value;
//             head += 1;
//             CHECK
//         } else if (format[i] == 'x') {
//             i++;

//             int value = va_arg(va, int);

//             // if (width > 0) {
//             //     int num_leading_zero_bits;

//             //     // asm("lzcnt %1, %0"
//             //     //     : "=r"(num_leading_zero_bits)
//             //     //     : "r"(value));
                
//             //     int padding = width > (32-num_leading_zero_bits) / 4 ? width - (32-num_leading_zero_bits) / 4 : 0;

//             //     for (int i = 0; i < padding; i++) {
//             //         buffer[head] = '0';
//             //         head++;
//             //         CHECK
//             //     }
//             // }

//             int len = output_hex(buffer + head, size - head, value, width);
//             head += len;
//             CHECK
//         } else if (format[i] == 's') {
//             i++;
            
//             const char* value = va_arg(va, const char*);
//             int len = strlen(value);
            
//             len = len > size-head ? size-head : len;
//             memcpy(buffer + head, value, len);
//             head += len;
//             CHECK
//         } else {
//             buffer[head] = '%';
//             head++;
//             CHECK
//         }
//     }

//     buffer[head] = '\0';
//     return head;
// }

// void printf(const char* format, ...) {
//     va_list va;
//     va_start(va, format);
//     vprintf(format, va);
//     va_end(va);
// }

#include "platform/platform.h"

#include <immintrin.h>
#include <stdlib.h>
#include <string.h>

// float okay;
float first[64] = {0.5};
static float second[64] = {0.7};

int uninit;

void* memcpy(void* dst, const void* src, size_t size) {
    for(int i=0;i<size;i++)
        *((char*)dst + i) = *((char*)src + i);
    return dst;
}

void parse_input(const char* input, int size, int* argc, char*** argv) {

    char** args = mem__alloc(8*50, NULL);
    *argv = mem__alloc(8*50, NULL);
    char* text = mem__alloc(50*50, NULL);
    int argi = 0;
    int text_len = 0;

    int start_head = 0;
    int head = 0;
    while (head < size) {
        if (input[head] == ' ') {
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


int entry(const char* path, const char* data, int size) {

    log__printf("entry from %s\n", path);

    int argc;
    char** argv;
    parse_input(data, size, &argc, &argv);



    FSHandle file = fs__open("README.md", FS_READ);

    char text[24];
    text[23] = 0;
    fs__read(file, 0, text, 23);
    log__printf("%s\n", text);

    fs__close(file);


    // FILE* file = fopen("README.md", "rb");
    // fclose(file);

    // prints("hello");
    // prints("again");

    // __m256 v = _mm256_load_ps(first);
    // printf("%f\n", ((float*)&v)[4]);
    log__printf("end entry\n");
    return 0;
}


// void main() {
//     entry("temp", 0, 0);
// }
#include "platform/platform.h"

#include "libc/string.h"

int ba_entry(const char* path, const char* data, int size) {
    log__printf("Hello from %s\n", path);
    return 0;
}

#if defined(OS_WINDOWS) || defined(OS_LINUX)


int main(int argc, const char** argv) {
    char path[512];

    strcpy(path, argv[0]);

    int len = strlen(path);
    int head = len;
    while (head > 0 && path[head] != '.' && path[head] != '/') head--;
    
    if (head >= 0 && path[head] == '.') {
        strcpy(path + head + 1, "ba");
    } else {
        strcpy(path + len, ".ba");
    }

    return ba_entry(path, NULL, 0);
}

#endif

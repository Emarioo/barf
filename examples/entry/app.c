
void log__printf(char* format, ...);

int ba_entry(const char* path, const char* data, int size) {
    log__printf("Hello from %s, %d bytes of data\n", path, size);
    log__printf("  %*.s\n", size, data);
    return 0;
}

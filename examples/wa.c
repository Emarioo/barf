
extern void prints(const char* text);

#include <immintrin.h>
#include <stdio.h>

// float okay;
float first[64] = {0.5};
static float second[64] = {0.7};

int uninit;

void main() {
    prints("hello");
    prints("again");

    __m256 v = _mm256_load_ps(first);
    printf("%f\n", ((float*)&v)[4]);
}

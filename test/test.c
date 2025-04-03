#include "../src/utils.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>


int test_extract_lsb();
int test_extract_msb();
int test_extract_bits(uint32_t value, int start, int count,
        uint32_t expected, bool msb);

void print_bits(uint32_t value);

int main() {
    int any_failed = 0;

    any_failed = any_failed || test_extract_lsb();
    any_failed = any_failed || test_extract_msb();

    return any_failed;
}


// Test the extract_lsb function
int test_extract_lsb() {
    return test_extract_bits(0x87654321, 8, 18, 0x36543, false);
}

// Test the extract_msb function
int test_extract_msb() {
    return test_extract_bits(0x87654321, 7, 17, 0x16543, true);
}

int test_extract_bits(uint32_t value, int start, int count,
        uint32_t expected, bool msb) {

    char *type = msb ? "msb" : "lsb";

    printf("---- Testing: extract_%s ----\n", type);

    uint32_t result = msb ?
        extract_msb(value, start, count) : extract_lsb(value, start, count);

    printf("extract_%s(0x%x, %d, %d) = 0x%x\n",
            type, value, start, count, result);

    // If A-OK, don't print anything else
    if (result == expected) {
        printf("Test OK.\n");
        return 0;
    }

    // Not OK, then print the details

    printf("Test failed!\n");
    printf(" * Original value: 0x%x\n  = ", value);
    print_bits(value);
    printf(" * Extracting %d bits starting at %d.\n", count, start);
    printf(" Expected: 0x%x\n  = ", expected);
    print_bits(expected);
    printf(" Got:      0x%x\n  = ", result);
    print_bits(result);

    return 1;
}

void print_bits(uint32_t value) {
    printf("0b ");
    for (int i = 31; i >= 0; i--) {
        printf("%d", (value >> i) & 1);
        if (i % 4 == 0) {
            printf(" ");
        }
    }
    printf("\n");
}

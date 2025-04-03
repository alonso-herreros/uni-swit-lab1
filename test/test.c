#include "../src/utils.h"
#include <stdio.h>
#include <stdint.h>

int test_extract_bits();


int main() {
    int any_failed = 0;

    any_failed = any_failed || test_extract_bits();

    return any_failed;
}


// Test the extract_bits function
int test_extract_bits() {
    printf("---- Testing: extract_bits ----\n");
    uint32_t value = 0x87654321;
    int start = 8;               // Second byte, third nibble
    int count = 20;              // 20 bits, aka 5 nibbles

    uint32_t expected = 0x76543; // Expected result after extraction
    uint32_t result = extract_bits(value, start, count);

    printf("extract_bits(0x%x, %d, %d) = 0x%x\n", value, start, count, result);

    // If A-OK, don't print anything else
    if (result == expected) {
        printf("Test OK.\n");
        return 0;
    }

    // Not OK, then print the details

    printf("Test failed!\n");
    printf(" * Original value: 0x%x\n", value);
    printf(" * Extracting %d bits starting at %d.\n", count, start);
    printf(" Expected: 0x%x\n", expected);
    printf(" Got:      0x%x\n", result);

    return 1;
}

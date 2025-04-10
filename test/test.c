#include "../src/utils.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

// ==== Macros ====
#define TEST_FAIL(format, ...) \
    do { \
        printf("! TEST FAIL ! " format, ##__VA_ARGS__); \
        return 1; \
    } while (0)
#define TEST_REPORT(name, fails) \
    do { \
        if (fails == 0) printf("\n=== Test " name ": all passed ===\n"); \
        else printf("\n!!! Test " name ": %d FAILED !!!\n", fails); \
    } while (0)

// ==== Prototypes ====


int test_extract_lsb();
int test_extract_msb();

void print_bits(uint32_t value);

int main() {
    int fails = 0;

    fails += test_extract_lsb();
    fails += test_extract_msb();

    TEST_REPORT("ALL", fails);

    return fails;
}

// ==== Test functions ====

// Wrapper function for extract_lsb and extract_msb
int _test_extract_bits(uint32_t value, int start, int count,
        uint32_t expected, bool msb) {

    char *type = msb ? "MSB at 0" : "LSB at 0";

    uint32_t result = msb ?
        extract_msb(value, start, count) : extract_lsb(value, start, count);

    printf("Extract %d, start at %d from 0x%X (%s): 0x%X (expected 0x%X)\n",
            count, start, value, type, result, expected);

    // If A-OK, don't print anything else
    if (result != expected) {
        printf("Input:");
        print_bits(value);
        printf("Result | Expected:\n");
        print_bits(result);
        print_bits(expected);
        TEST_FAIL("Result doesn't match\n");
    }

    return 0;
}

// Test the extract_lsb function
int test_extract_lsb() {
    return _test_extract_bits(0x87654321, 8, 18, 0x36543, false);
}

// Test collection for extract_msb
int test_extract_msb() {
    printf("=== Testing extract_msb ===\n");
    int fails = 0;

    // Test 1: Extract first octet (MSB)
    printf("\n--- Test Case 1: First octet extraction ---\n");
    fails += _test_extract_bits(0xC0A80101, 0, 8, 0xC0, true);

    // Test 2: Extract second octet
    printf("\n--- Test Case 2: Second octet extraction ---\n");
    fails += _test_extract_bits(0xC0A80101, 8, 8, 0xA8, true);

    // Test 3: Extract 4 bits from position 4
    printf("\n--- Test Case 3: 4-bit extraction from position 4 ---\n");
    fails += _test_extract_bits(0xC0A80101, 4, 4, 0x0, true);

    // Test 4: Extract 16 bits from start
    printf("\n--- Test Case 4: 16-bit extraction from start ---\n");
    fails += _test_extract_bits(0xC0A80101, 0, 16, 0xC0A8, true);

    // Test 5: Edge case (0 bits)
    printf("\n--- Test Case 5: Zero-bit extraction ---\n");
    fails += _test_extract_bits(0xC0A80101, 0, 0, 0, true);

    // Test 6: Edge case (0 bits)
    printf("\n--- Test Case 6: 17-bit extraction from position 7 ---\n");
    fails += _test_extract_bits(0x87654321, 7, 17, 0x16543, true);

    TEST_REPORT("extract_msb", fails);

    return fails;
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

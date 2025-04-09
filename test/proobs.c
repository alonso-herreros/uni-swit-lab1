#include "../src/lc_trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Function to print a rule in human-readable format
void print_rule(const Rule *rule) {
    printf("%u.%u.%u.%u/%u -> iface %u\n",
           (rule->prefix >> 24) & 0xFF,
           (rule->prefix >> 16) & 0xFF,
           (rule->prefix >> 8) & 0xFF,
           rule->prefix & 0xFF,
           rule->prefix_len,
           rule->out_iface);
}

// Function to print an array of rules
void print_rules(const Rule *rules, size_t count) {
    for (size_t i = 0; i < count; i++) {
        printf("[%zu] ", i);
        print_rule(&rules[i]);
    }
}

// Helper function to create a rule
Rule make_rule(const char *ip, uint8_t len, int iface) {
    Rule r;
    unsigned int a, b, c, d;
    sscanf(ip, "%u.%u.%u.%u", &a, &b, &c, &d);
    r.prefix = (a << 24) | (b << 16) | (c << 8) | d;
    r.prefix_len = len;
    r.out_iface = iface;
    return r;
}

// Test function for compute_skip
void test_compute_skip() {
    printf("\n=== Testing compute_skip ===\n");

    // Test case 1: All rules share first 16 bits
    Rule rules1[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.2.0", 24, 2),
        make_rule("192.168.3.0", 24, 3)};

    printf("\nTest Case 1 (Common 16 bits):\n");
    print_rules(rules1, 3);
    uint8_t skip = compute_skip(rules1, 3, 0);
    printf("Computed skip: %u bits\n", skip);

    // Test case 2: Different first octet
    Rule rules2[] = {
        make_rule("10.0.0.0", 8, 1),
        make_rule("192.168.0.0", 16, 2)};

    printf("\nTest Case 2 (Different first octet):\n");
    print_rules(rules2, 2);
    skip = compute_skip(rules2, 2, 0);
    printf("Computed skip: %u bits\n", skip);

    // Test case 3: Single rule
    printf("\nTest Case 3 (Single rule):\n");
    print_rules(rules1, 1);
    skip = compute_skip(rules1, 1, 0);
    printf("Computed skip: %u bits\n", skip);
}

// Test function for compute_branch
void test_compute_branch() {
    printf("\n=== Testing compute_branch ===\n");

    // Test case 1: Needs 2-bit branching
    Rule rules1[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.2.0", 24, 2),
        make_rule("192.168.3.0", 24, 3),
        make_rule("192.168.4.0", 24, 4)};

    printf("\nTest Case 1 (4 rules with different 3rd octet):\n");
    print_rules(rules1, 4);
    uint8_t branch = compute_branch(rules1, 4, 16); // After first 16 bits
    printf("Computed branch: %u bits\n", branch);

    // Test case 2: Single rule
    printf("\nTest Case 2 (Single rule):\n");
    print_rules(rules1, 1);
    branch = compute_branch(rules1, 1, 0);
    printf("Computed branch: %u bits\n", branch);

    // Test case 3: Needs 1-bit branching
    Rule rules3[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.129.0", 24, 2)};

    printf("\nTest Case 3 (2 rules with different MSB in 3rd octet):\n");
    print_rules(rules3, 2);
    branch = compute_branch(rules3, 2, 16); // After first 16 bits
    printf("Computed branch: %u bits\n", branch);
}

// Test function for sort_rules
void test_sort_rules() {
    printf("\n=== Testing sort_rules ===\n");

    // Test case 1: Mixed prefixes with default route
    Rule test1[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("0.0.0.0", 0, 2), // Default route
        make_rule("10.0.0.0", 8, 3),
        make_rule("192.168.0.0", 16, 4),
        make_rule("192.168.0.0", 24, 5), // Same network, longer prefix
        make_rule("10.0.0.0", 16, 6)     // Same network, longer prefix
    };

    printf("\nTest Case 1 (Mixed prefixes with default route):\n");
    printf("Original rules:\n");
    print_rules(test1, sizeof(test1) / sizeof(test1[0]));

    Rule *sorted1 = sort_rules(test1, sizeof(test1) / sizeof(test1[0]));
    printf("\nSorted rules:\n");
    print_rules(sorted1, sizeof(test1) / sizeof(test1[0]));
    free(sorted1);

    // Test case 2: Same network, different prefix lengths
    Rule test2[] = {
        make_rule("192.168.1.0", 28, 1),
        make_rule("192.168.1.0", 24, 2),
        make_rule("192.168.1.0", 16, 3),
        make_rule("192.168.1.0", 32, 4)};

    printf("\nTest Case 2 (Same network, different prefix lengths):\n");
    printf("Original rules:\n");
    print_rules(test2, sizeof(test2) / sizeof(test2[0]));

    Rule *sorted2 = sort_rules(test2, sizeof(test2) / sizeof(test2[0]));
    printf("\nSorted rules:\n");
    print_rules(sorted2, sizeof(test2) / sizeof(test2[0]));
    free(sorted2);

    // Test case 3: Multiple default routes (shouldn't happen but test anyway)
    Rule test3[] = {
        make_rule("0.0.0.0", 0, 2),
        make_rule("0.0.0.0", 0, 1), // Same default, different interface
        make_rule("10.0.0.0", 8, 3)};

    printf("\nTest Case 3 (Multiple default routes):\n");
    printf("Original rules:\n");
    print_rules(test3, sizeof(test3) / sizeof(test3[0]));

    Rule *sorted3 = sort_rules(test3, sizeof(test3) / sizeof(test3[0]));
    printf("\nSorted rules:\n");
    print_rules(sorted3, sizeof(test3) / sizeof(test3[0]));
    free(sorted3);
}

// Test function for compute_default
void test_compute_default() {
    printf("\n=== Testing compute_default ===\n");

    // Test case 1: Has default route
    Rule rules1[] = {
        make_rule("0.0.0.0", 0, 1), // Default route
        make_rule("192.168.1.0", 24, 2),
        make_rule("192.168.0.0", 16, 3)};

    printf("\nTest Case 1 (With default route):\n");
    print_rules(rules1, 3);
    Rule *default_rule = compute_default(rules1, 3, 0);
    printf("Default rule: ");
    if (default_rule)
        print_rule(default_rule);
    else
        printf("None\n");

    // Test case 2: No default route
    Rule rules2[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.0.0", 16, 2)};

    printf("\nTest Case 2 (No default route):\n");
    print_rules(rules2, 2);
    default_rule = compute_default(rules2, 2, 0);
    printf("Default rule: ");
    if (default_rule)
        print_rule(default_rule);
    else
        printf("None\n");

    // Test case 3: Default appears after specific routes
    Rule rules3[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.0.0", 16, 2),
        make_rule("0.0.0.0", 0, 3) // Default route at end
    };

    printf("\nTest Case 3 (Default at end):\n");
    print_rules(rules3, 3);
    default_rule = compute_default(rules3, 3, 0);
    printf("Default rule: ");
    if (default_rule)
        print_rule(default_rule);
    else
        printf("None\n");
}

// Test function for prefix_match
void test_prefix_match() {
    printf("\n=== Testing prefix_match ===\n");

    Rule rule = make_rule("192.168.1.0", 24, 1);
    printf("Test rule: ");
    print_rule(&rule);

    struct {
        const char *ip;
        bool expected;
    } test_cases[] = {
        {"192.168.1.1", true},
        {"192.168.1.255", true},
        {"192.168.0.1", false},
        {"192.168.2.1", false},
        {"10.0.0.1", false}};

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        uint32_t ip = 0;
        unsigned int a, b, c, d;
        sscanf(test_cases[i].ip, "%u.%u.%u.%u", &a, &b, &c, &d);
        ip = (a << 24) | (b << 16) | (c << 8) | d;

        bool result = prefix_match(&rule, ip);
        printf("IP %s - Expected: %s, Got: %s - %s\n",
               test_cases[i].ip,
               test_cases[i].expected ? "true" : "false",
               result ? "true" : "false",
               result == test_cases[i].expected ? "PASS" : "FAIL");
    }
}

// Test function for extract_bits
void test_extract_bits() {
    printf("\n=== Testing extract_bits ===\n");

    uint32_t test_value = 0xABCDEF12; // Binary: 10101011 11001101 11101111 00010010

    struct {
        uint8_t start;
        uint8_t n_bits;
        uint32_t expected;
    } test_cases[] = {
        {0, 4, 0x2},        // Last 4 bits
        {4, 8, 0xF1},       // Next byte
        {16, 8, 0xCD},      // Third byte
        {24, 8, 0xAB},      // First byte
        {8, 12, 0xDEF},     // Middle 12 bits
        {5, 3, 0x0},        // 3 bits starting at 5
        {0, 32, 0xABCDEF12} // All bits
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        uint32_t result = extract_bits(test_value, test_cases[i].start, test_cases[i].n_bits);
        printf("Extract %u bits from position %u - Expected: 0x%X, Got: 0x%X - %s\n",
               test_cases[i].n_bits,
               test_cases[i].start,
               test_cases[i].expected,
               result,
               result == test_cases[i].expected ? "PASS" : "FAIL");
    }
}

// Test function for create_trie
void test_create_trie() {
    printf("\n=== Testing create_trie ===\n");

    // Test case 1: Simple trie with 3 rules
    Rule rules1[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.2.0", 24, 2),
        make_rule("192.168.3.0", 24, 3) // Default route
    };

    printf("\nTest Case 1: Simple trie with 3 rules\n");
    printf("Input rules:\n");
    print_rules(rules1, sizeof(rules1) / sizeof(rules1[0]));

    // Sort rules first (create_trie expects sorted rules)
    Rule *sorted_rules = sort_rules(rules1, sizeof(rules1) / sizeof(rules1[0]));

    printf("\nSorted rules:\n");
    print_rules(sorted_rules, sizeof(rules1) / sizeof(rules1[0]));
    free(sorted_rules);

    TrieNode *trie = create_trie(sorted_rules, sizeof(rules1) / sizeof(rules1[0]));
    if (trie == NULL) {
        printf("FAIL: Trie creation returned NULL\n");
        free(sorted_rules);
        return;
    }

    printf("PASS: Trie created successfully\n");

    // Basic verification
    printf("Trie root node properties:\n");
    printf(" - Skip: %u\n", trie->skip);
    printf(" - Branch: %u\n", trie->branch);
    printf(" - Pointer: %p\n", (void *)trie->pointer);

    // Test case 2: Empty trie
    printf("\nTest Case 2: Empty trie\n");
    TrieNode *empty_trie = create_trie(NULL, 0);
    if (empty_trie == NULL) {
        printf("PASS: NULL returned for empty rules (expected behavior)\n");
    }
    else {
        printf("FAIL: Expected NULL for empty rules\n");
        // Liberar solo si create_trie no devolvió NULL
        free(empty_trie);
    }
}

int main() {
    printf("=== LC-Trie Function Test Suite ===\n");

    // Run all test functions
    test_compute_skip();
    test_compute_branch();
    test_sort_rules();
    test_compute_default();
    test_prefix_match();
    test_extract_bits();
    test_create_trie();

    printf("\nAll tests completed.\n");
    return 0;
}

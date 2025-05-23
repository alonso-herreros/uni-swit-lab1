#include "../src/lc_trie.h"
#include "../src/utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

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

ip_addr_t str_to_ip(const char *ip_str);

Rule make_rule(const char *ip, uint8_t len, int iface);
void print_rules(const Rule *rules, size_t count);
void print_rule(const Rule *rule);
int eq_rules(const Rule *a, const Rule *b);
TrieNode *create_leaf(uint32_t prefix, uint8_t len, uint32_t iface);
TrieNode *create_internal(uint8_t branch, uint8_t skip, TrieNode **children);

void print_node(const TrieNode *node);
int eq_nodes(const TrieNode *a, const TrieNode *b);

void print_bits(uint32_t value);
void sprint_bits(char *dest, int value, int bits, int nibble_offset);
void print_trie(const TrieNode *trie, char *tree_prefix, char *match_prefix,
        int pre_skip);

int eq_tries(const TrieNode *a, const TrieNode *b);
TrieNode *build_test_trie();
TrieNode *build_test_trie2();


// ==== Test functions ====

// =============================================================== //
// Utils tests                                                     //
// =============================================================== //

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

// =============================================================== //
// LC-Trie tests                                                   //
// =============================================================== //

// Test wrapper for compute_skip
int _test_compute_skip(Rule *rules, size_t num_rules, uint8_t pre_skip,
        int expected) {
    print_rules(rules, num_rules);
    uint8_t skip = compute_skip(rules, num_rules, pre_skip);
    printf("Computed skip, skipping %u: %u bits (expected %u)\n",
        pre_skip, skip, expected);
    if (skip != expected)
        TEST_FAIL("Expected skip: %u, got: %u\n", expected, skip);
    else
        return 0;
}

// Test collection for compute_skip
int test_compute_skip() {
    printf("\n=== Testing compute_skip ===\n");
    int fails = 0;

    // Test case 1
    printf("\n--- Test Case 1 (Common 22 bits) ---\n");

    Rule rules1[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.2.0", 24, 2),
        make_rule("192.168.3.0", 24, 3)
    };
    fails += _test_compute_skip(rules1, 3, 0, 22);

    // Test case 2
    printf("\n--- Test Case 2 (Different first bit) ---\n");
    Rule rules2[] = {
        make_rule("10.0.0.0", 8, 1),
        make_rule("192.168.0.0", 16, 2)
    };
    fails += _test_compute_skip(rules2, 2, 0, 0);

    // Test case 3
    printf("\n--- Test Case 3 (Single rule) ---\n");
    fails += _test_compute_skip(rules1, 1, 0, 24);

    TEST_REPORT("compute_skip", fails);

    return fails;
}


// Test wrapper for compute_branch
int _test_compute_branch(Rule *rules, size_t num_rules,
        uint8_t pre_skip, int expected) {
    print_rules(rules, num_rules);
    uint8_t branch = compute_branch(rules, num_rules, pre_skip);
    printf("Computed branch skipping %u: %u bits (expected %u)\n",
        pre_skip, branch, expected);
    if (branch != expected)
        TEST_FAIL("Expected branch: %u, got: %u\n", expected, branch);
    else
        return 0;
}

// Test function for compute_branch
int test_compute_branch() {
    printf("\n=== Testing compute_branch ===\n");
    int fails = 0;

    // Test case 1
    printf("\n--- Test Case 1 (4 rules with different 3rd octet) ---\n");
    Rule rules1[] = {
        make_rule("192.168.0.0", 24, 1),
        make_rule("192.168.1.0", 24, 2),
        make_rule("192.168.2.0", 24, 3),
        make_rule("192.168.3.0", 24, 4)};
    fails += _test_compute_branch(rules1, 4, 22, 2);

    // Test case 2
    printf("\n--- Test Case 2 (Single rule) ---\n");
    fails += _test_compute_branch(rules1, 1, 0, 0);

    // Test case 3
    printf("\n--- Test Case 3 (2 rules with different MSB in 3rd octet) ---\n");
    Rule rules3[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.129.0", 24, 2)};
    fails += _test_compute_branch(rules3, 2, 16, 1);

    TEST_REPORT("compute_skip", fails);

    return fails;
}

// Wrapper function for sort_rules
int _test_sort_rules(Rule *rules, size_t num_rules, Rule *expected) {
    printf("Original rules:\n");
    print_rules(rules, num_rules);

    Rule *sorted = sort_rules(rules, num_rules);
    printf("Sorted rules:\n");
    print_rules(sorted, num_rules);

    for (size_t i = 0; i < num_rules; i++) {
        if (!eq_rules(&sorted[i], &expected[i])) {
            TEST_FAIL("Wrong rule ordering\n");
        }
    }

    free(sorted);

    return 0;
}

// Test function for sort_rules
int test_sort_rules() {
    printf("\n=== Testing sort_rules ===\n");
    int fails = 0;

    // Test case 1
    printf("\n--- Test Case 1 (Mixed prefixes with default route) ---\n");
    Rule test1[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("0.0.0.0", 0, 2), // Default route
        make_rule("10.0.0.0", 8, 3),
        make_rule("192.168.0.0", 16, 4),
        make_rule("192.168.0.0", 24, 5), // Same network, longer prefix
        make_rule("10.0.0.0", 16, 6)     // Same network, longer prefix
    };
    Rule sorted1[] = {
        make_rule("0.0.0.0", 0, 2),      // Default route
        make_rule("10.0.0.0", 8, 3),
        make_rule("10.0.0.0", 16, 6),    // Same network, longer prefix
        make_rule("192.168.0.0", 16, 4),
        make_rule("192.168.0.0", 24, 5), // Same network, longer prefix
        make_rule("192.168.1.0", 24, 1)
    };
    fails += _test_sort_rules(test1, sizeof(test1) / sizeof(test1[0]), sorted1);

    // Test case 2
    printf("\n--- Test Case 2 (Same network, different prefix lengths) ---\n");
    Rule test2[] = {
        make_rule("192.168.1.0", 28, 1),
        make_rule("192.168.1.0", 24, 2),
        make_rule("192.168.1.0", 16, 3),
        make_rule("192.168.1.0", 32, 4)
    };
    Rule sorted2[] = {
        make_rule("192.168.1.0", 16, 3),
        make_rule("192.168.1.0", 24, 2),
        make_rule("192.168.1.0", 28, 1),
        make_rule("192.168.1.0", 32, 4)
    };
    fails += _test_sort_rules(test2, sizeof(test2) / sizeof(test2[0]), sorted2);

    // Test case 3 (how did we get here?)
    printf("\n--- Test Case 3 (Multiple default routes) ---\n");
    Rule test3[] = {
        make_rule("0.0.0.0", 0, 2),
        make_rule("0.0.0.0", 0, 1), // Same default, different interface
        make_rule("10.0.0.0", 8, 3)
    };
    Rule sorted3[] = {
        make_rule("0.0.0.0", 0, 2),
        make_rule("0.0.0.0", 0, 1), // Same default, different interface
        make_rule("10.0.0.0", 8, 3)
    };
    fails += _test_sort_rules(test3, sizeof(test3) / sizeof(test3[0]), sorted3);

    TEST_REPORT("sort_rules", fails);

    return fails;
}

// Wrapper function for compute_default
int _test_compute_default(Rule *rules, size_t num_rules,
        uint8_t pre_skip, Rule *expected) {
    printf("Input rules:\n");
    print_rules(rules, num_rules);

    Rule *default_rule = compute_default(rules, num_rules, pre_skip);
    printf("Default rule: ");
    if (default_rule)  print_rule(default_rule);
    else  printf("None\n");

    if (default_rule != expected && !eq_rules(default_rule, expected)) {
        TEST_FAIL("Wrong default rule\n");
    }

    return 0;
}

// Test collection for compute_default
int test_compute_default() {
    printf("\n=== Testing compute_default ===\n");
    int fails = 0;

    // Test case 1
    printf("\n--- Test Case 1 (All-encompassing default) ---\n");
    Rule rules1[] = {
        make_rule("0.0.0.0", 0, 1), // Default route
        make_rule("192.168.0.0", 24, 3),
        make_rule("192.168.1.0", 24, 2),
    };
    fails += _test_compute_default(rules1, 3, 0, &(rules1[0]));

    // Test case 2
    printf("\n--- Test Case 2 (Same IP, different length) ---\n");
    Rule rules2[] = {
        make_rule("192.168.0.0", 16, 2),
        make_rule("192.168.0.0", 24, 1),
    };
    fails += _test_compute_default(rules2, 2, 0, &(rules2[1]));

    // Test case 3
    printf("\n--- Test Case 3 (Linear group) ---\n");
    Rule rules3[] = {
        make_rule("192.168.0.0", 16, 2),
        make_rule("192.168.128.0", 20, 1),
        make_rule("192.168.129.0", 24, 1),
    };
    fails += _test_compute_default(rules3, 3, 0, &(rules3[2]));

    // Test case 4
    printf("\n--- Test Case 4 (Inner default) ---\n");
    Rule rules4[] = {
        make_rule("0.0.0.0", 0, 3),
        make_rule("192.168.0.0", 16, 2),
        make_rule("192.168.0.0", 24, 1),
        make_rule("192.168.1.0", 24, 1),
    };
    fails += _test_compute_default(rules4, 4, 0, &(rules4[1]));

    // Test case 5
    printf("\n--- Test Case 5 (No default) ---\n");
    Rule rules5[] = {
        make_rule("192.168.0.0", 24, 1),
        make_rule("192.168.1.0", 24, 1),
    };
    fails += _test_compute_default(rules5, 2, 0, NULL);

    TEST_REPORT("compute_default", fails);

    return fails;
}

// Wrapper function for rule_match
int _test_rule_match(Rule *rule, const char *ip_str, bool expected) {
    printf("Testing '%s' against rule: ", ip_str);
    print_rule(rule);

    ip_addr_t ip = 0;
    unsigned int a, b, c, d;
    sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    ip = (a << 24) | (b << 16) | (c << 8) | d;

    bool match = rule_match(rule, ip);
    printf("Match: %s (expected: %s)\n",
        match ? "true" : "false",
        expected ? "true" : "false");

    if (match != expected) {
        TEST_FAIL("\n");
    }

    return 0;
}

// Test collection for rule_match
int test_rule_match() {
    printf("\n=== Testing rule_match ===\n");
    int fails = 0;

    Rule rule = make_rule("192.168.1.0", 24, 1);

    struct {
        const char *ip;
        bool expected;
    } cases[] = {
        {"192.168.1.1", true},
        {"192.168.1.255", true},
        {"192.168.0.1", false},
        {"192.168.2.1", false},
        {"10.0.0.1", false}};

    for (size_t i = 0; i < 5; i++) {
        printf("\n--- Test Case %zu ---\n", i + 1);
        fails += _test_rule_match(&rule, cases[i].ip, cases[i].expected);
    }

    TEST_REPORT("rule_match", fails);

    return fails;
}


// Auxiliary function to automate node comparison
int _inspect_node(TrieNode *node, TrieNode *expected) {
    printf("Node %p: ", (void *)node);
    print_node(node);
    printf("Expected:            ");
    print_node(expected);

    if (!eq_nodes(node, expected)) {
        TEST_FAIL("Nodes are NOT equal\n");
    }

    return 0;
}

// Wrapper function to test create_trie
int _test_create_trie(Rule *rules, size_t num_rules, TrieNode *expected_root) {
    printf("Input rules:\n");
    print_rules(rules, num_rules);

    TrieNode *trie = create_trie(rules, num_rules);
    printf("Trie created:\n");
    print_trie(trie, NULL, NULL, 0);

    printf("Expected:\n");
    print_trie(expected_root, NULL, NULL, 0);

    if (!eq_tries(trie, expected_root)) {
        free_trie(trie);
        TEST_FAIL("Tries are not equal\n");
    }

    if (trie == NULL && expected_root != NULL) {
        free_trie(trie);
        TEST_FAIL("Trie creation failed\n");
    } else if (expected_root == NULL && trie != NULL) {
        free_trie(trie);
        TEST_FAIL("Expected NULL trie, returned %p\n", trie);
    }

    free_trie(trie);

    return 0;
}

// Test collection for create_trie
int test_create_trie() {
    printf("\n=== Testing create_trie ===\n");
    int fails = 0;

    // Test case 1
    printf("\n--- Test Case 1: Simple trie with 3 rules ---\n");
    Rule rules1[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.2.0", 24, 2),
        make_rule("192.168.3.0", 24, 3) // Default route
    };
    size_t nrules1 = sizeof(rules1) / sizeof(rules1[0]);

    // Manual construction of the expected trie
    TrieNode *root = calloc(1, sizeof(TrieNode));
    *root = (TrieNode){.skip = 22, .branch = 1};

    // * (s22 b1)
    TrieNode *children = calloc(2, sizeof(TrieNode));
    root->pointer = children;
    children[0] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules1[0]};
    children[1] = (TrieNode){.skip = 0, .branch = 1}; // ?{22} 1*

    // ???? ???? ???? ???? ???? ??1* (s0 b1)
    TrieNode *children1 = calloc(2, sizeof(TrieNode));
    children[1].pointer = children1;
    children1[0] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules1[1]};
    children1[1] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules1[2]};

    fails += _test_create_trie(rules1, nrules1, root);

    // Test case 2
    printf("\n--- Test Case 2: Empty trie ---\n");
    fails += _test_create_trie(NULL, 0, NULL);

    // Test case 3
    printf("\n--- Test Case 3: Single rule ---\n");
    Rule rules3[] = {
        make_rule("0.0.0.0", 0, 1),
    };
    size_t nrules3 = sizeof(rules3) / sizeof(rules3[0]);

    // Manual construction of the expected trie
    TrieNode *root3 = calloc(1, sizeof(TrieNode));
    *root3 = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules3[0]};

    fails += _test_create_trie(rules3, nrules3, root3);

    // Test case 4
    printf("\n--- Test Case 4: Complex trie ---\n");
    Rule rules[] = { // Same rules as in build_test_trie2
        make_rule("0.0.0.0",     0,  1),   // Default route
        make_rule("0.1.0.0",     16, 2),
        make_rule("10.0.0.0",    8,  3),
        make_rule("10.0.0.0",    16, 10),
        make_rule("10.1.0.0",    16, 11),
        make_rule("10.2.0.0",    16, 12),
        make_rule("10.4.0.0",    16, 14),
        make_rule("10.5.0.0",    16, 15),
        make_rule("10.6.0.0",    16, 16),
        make_rule("10.7.0.0",    16, 17),
        make_rule("172.16.0.0",  12, 5),
        make_rule("172.20.0.0",  16, 20),
        make_rule("172.21.0.0",  16, 21),
        make_rule("172.22.0.0",  16, 22),
        make_rule("172.23.0.0",  16, 23),
        make_rule("192.168.1.0", 24, 101),
    };
    size_t nrules4 = sizeof(rules) / sizeof(rules[0]);

    TrieNode *root4 = build_test_trie2();
    fails += _test_create_trie(rules, nrules4, root4);
    free(root4);

    if (FILL_FACTOR > 0.875) {
        printf("^!! WARNING: test required FILL_FACTOR > 0.875, but it's %f\n"
               "    Try compiling with -DFILL_FACTOR=0.875 or lower\n",
                1.0*FILL_FACTOR);
    }

    TEST_REPORT("create_trie", fails);

    return fails;
}


// Wrapper function for testing count_nodes_trie
int _test_count_nodes(TrieNode *trie, uint32_t expected) {
    printf("Counting nodes for trie:\n");
    print_trie(trie, NULL, NULL, 0);
    uint32_t count = count_nodes_trie(trie);
    printf("Counted nodes: %u (expected: %u)\n", count, expected);
    if (count != expected) {
        TEST_FAIL("Expected node count %u, got %u\n", expected, count);
    }
    return 0;
}

// Test function for count_nodes_trie
int test_count_nodes() {
    printf("\n=== Testing count_nodes_trie ===\n");
    int fails = 0;

    // printf("\n--- Test Case 1: Manually built trie ---\n");
    // // Creamos un pequeño trie manualmente
    // TrieNode *leaf1 = create_leaf(0x64400000, 10, 1);  // 100.64.0.0/10
    // TrieNode *leaf2 = create_leaf(0x64800000, 9, 2);   // 100.128.0.0/9
    // TrieNode *leaf3 = create_leaf(0x64000000, 8, 3);   // 100.0.0.0/8
    // TrieNode *leaf5 = create_leaf(0xC0000000, 8, 4);   // 192.0.0.0/8

    // TrieNode *children2[2] = {leaf2, leaf3};
    // TrieNode *internal2 = create_internal(1, 2, children2);

    // TrieNode *children1[2] = {leaf1, internal2};
    // TrieNode *internal1 = create_internal(1, 4, children1);

    // TrieNode *root_children[2] = {internal1, leaf5};
    // TrieNode *root = create_internal(1, 0, root_children);

    // // Test the case
    // fails += _test_count_nodes(root, 7);
    // free(root);

    // Test case 2
    printf("\n--- Test Case 2: test_build_trie ---\n");
    TrieNode *trie2 = build_test_trie();
    fails += _test_count_nodes(trie2, 7);

    // Test case 3
    printf("\n--- Test Case 3: test_build_trie2 ---\n");
    TrieNode *trie3 = build_test_trie2();
    fails += _test_count_nodes(trie3, 19);

    TEST_REPORT("count_nodes_trie", fails);

    return fails;
}


// Test wrapper for lookup
int _test_lookup(ip_addr_t ip, TrieNode *trie, int expected) {
    int access_count = 0;
    int result = lookup_ip(ip, trie, &access_count);

    char ip_str[16];
    sprintf(ip_str, "%u.%u.%u.%u",
            (ip >> 24) & 0xFF, (ip >> 16) & 0xFF,
            (ip >> 8) & 0xFF, ip & 0xFF);
    printf("IP: %-15s -> Result: %d (Expected: %d) in %d accesses\n",
            ip_str, result, expected, access_count);

    if (result != expected) {
        TEST_FAIL("Wrong match\n");
    }

    return 0;
}

// Test collection for lookups
int test_lookup() {
    printf("\n=== Testing lookup ===\n");
    int fails = 0;

    // Build test trie
    TrieNode *trie = build_test_trie();
    printf("\n-== Testing with Trie 1 ==-\n");
    print_trie(trie, NULL, NULL, 0);

    // Test cases
    struct {
        uint32_t ip;
        int expected;
        const char *description;
    } tests[] = {
        {0x0AC86432, 2, "10.200.100.50"},    // 10.200.100.50
        {0xAC100A0A, 3, "172.16.10.10"},    // 172.16.10.10
        {0xC0A8010A, 1, "192.168.1.10"},    // 192.168.1.10
        {0x0AFFFFFF, 2, "10.255.255.255"},    // 10.255.255.255
        {0xDFFFFFFF, 0, "223.255.255.255"},    // 223.255.255.255
        {0x00000101, 0, "0.0.1.1"},    // 0.0.1.1
        {0xAC3FFF00, 4, "172.63.255.255"},    // 172.63.255.255
        {0xC0323232, 0, "192.50.50.50 Out of range (Below the range)"},    // 192.50.50.50 /FAILS
        {0xFFFFFF00, 0, "255.255.255.0"},    // 255.255.255.0
        {0xAC100001, 3, "172.16.0.1"},    // 172.16.0.1
        {0xAC20000A, 4, "172.32.0.10"},    // 172.32.0.10
        {0xC0A83232, 1, "192.168.50.50"},    // 192.50.50.50 /FAILS
        {0xC0A8010A, 1, "192.168.1.10"},    // 192.168.1.10
        {0xFFFF0000, 0, "255.255.0.0"},   // 255.255.0.0
        // Tests for 192.168.0.0/16 (port 1)
        {0xC0A80000, 1, "Lower bound 192.168.0.0"},
        {0xC0A80101, 1, "Typical IP 192.168.1.1"},
        {0xC0A8FFFF, 1, "Upper bound 192.168.255.255"},
        {0xC0A7FFFF, 0, "Out of range 192.167.255.255"},
        {0xC0A90000, 0, "Out of range 192.169.0.0"},
        // Tests for 10.0.0.0/8 (port 2)
        {0x0A000000, 2, "Lower bound 10.0.0.0"},
        {0x0A010203, 2, "Typical IP 10.1.2.3"},
        {0x0AFFFFFF, 2, "Upper bound 10.255.255.255"},
        {0x09000000, 0, "Out of range 9.0.0.0"},
        {0x0B000000, 0, "Out of range 11.0.0.0"},
        // Tests for 172.16.0.0/12 (port 3)
        {0xAC100000, 3, "Lower bound 172.16.0.0"},
        {0xAC101234, 3, "Typical IP 172.16.18.52"},
        {0xAC1FFFFF, 3, "Upper bound 172.31.255.255"},
        {0xAC0FFFFF, 0, "Out of range 172.15.255.255"},
        {0xAC200000, 4, "Out of range (belongs to 172.32.0.0/11)"},
        // Tests for 172.32.0.0/11 (port 4)
        {0xAC200000, 4, "Lower bound 172.32.0.0"},
        {0xAC3F1234, 4, "Typical IP 172.63.18.52"},
        {0xAC3FFFFF, 4, "Upper bound 172.63.255.255"},
        {0xAC400000, 0, "Out of range 172.64.0.0"},
        // Special cases
        {0x7F000001, 1, "Loopback 127.0.0.1"},
        {0x00000000, 0, "Zero address"},
        {0xFFFFFFFF, 0, "Broadcast address"},
        {0x0AFFFFFF, 2, "Max IP in 10.0.0.0/8"},
        {0xC0A8FFFF, 1, "Max IP in 192.168.0.0/16"},
        // Boundary transition cases
        {0xAC1FFFFF, 3, "Upper edge 172.31.255.255 (belongs to /12)"},
        {0xAC200000, 4, "Lower edge 172.32.0.0 (belongs to /11)"},
        {0x0AFFFFFF, 2, "Upper edge 10.255.255.255"},
        {0x0B000000, 0, "Lower edge out of range 11.0.0.0"},
        // Random IPs for coverage
        {0x45A3D2F1, 1, "Random IP 69.163.210.241"},
        {0xDEADBEEF, 0, "Special pattern IP 222.173.190.239"}, // Nice
        {0x12345678, 0, "Special pattern IP 18.52.86.120"}
    };

    for (int i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        // printf("\n--- Test Case %d: %s ---\n", i+1, tests[i].description);
        fails += _test_lookup(tests[i].ip, trie, tests[i].expected);
    }

    // Test case 2
    printf("\n-== Testing with Trie 2 ==-\n");
    TrieNode *trie2 = build_test_trie2();
    print_trie(trie2, NULL, NULL, 0);

    struct {
        uint32_t ip;
        int expected;
        const char *comment;
    } tests2[] = {
        // Default route fallback (requires backtracking)
        {str_to_ip("0.0.0.1"),          1, "(Requires backtracking)"},
        {str_to_ip("255.255.255.255"),  1, "(Requires backtracking)"},
        // 0.1.0.0/16
        {str_to_ip("0.1.0.0"),          2},
        // 10.0.0.0/8 and subnets
        {str_to_ip("10.0.0.0"),        10},
        {str_to_ip("10.1.0.0"),        11},
        {str_to_ip("10.3.0.0"),         3},
        {str_to_ip("10.7.0.0"),        17},
        {str_to_ip("10.10.0.0"),        3, "(Requires backtracking)"},
        // 172.16.0.0/12 and subnets
        {str_to_ip("172.16.0.1"),       5, "(Requires backtracking)"},
        {str_to_ip("172.20.255.255"),  20},
        {str_to_ip("172.21.0.0"),      21},
        {str_to_ip("172.23.0.0"),      23},
        {str_to_ip("172.23.0.2"),      23},
        // 192.168.1.0/24
        {str_to_ip("192.168.1.50"),   101},
        // False matches
        {str_to_ip("1.1.0.1"),          1, "(False match for 2)"},
        {str_to_ip("193.168.1.1"),      1, "(False match for 101)"},
        // None
        {str_to_ip("192.168.0.50"),     1, "(Requires backtracking)"},
        {str_to_ip("222.173.190.329"),  1}, // 0xDEADBEEF
        {str_to_ip("18.52.86.120"),     1}, // 0x12345678
    };

    for (int i = 0; i < sizeof(tests2) / sizeof(tests2[0]); i++) {
        const char *hint = tests2[i].comment ? tests2[i].comment : "";
        printf("\n--- Test Case %d %s ---\n", i+1, hint);
        fails += _test_lookup(tests2[i].ip, trie2, tests2[i].expected);
    }

    TEST_REPORT("lookup", fails);

    free_trie(trie);
    free_trie(trie2);

    return fails;
}


// ==== Helper functions ====

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

int eq_rules(const Rule *a, const Rule *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }
    return (a->prefix == b->prefix &&
            a->prefix_len == b->prefix_len &&
            a->out_iface == b->out_iface);
}

void print_node(const TrieNode *node) {
    printf("skip=%u, branch=%u, pointer=%4p",
           node->skip, node->branch, (void *)node->pointer);

    if (node->branch == 0) {
        printf(" to rule ");
        if (node->pointer != NULL) {
            print_rule((Rule *)node->pointer);
        }
        else {
            printf("\n");
        }
    }
    else {
        printf(" to child\n");
    }
}

// Function to compare two TrieNode structures
// If the second node's `pointer` is NULL, no `pointer` comparison is done
int eq_nodes(const TrieNode *a, const TrieNode *b) {
    return (a->skip == b->skip &&
            a->branch == b->branch &&
            (b->pointer == NULL || a->pointer == b->pointer));
}

// Print the structure of a trie
void print_trie(const TrieNode *trie, char *tree_prefix, char *match_prefix,
        int pre_skip) {
    const static size_t STRING_LENGTH = 321;

    if (trie==NULL) {
        printf("<NULL TRIE>\n");
        return;
    }

    bool root = 0; // This allows us to skip indenting the whole trie
    if (tree_prefix == NULL || match_prefix == NULL) {
        tree_prefix = match_prefix = ""; // Initialize both
        root = 1;
    }

    int skip = trie->skip;
    int branch = trie->branch;

    // Print the current node's 'path'
    printf("%s%s%s* (s%d b%d)", tree_prefix, root?"":"|-", match_prefix,
            skip, branch);

    if (branch == 0) { // Print the associated rule
        if (trie->pointer != NULL) {
            printf(": ");
            print_rule((Rule *)trie->pointer);
        } else {
            printf("::\n");
        }
        return;
    } else { // Print a line terminator
        printf(";\n");
    }

    // Recursion adding prefixes
    char new_tree_prefix[STRING_LENGTH];
    sprintf(new_tree_prefix, "%s%s", tree_prefix, root?"":"| ");

    char new_match_prefix[STRING_LENGTH];
    memcpy(new_match_prefix, match_prefix, STRING_LENGTH);
    sprint_bits(new_match_prefix, -1, skip, pre_skip);

    int max_children = 1 << branch;
    for (int i = 0; i < max_children; i++) {
        char child_match_prefix[STRING_LENGTH];
        memcpy(child_match_prefix, new_match_prefix, STRING_LENGTH);
        sprint_bits(child_match_prefix, i, branch, pre_skip+skip);

        TrieNode *child = (TrieNode *)trie->pointer + i;
        print_trie(child, new_tree_prefix, child_match_prefix,
                pre_skip+skip+branch);
    }
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

void sprint_bits(char *dest, int value, int bits, int nibble_offset) {
    /* memcpy(dest, "", 1); */
    for (int i=0; i < bits; i++) {
        int bit_pos = i + nibble_offset;
        if (bit_pos != 0 && bit_pos % 4 == 0) {
            sprintf(dest, "%s ", dest);
        }
        if (value < 0) { // Interpret value < 0 as printing '?'s
            sprintf(dest, "%s?", dest);
        } else {
            sprintf(dest, "%s%d", dest, (value >> (bits-i-1)) & 1);
        }
    }
}

ip_addr_t str_to_ip(const char *ip_str) {
    unsigned int a, b, c, d;
    sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    return (a << 24) | (b << 16) | (c << 8) | d;
}

// Helper function to create a rule
Rule make_rule(const char *ip, uint8_t len, int iface) {
    return (Rule){
        .prefix = str_to_ip(ip),
        .prefix_len = len,
        .out_iface = iface
    };
}

// Helper function to compare two tries
int eq_tries(const TrieNode *a, const TrieNode *b) {
    if (a == NULL && b == NULL) return 1;
    if (a == NULL || b == NULL) return 0;

    if (a->skip != b->skip || a->branch != b->branch) {
        return 0;
    }

    if (a->branch == 0) { // Implies b->branch == 0
        return eq_rules((Rule *)a->pointer, (Rule *)b->pointer);
    }

    int max_children = 1 << a->branch;
    for (int i = 0; i < max_children; i++) {
        TrieNode *child_a = (TrieNode *)a->pointer + i;
        TrieNode *child_b = (TrieNode *)b->pointer + i;
        if (!eq_tries(child_a, child_b)) return 0;
    }
    return 1;
}


/** Builds a test LC-Trie structure with sample routing entries
 *
 * @note The trie does not have a 100% fill factor, that's why
 *      the default route is not at the root.
 *
 * @return Pointer to the root of the constructed trie
 * * (s0 b2);
 * |-00* (s0 b0): 10.0.0.0/8 -> iface 2;
 * |-01* (s0 b0): 0.0.0.0/0 -> iface 1;
 * |-10* (s8 b1);
 * | |-10?? ???? ??0* (s0 b0): 172.16.0.0/12 -> iface 3;
 * | \-10?? ???? ??1* (s0 b0): 172.32.0.0/11 -> iface 4;
 * \-11* (s0 b0): 192.168.0.0/16 -> iface 1;
 */
TrieNode *build_test_trie() {

    Rule rules_local[] = {
        make_rule("0.0.0.0", 0, 1),
        make_rule("192.168.0.0", 16, 1),
        make_rule("10.0.0.0", 8, 2),
        make_rule("172.16.0.0", 12, 3),
        make_rule("172.32.0.0", 11, 4),
    };
    // Allocation required because initialized rules_local is on the stack
    Rule *rules = malloc(sizeof(rules_local));
    memcpy(rules, rules_local, sizeof(rules_local));

    // Level 2 nodes (for 172.x.x.x routes)
    TrieNode *level2 = calloc(2, sizeof(TrieNode));
    level2[0] = (TrieNode){.branch = 0, .pointer = &rules[3]}; // 172.16.0.0/12
    level2[1] = (TrieNode){.branch = 0, .pointer = &rules[4]}; // 172.32.0.0/11

    // Level 1 nodes (first 2 bits)
    TrieNode *level1 = calloc(4, sizeof(TrieNode));
    level1[0] = (TrieNode){.branch = 0, .pointer = &rules[2]}; // 10.0.0.0/8
    level1[1] = (TrieNode){.branch = 0, .pointer = &rules[0]}; // 0.0.0.0/0
    level1[2] = (TrieNode){.branch = 1, .skip = 8, .pointer = level2};
    level1[3] = (TrieNode){.branch = 0, .pointer = &rules[1]}; // 192.168.0.0/16

    // Root node
    TrieNode *root = calloc(1, sizeof(TrieNode));
    root->branch = 2;
    root->skip = 0;
    root->pointer = level1;

    return root;
}

/** Builds a test LC-Trie meant to test fallbacks and backtracking
 *
 * @return pointer to the root of the constructed trie
 *
 * * (s0 b1): 0.0.0.0/0 -> iface 1;
 * |-0* (s3 b1);
 * | |-0??? 0* (s0 b0): 0.1.0.0/16 -> iface 2;
 * | \-0??? 1* (s8 b3): 10.0.0.0/8 -> iface 3;
 * | . |-0??? 1??? ???? ?000 (s0 b0): 10.0.0.0/16 -> iface 10;
 * | . |-0??? 1??? ???? ?001 (s0 b0): 10.1.0.0/16 -> iface 11;
 * | . |-0??? 1??? ???? ?010 (s0 b0): 10.2.0.0/16 -> iface 12;
 * | . |-0??? 1??? ???? ?011 (s0 b0)::
 * | . |-0??? 1??? ???? ?100 (s0 b0): 10.4.0.0/16 -> iface 14;
 * | . |-0??? 1??? ???? ?101 (s0 b0): 10.5.0.0/16 -> iface 15;
 * | . |-0??? 1??? ???? ?110 (s0 b0): 10.6.0.0/16 -> iface 16;
 * | . \-0??? 1??? ???? ?111 (s0 b0): 10.7.0.0/16 -> iface 17;
 * | .
 * \-1* (s0 b1);
 * . |-10* (s12 b2);
 * . | | ~ 172.16.0.0/12 -> iface 5;
 * . | |-10?? ???? ???? ??00: 172.20.0.0/16 -> iface 20;
 * . | |-10?? ???? ???? ??01: 172.21.0.0/16 -> iface 21;
 * . | |-10?? ???? ???? ??10: 172.22.0.0/16 -> iface 22;
 * . | \-10?? ???? ???? ??11: 172.23.0.0/16 -> iface 23;
 * . |
 * . \-11* (s0 b0): 192.168.1.0/24 -> iface 101;
 */
TrieNode *build_test_trie2() {

    Rule rules_local[] = {
        make_rule("0.0.0.0",     0,  1),   // Default route
        make_rule("0.1.0.0",     16, 2),
        make_rule("10.0.0.0",    8,  3),
        make_rule("10.0.0.0",    16, 10),
        make_rule("10.1.0.0",    16, 11),
        make_rule("10.2.0.0",    16, 12),
        make_rule("10.4.0.0",    16, 14),
        make_rule("10.5.0.0",    16, 15),
        make_rule("10.6.0.0",    16, 16),
        make_rule("10.7.0.0",    16, 17),
        make_rule("172.16.0.0",  12, 5),
        make_rule("172.20.0.0",  16, 20),
        make_rule("172.21.0.0",  16, 21),
        make_rule("172.22.0.0",  16, 22),
        make_rule("172.23.0.0",  16, 23),
        make_rule("192.168.1.0", 24, 101),
    };
    // Allocation required because initialized rules_local is on the stack
    Rule *rules = malloc(16*sizeof(Rule));
    memcpy(rules, rules_local, sizeof(rules_local));

    // Setting rule hierarchy
    rules[1].parent = &rules[0];
    rules[2].parent = &rules[0];
    rules[3].parent = &rules[2];
    rules[4].parent = &rules[2];
    rules[5].parent = &rules[2];
    rules[6].parent = &rules[2];
    rules[7].parent = &rules[2];
    rules[8].parent = &rules[2];
    rules[9].parent = &rules[2];
    rules[10].parent = &rules[0];
    rules[11].parent = &rules[10];
    rules[12].parent = &rules[10];
    rules[13].parent = &rules[10];
    rules[14].parent = &rules[10];
    rules[15].parent = &rules[0];


    TrieNode *root = calloc(1, sizeof(TrieNode));
    root[0] = (TrieNode){.skip = 0, .branch = 1};

    // * (s0 b1)
    TrieNode *children = calloc(2, sizeof(TrieNode));
    children[0] = (TrieNode){.skip = 3, .branch = 1}; // 0*
    children[1] = (TrieNode){.skip = 0, .branch = 1}; // 1*
    root[0].pointer = children;

    // 0* (s3 b1)
    TrieNode *children0 = calloc(2, sizeof(TrieNode));
    children0[0] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[1]};
    children0[1] = (TrieNode){.skip = 8, .branch = 3}; // 0??? 1*
    children[0].pointer = children0;

    // 0??? 1* (s8 b3)
    TrieNode *children01 = calloc(8, sizeof(TrieNode));
    children01[0] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[3]};
    children01[1] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[4]};
    children01[2] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[5]};
    children01[3] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[2]};
    children01[4] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[6]};
    children01[5] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[7]};
    children01[6] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[8]};
    children01[7] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[9]};
    children0[1].pointer = children01;

    // 1* (s0 b1)
    TrieNode *children1 = calloc(2, sizeof(TrieNode));
    children1[0] = (TrieNode){.skip = 12, .branch = 2}; // 10*
    children1[1] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[15]};
    children[1].pointer = children1;

    // 10* (s8 b2)
    TrieNode *children10 = calloc(4, sizeof(TrieNode));
    children10[0] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[11]};
    children10[1] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[12]};
    children10[2] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[13]};
    children10[3] = (TrieNode){.skip = 0, .branch = 0, .pointer = &rules[14]};
    children1[0].pointer = children10;

    return root;
}

// Funciones auxiliares (para crear nodos)
TrieNode *create_leaf(uint32_t prefix, uint8_t len, uint32_t iface)
{
    Rule *rule = malloc(sizeof(Rule));
    rule->prefix = prefix;
    rule->prefix_len = len;
    rule->out_iface = iface;

    TrieNode *leaf = malloc(sizeof(TrieNode));
    leaf->branch = 0;
    leaf->skip = 0;
    leaf->pointer = rule;
    return leaf;
}

TrieNode *create_internal(uint8_t branch, uint8_t skip, TrieNode **children)
{
    TrieNode *node = malloc(sizeof(TrieNode));
    node->branch = branch;
    node->skip = skip;

    // Cantidad de hijos esperados
    uint32_t n = 1 << branch;

    // Asignamos el array de punteros a hijos
    TrieNode **child_array = malloc(n * sizeof(TrieNode *));
    for (uint32_t i = 0; i < n; i++)
    {
        child_array[i] = children[i]; // apuntamos directamente, no copiamos
    }

    node->pointer = child_array;
    return node;
}

// ==== Main flow ====

// Run all tests
int main() {
    int fails = 0;

    printf("\n==x=x== Utils Test Suite ==x=x==\n");
    int fails_utils = 0;

    fails_utils += test_extract_lsb();
    fails_utils += test_extract_msb();

    TEST_REPORT("Utils", fails_utils);
    fails += fails_utils;

    printf("\n\n==x=x== LC-Trie Function Test Suite ==x=x==\n");
    int fails_lc_trie = 0;

    fails_lc_trie += test_compute_skip();
    fails_lc_trie += test_compute_branch();
    fails_lc_trie += test_sort_rules();
    fails_lc_trie += test_compute_default();
    fails_lc_trie += test_rule_match();
    fails_lc_trie += test_create_trie();
    fails_lc_trie += test_count_nodes();
    fails_lc_trie += test_lookup();

    TEST_REPORT("LC-Trie", fails_lc_trie);
    fails += fails_lc_trie;

    printf("\n\n=x=x=x= Global report =x=x=x=");
    TEST_REPORT("ALL", fails);
    printf("\n");

    return fails;
}



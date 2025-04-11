#include "../src/lc_trie.h"
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

Rule make_rule(const char *ip, uint8_t len, int iface);
void print_rules(const Rule *rules, size_t count);
void print_rule(const Rule *rule);
int eq_rules(const Rule *a, const Rule *b);

void print_node(const TrieNode *node);
int eq_nodes(const TrieNode *a, const TrieNode *b);

void sprint_bits(char *dest, int value, int bits, int nibble_offset);
void print_trie(const TrieNode *trie, char *tree_prefix, char *match_prefix,
        int pre_skip);

TrieNode *build_test_trie();


// ==== Test functions ====

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
    printf("\n--- Test Case 1 (With default route) ---\n");
    Rule rules1[] = {
        make_rule("0.0.0.0", 0, 1), // Default route
        make_rule("192.168.1.0", 24, 2),
        make_rule("192.168.0.0", 16, 3)
    };

    fails += _test_compute_default(rules1, 3, 0, &(rules1[0]));

    // Test case 2
    printf("\n--- Test Case 2 (No default route) ---\n");
    Rule rules2[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.0.0", 16, 2)};

    fails += _test_compute_default(rules2, 2, 0, NULL);

    // Test case 3
    printf("\n--- Test Case 3 (Default at end) ---\n");
    Rule rules3[] = {
        make_rule("192.168.1.0", 24, 1),
        make_rule("192.168.0.0", 16, 2),
        make_rule("0.0.0.0", 0, 3) // Default route at end
    };

    fails += _test_compute_default(rules3, 3, 0, &(rules3[2]));

    TEST_REPORT("compute_default", fails);

    return fails;
}

// Wrapper function for prefix_match
int _test_prefix_match(Rule *rule, const char *ip_str, bool expected) {
    printf("Testing '%s' against rule: ", ip_str);
    print_rule(rule);

    ip_addr_t ip = 0;
    unsigned int a, b, c, d;
    sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d);
    ip = (a << 24) | (b << 16) | (c << 8) | d;

    bool match = prefix_match(rule, ip);
    printf("Match: %s (expected: %s)\n",
        match ? "true" : "false",
        expected ? "true" : "false");

    if (match != expected) {
        TEST_FAIL("\n");
    }

    return 0;
}

// Test collection for prefix_match
int test_prefix_match() {
    printf("\n=== Testing prefix_match ===\n");
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
        fails += _test_prefix_match(&rule, cases[i].ip, cases[i].expected);
    }

    TEST_REPORT("prefix_match", fails);

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

    printf("Input rules:\n");
    print_rules(rules1, sizeof(rules1) / sizeof(rules1[0]));
    // Sort rules (create_trie currently expects sorted rules)
    Rule *sorted1 = sort_rules(rules1, sizeof(rules1) / sizeof(rules1[0]));
    printf("\nSorted rules:\n");
    print_rules(sorted1, 3);

    TrieNode *trie = create_trie(sorted1, sizeof(rules1) / sizeof(rules1[0]));
    printf("Trie created:\n");
    print_trie(trie, NULL, NULL, 0);
    if (trie == NULL) {
        TEST_FAIL("Trie creation FAILED: returned NULL\n");
    }

    // Inspecting root
    printf("\nInspecting root:\n");
    TrieNode expected_root = {.branch=1, .skip=22, .pointer=NULL};
    fails += _inspect_node(trie, &expected_root);

    // Inspecting first child
    printf("\nInspecting first child:\n");
    TrieNode *child = trie->pointer;
    TrieNode expected_child1 = {.skip=0, .branch=0, .pointer=&sorted1[0]};
    fails += _inspect_node(child, &expected_child1);

    // Inspecting second child
    printf("\nInspecting second child:\n");
    TrieNode *child2 = (TrieNode *) trie->pointer+1;
    TrieNode expected_child2 = {.skip=0, .branch=1, .pointer=NULL};
    fails += _inspect_node(child2, &expected_child2);

    // Inspecting second child's children
    printf("\nInspecting second child's children:\n");
    TrieNode *child21 = (TrieNode *) child2->pointer;
    TrieNode expected_child21 = {.skip=0, .branch=0, .pointer=&sorted1[1]};
    fails += _inspect_node(child21, &expected_child21);
    TrieNode *child22 = (TrieNode *) child2->pointer + 1;
    TrieNode expected_child22 = {.skip=0, .branch=0, .pointer=&sorted1[2]};
    fails += _inspect_node(child22, &expected_child22);

    free(sorted1);
    free_trie(trie);

    // Test case 2
    printf("\n--- Test Case 2: Empty trie ---\n");
    TrieNode *empty_trie = create_trie(NULL, 0);
    print_trie(empty_trie, NULL, NULL, 0);
    if (empty_trie != NULL) {
        printf("! TEST FAIL ! Expected NULL Trie for empty rules\n");
        free(empty_trie);
        fails += 1;
    }

    TEST_REPORT("create_trie", fails);

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
    printf("\n--- Testing with Trie 1 ----\n");
    print_trie(trie, NULL, NULL, 0);

    // Test cases
    struct
    {
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
        {0x7F000001, 0, "Loopback 127.0.0.1"},
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
        {0x45A3D2F1, 0, "Random IP 69.163.210.241"},
        {0xDEADBEEF, 0, "Special pattern IP 222.173.190.239"}, // Nice
        {0x12345678, 0, "Special pattern IP 18.52.86.120"}
    };

    for (int i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
        // printf("\n--- Test Case %d: %s ---\n", i+1, tests[i].description);
        fails += _test_lookup(tests[i].ip, trie, tests[i].expected);
    }

    TEST_REPORT("lookup", fails);

    free_trie(trie);

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

/**
 * Builds a test LC-Trie structure with sample routing entries
 *
 * @note The trie does not have a 100% fill factor, that's why
 *      the default route is not at the root.
 *
 * @return Pointer to the root of the constructed trie
 * * (s0 b2);
 * |-00* (s0 b0): 10.0.0.0/8 -> iface 2
 * |-01* (s0 b0): 0.0.0.0/0 -> iface 1
 * |-10* (s8 b1);
 * | |-10?? ???? ??0* (s0 b0): 172.16.0.0/12 -> iface 3
 * | \-10?? ???? ??1* (s0 b0): 172.32.0.0/11 -> iface 4
 * \-11* (s0 b0): 192.168.0.0/16 -> iface 1
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

// ==== Main flow ====

// Run all tests
int main() {
    printf("=== LC-Trie Function Test Suite ===\n");
    int fails = 0;

    // Run all test functions
    fails += test_compute_skip();
    fails += test_compute_branch();
    fails += test_sort_rules();
    fails += test_compute_default();
    fails += test_prefix_match();
    fails += test_create_trie();
    fails += test_lookup();

    TEST_REPORT("ALL", fails);

    return fails;
}

#include "lookup.h"
#include "utils.h"
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

/**
 * Unit tests for bit extraction function
 */
void test_extract_bits()
{
    printf("=== Running extract_bits() tests ===\n");
    
    // Test 1: Extract first octet (MSB)
    assert(extract_msb(0xC0A80101, 0, 8) == 0xC0);
    printf("Test 1 passed: First octet extraction\n");

    // Test 2: Extract second octet
    assert(extract_msb(0xC0A80101, 8, 8) == 0xA8);
    printf("Test 2 passed: Second octet extraction\n");

    // Test 3: Extract 4 bits from position 4
    assert(extract_msb(0xC0A80101, 4, 4) == 0x0);
    printf("Test 3 passed: 4-bit extraction from position 4\n");

    // Test 4: Extract 16 bits from start
    assert(extract_msb(0xC0A80101, 0, 16) == 0xC0A8);
    printf("Test 4 passed: 16-bit extraction from start\n");

    // Test 5: Edge case (0 bits)
    assert(extract_msb(0xC0A80101, 0, 0) == 0);
    printf("Test 5 passed: Zero-bit extraction\n");

    printf("=== All extract_bits tests passed ===\n\n");
}

/**
 * Unit tests for prefix checking function
 */
void test_check_prefix()
{
    printf("=== Running check_prefix() tests ===\n");

    // Test 1: Full 32-bit prefix match
    //192.168.1.1 -> 192.168.1.1/32
    assert(check_prefix(0xC0A80101, 0xC0A80101, 32) == 1);
    printf("Test 1 passed: Full 32-bit prefix match\n");

    // Test 2: 24-bit prefix match
    //192.168.1.2 -> 192.168.1.0/24
    assert(check_prefix(0xC0A80102, 0xC0A80100, 24) == 1);
    printf("Test 2 passed: 24-bit prefix match\n");

    // Test 3: 16-bit prefix mismatch
    //192.168.1.1 -✗> 192.184.0.0/16
    assert(check_prefix(0xC0A80101, 0xC0B80000, 16) == 0);
    printf("Test 3 passed: 16-bit prefix mismatch\n");

    // Test 4: 12-bit prefix match
    //192.16.0.2 -> 192.16.0.0/12
    assert(check_prefix(0xAC100002, 0xAC100000, 12) == 1);
    printf("Test 4 passed: 12-bit prefix match\n");

    // Test 5: 12-bit prefix match
    //192.32.0.2 -✗> 192.16.0.0/12
    assert(check_prefix(0xAC200002, 0xAC100000, 12) == 0);
    printf("Test 5 passed: 12-bit prefix match\n");

    printf("=== All check_prefix tests passed ===\n\n");
}

/**
 * Builds a test LC-Trie structure with sample routing entries
 * @return Pointer to the root of the constructed trie
 */
TrieNode *build_test_trie()
{
    // Create leaf nodes for each route
    Rule *leaf1 = calloc(1, sizeof(Rule)); // 192.168.0.0/16
    leaf1->prefix = 0xC0A80000;
    leaf1->prefix_len = 16;
    leaf1->out_iface = 1;

    Rule *leaf2 = calloc(1, sizeof(Rule)); // 10.0.0.0/8
    leaf2->prefix = 0x0A000000;
    leaf2->prefix_len = 8;
    leaf2->out_iface = 2;

    Rule *leaf3 = calloc(1, sizeof(Rule)); // 172.16.0.0/12
    leaf3->prefix = 0xAC100000;
    leaf3->prefix_len = 12;
    leaf3->out_iface = 3;

    Rule *leaf4 = calloc(1, sizeof(Rule)); // 172.32.0.0/11
    leaf4->prefix = 0xAC200000;
    leaf4->prefix_len = 11;
    leaf4->out_iface = 4;

    Rule *default_leaf = calloc(1, sizeof(Rule)); // Default route
    default_leaf->prefix = 0;
    default_leaf->prefix_len = 0;
    default_leaf->out_iface = 0;

    // Level 2 nodes (for 172.x.x.x routes)
    TrieNode *level2 = calloc(2, sizeof(TrieNode));
    level2[0] = (TrieNode){.branch = 0, .pointer = leaf3}; // 172.16.0.0/12
    level2[1] = (TrieNode){.branch = 0, .pointer = leaf4}; // 172.32.0.0/11

    // Level 1 nodes (first 2 bits)
    TrieNode *level1 = calloc(4, sizeof(TrieNode));
    level1[0] = (TrieNode){.branch = 0, .pointer = leaf2};             // 00 -> 10.0.0.0/8
    level1[1] = (TrieNode){.branch = 0, .pointer = default_leaf};      // 01 -> Default
    level1[2] = (TrieNode){.branch = 1, .skip = 8, .pointer = level2}; // 10 -> 172.x.x.x
    level1[3] = (TrieNode){.branch = 0, .pointer = leaf1};             // 11 -> 192.168.0.0/16

    // Root node
    TrieNode *root = calloc(1, sizeof(TrieNode));
    root->branch = 2;
    root->skip = 0;
    root->pointer = level1;

    return root;
}

/**
 * Recursively frees memory allocated for the trie
 * @param root Root node of the trie to free
 */
void free_trie(TrieNode *root)
{
    if (root == NULL)
        return;

    if (root->branch == 0)
    {
        free(root->pointer); // Free leaf node
    }
    else
    {
        TrieNode *children = root->pointer;
        int num_children = 1 << root->branch;

        for (int i = 0; i < num_children; i++)
        {
            free_trie(&children[i]);
        }
        free(children);
    }

    free(root);
}

int main()
{
    // Run unit tests first
    test_extract_bits();
    test_check_prefix();

    // Build test trie
    TrieNode *trie = build_test_trie();

    // Comprehensive test cases
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
        {0xFFFF0000, 0, "255.255.0.0"}};   // 255.255.0.0

        /*
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
        {0xDEADBEEF, 0, "Special pattern IP 222.173.190.239"},
        {0x12345678, 0, "Special pattern IP 18.52.86.120"}};

        */
        


    printf("\n=== Running lookup tests ===\n");
    for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
    {
        int result = lookup(tests[i].ip, trie, 0);
        printf("Test %zu: %s\n", i + 1, tests[i].description);
        printf("IP: %08X -> Result: %d (Expected: %d) %s\n\n",
               tests[i].ip, result, tests[i].expected,
               result == tests[i].expected ? "✓ PASS" : "✗ FAIL");
    }

    free_trie(trie);
    return 0;
}
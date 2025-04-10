#include "lookup.h"
#include "utils.h"
#include <stdlib.h>


/**
 * Checks if an IP address matches a network prefix
 * @param ip_addr IP address to check
 * @param target Network prefix to match against
 * @param prefix_len Length of the network prefix (0-32)
 * @return 1 if match, 0 otherwise
 */
int check_prefix(uint32_t ip_addr, uint32_t target, uint8_t prefix_len) {
    uint32_t mask = prefix_len == 32 ? 0xFFFFFFFF : (~0U) << (32 - prefix_len);
    return (ip_addr & mask) == (target & mask);
}

/**
 * Performs longest prefix match using LC-Trie
 * @param ip_addr IP address to lookup
 * @param trie Pointer to the root of the LC-Trie
 * @param default_port Default port to return if no match found
 * @return The output port number or default_port if no match
 */
int lookup(uint32_t ip_addr, TrieNode *trie, int default_port) {
    TrieNode *current = trie;
    uint8_t bit_pos = current->skip;
    uint8_t read_bits = current->branch;
    
    // Traverse the trie until reaching a leaf node
    while (read_bits != 0) {
        uint32_t bits = extract_msb(ip_addr, bit_pos, read_bits);
        TrieNode *next = ((TrieNode *)current->pointer) + bits;
        
        if (next == NULL) {
            return default_port;
        }

        bit_pos += read_bits + next->skip;
        read_bits = next->branch;
        current = next;
    }

    // Check the leaf node's prefix
    Rule *match = (Rule *)current->pointer;
    if (match == NULL) {
        return default_port;
    }

    return check_prefix(ip_addr, match->prefix, match->prefix_len) ? match->out_iface : default_port;
}


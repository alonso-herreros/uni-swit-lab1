#include "lookup.h"
#include <stdlib.h>


/**
 * Extracts 'n_bits' from 'bitstring' starting at 'start' position
 * @param bitstring The 32-bit input value
 * @param start Starting bit position (0-31)
 * @param n_bits Number of bits to extract (0-32)
 * @return Extracted bits as right-aligned value
 */
uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits) {
    return (bitstring >> (32 - start - n_bits)) & ((1U << n_bits) - 1);
}

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
        uint32_t bits = extract_bits(ip_addr, bit_pos, read_bits);
        TrieNode *next = ((TrieNode *)current->pointer) + bits;
        
        if (next == NULL) {
            return default_port;
        }

        bit_pos += read_bits + next->skip;
        read_bits = next->branch;
        current = next;
    }

    // Check the leaf node's prefix
    LeafNode *match = (LeafNode *)current->pointer;
    if (match == NULL) {
        return default_port;
    }

    return check_prefix(ip_addr, match->prefix, match->prefix_len) ? match->out_port : default_port;
}


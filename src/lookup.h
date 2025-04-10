#ifndef LOOKUP_H
#define LOOKUP_H

#include <stddef.h>  // For size_t, among others?
#include <stdint.h>  // For fixed-width integer types like uint32_t
#include <stdbool.h> // For the bool type



// ==== Data Types ====

/// An IP address as a 32-bit unsigned integer.
typedef uint32_t ip_addr_t;

// ==== Data Structures ====

/** Node of an LC-Trie (includes root).
 *
 * A node in an LC-Trie. It can be either an internal node or a leaf node.
 */
typedef struct TrieNode
{
    /** Branch number (aka "branching factor").
     *
     * This is the number of bits that are used to branch out from this node.
     * For example, if the branch number is 2, this node will have 4 children.
     */
    uint8_t branch;

    /** Length of the largest common prefix (LCP) under this node.
     *
     * This value is crucial as it allows us to skip the comparison of bits
     * that all members of a group share. In PATRICIA trees, this was used to
     * remove nodes with only one child. As the base vector is sorted, if the
     * first and last members of a group share a prefix, all members in between
     * must share it too.
     */
    uint8_t skip;

    /** Pointer to the first child node or corresponding rule.
     *
     * If this is an internal node, it points to its first child node.
     * If this is a leaf node, it points to the rule associated with it.
     * 'You don't need type safety if you know what you're doing' – Sun Tzu
     */
    void *pointer;
} TrieNode;

/** Forwarding rule.
 *
 * Associates a CIDR prefix with an outgoing interface. A packet with a
 * destination address matching the prefix should be sent to this interface
 * (unless overridden by a more specific rule).
 */
typedef struct Rule {
    /** CIDR prefix.
     *
     * Only the first `prefix_len` bits are significant. The rest are ignored,
     * and should be set to 0.
     */
    ip_addr_t prefix;

    /// Length of the prefix in bits.
    uint8_t prefix_len;

    /// Outgoing interface associated with this rule.
    uint32_t out_iface;
} Rule;

int check_prefix(uint32_t ip_addr, uint32_t target, uint8_t prefix_len);
uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits);
int lookup(uint32_t ip_addr, TrieNode *trie, int default_port);

#endif
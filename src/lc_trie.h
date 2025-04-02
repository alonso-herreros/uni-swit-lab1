#ifndef LC_TRIE_H
#define LC_TRIE_H

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
typedef struct TrieNode {
    /** Branch number (aka "branching factor").
     *
     * This is the number of bits that are used to branch out from this node.
     * For example, if the branch number is 2, this node will have 4 children.
     */
    uint8_t  branch;

    /** Length of the largest common prefix (LCP) under this node.
     *
     * This value is crucial as it allows us to skip the comparison of bits
     * that all members of a group share. In PATRICIA trees, this was used to
     * remove nodes with only one child. As the base vector is sorted, if the
     * first and last members of a group share a prefix, all members in between
     * must share it too.
     */
    uint8_t  skip;

    /** Pointer to the first child node or corresponding rule.
     *
     * If this is an internal node, it points to its first child node.
     * If this is a leaf node, it points to the rule associated with it.
     */
    uint32_t pointer;
} TrieNode;


// ==== Function Prototypes ====

// TODO: complete these docs

/** Create an LC-Trie from a FIB file.
 */
TrieNode* create_trie(const char *file);

/** Free the memory allocated for the LC-Trie.
 *
 * @param trie Pointer to the root node of the LC-Trie.
 */
void free_trie(TrieNode *trie);

/** Count the total number of nodes in a given LC-Trie.
 *
 * @param trie Pointer to the root node of the LC-Trie.
 *
 * @return The total number of nodes in the LC-Trie.
 */
uint32_t count_nodes_trie(TrieNode *trie);

/** Look up an IP address in the given LC-Trie and return the next out port.
 *
 * @param ip_addr The IP address to look up.
 * @param trie Pointer to the root node of the LC-Trie.
 *
 * @return The outgoing interface associated with the longest matching prefix.
 */
uint32_t lookup_ip(ip_addr_t ip_addr, TrieNode *trie);

// Not going to add a 'compress_trie' function since the trie is born
// compressed

#endif // LC_TRIE_H

#ifndef LC_TRIE_H
#define LC_TRIE_H

#include <stddef.h>  // For size_t, among others?
#include <stdint.h>  // For fixed-width integer types like uint32_t
#include <stdbool.h> // For the bool type


// ==== Data Types ====

/** IP address type.
 *
 * This is a 32-bit unsigned integer representing an IPv4 address.
 */
typedef uint32_t ip_addr_t;


// ==== Data Structures ====

typedef struct TrieNode {
    uint8_t  branch;
    uint8_t  skip;
    uint32_t pointer; // Can point to a child node or an index in the base vector
} TrieNode;


// ==== Function Prototypes ====

// TODO: complete these docs

/** Create an LC-Trie from a FIB file.
 */
TrieNode* create_trie(const char *file);

/** Free the memory allocated for the LC-Trie.
 */
void free_trie(TrieNode *trie);

/** Count the total number of nodes in a given LC-Trie.
 */
uint32_t count_nodes_trie(ip_addr_t ip_addr, TrieNode *trie);

/** Look up an IP address in the given LC-Trie and return the next out port.
 */
uint32_t lookup_ip(ip_addr_t ip_addr, TrieNode *trie);

// Not going to add a 'compress_trie' function since the trie is born
// compressed

#endif // LC_TRIE_H

#ifndef LC_TRIE_H
#define LC_TRIE_H

#include <stddef.h>  // For size_t, among others?
#include <stdint.h>  // For fixed-width integer types like uint32_t
#include <stdbool.h> // For the bool type

// ==== Constants ====
#ifndef FILL_FACTOR     // Can be overridden at compile time
#define FILL_FACTOR 1.0 // Determines how densely populated branches must be
#endif

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

    struct Rule *parent; // Pointer to the parent rule in the hierarchy
} Rule;

// ==== Function Prototypes ====

// TODO: complete these docs

/** Create an LC-Trie from a set of rules.
 *
 * @param rules Pointer to an array of rules.
 * @param num_rules Number of rules in the array.
 *
 * @return Pointer to the root node of the LC-Trie.
 */
TrieNode *create_trie(Rule *rules, size_t num_rules);

/** Free the memory allocated for the LC-Trie.
 *
 * @param trie Pointer to the root node of the LC-Trie.
 */
void free_trie(TrieNode *trie);

/** Free the memory allocated for the LC-Trie's rules. Available in case the
 *  caller lost the pointer.
 *
 * @param trie Pointer to the root node of the LC-Trie.
 */
void free_trie_rules(TrieNode *trie);

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
 * @param[out] access_count Number of node accesses during the lookup. Will be
 *      overwritten, not added to. Pass NULL to ignore.
 *
 * @return The outgoing interface associated with the longest matching prefix,
 *      or 0 if no rules match.
 */
uint32_t lookup_ip(ip_addr_t ip_addr, TrieNode *trie, int *access_count);

// Not going to add a 'compress_trie' function since the trie is born
// compressed

// I needed to add them here in order to check my functions in proobs_main.c
// Rule* parseFibFile(const char* filename, size_t* count);

Rule *sort_rules(Rule *rules, size_t num_rules);
uint8_t compute_branch(const Rule *group, size_t group_size, uint8_t pre_skip);

uint8_t compute_skip(const Rule *group, size_t group_size, uint8_t pre_skip);

Rule *compute_default(const Rule *group, size_t group_size, uint8_t pre_skip);

int set_group_parent(Rule *group, size_t group_size, Rule *default_rule);

bool rule_match(const Rule *rule, ip_addr_t address);
bool prefix_match(ip_addr_t ip1, ip_addr_t ip2, uint8_t len);

#endif // LC_TRIE_H

#include "lc_trie.h"
#include <stdio.h>
// ==== Data Structures ====



// ==== Function Prototypes ====

// ---- Trie creation ----

/** Recursively create a subtrie.
 *
 *  @param group the memory address of the group's first member (a memory
 *      address in a SORTED base vector)
 *  @param group_size the number of actions in this group, including the one
 *      at `group`.
 *  @param pre_skip the number of bits already skipped and read by parent groups
 *  @param node_ptr the memory address where the root node of the subtrie should
 *      be placed. Must have been previously allocated.
 *
 *  @returns the memory address of the root node of the generated subtrie
 */
TrieNode* create_subtrie(
    Rule *group, size_t group_size, uint8_t pre_skip,
    TrieNode *node_ptr, Rule *default_rule);

// ---- Dependency functions ----

/** Get the length of the largest common prefix in a group of actions.
 *
 *  @param group the memory address of the group's first member (a memory
 *      address in a SORTED base vector)
 *  @param group_size the number of actions in this group, including the one
 *      at `group`. Must be greater than 0.
 *  @param pre_skip the number of bits already skipped and read by parent groups
 *
 *  @return the skip value. If `group_size` is 1, all remaining bits can be
 *      skipped. The absolute maximum value is 32.
 */
uint8_t compute_skip(const Rule *group, size_t group_size, uint8_t pre_skip);

/** Get the branch factor for the given group. Depends on FILL_FACTOR. A
 *  FILL_FACTOR of 1 enforces complete population.
 *
 *  @param group the memory address of the group's first member (a memory
 *      address in a SORTED base vector)
 *  @param group_size the number of actions in this group, including the one
 *      at `group`.
 *  @param pre_skip the number of bits already skipped and read by parent groups
 *
 *  @return the branching factor. The absolute maximum value is 32.
 */
uint8_t compute_branch(const Rule *group, size_t group_size, uint8_t pre_skip);


Rule* compute_default(const Rule *group, size_t group_size, uint8_t pre_skip);

/** Check if an IP address matches a given rule.
 *
 *  @param action the rule to check against
 *  @param address the IP address to check
 *
 *  @return true if the address matches the rule, false otherwise
 */
bool prefix_match(const Rule *action, ip_addr_t address);

/** Extract a specific number of bits from a bitstring.
 *
 * The bits are extracted and placed in the least significant bits of the
 * result.
 *
 *  @param bitstring the bitstring to extract from
 *  @param start the starting position (0-indexed)
 *  @param n_bits the number of bits to extract
 *
 *  @return the extracted bits as an unsigned integer
 */
uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits);


// ---- Lookup ----
bool check_prefix(ip_addr_t ip_addr, ip_addr_t target_prefix, uint8_t prefix_len);
ip_addr_t get_prefix(ip_addr_t bitstring, uint8_t prefix_len);


// ==== Functions ====


// WARN: Untested example implementation. Subject to change
bool prefix_match(const Rule *rule, ip_addr_t address) {
    ip_addr_t address_trunc = extract_bits(address, 0, rule->prefix_len);
    return rule->prefix == address_trunc;
}

// WARN: Untested example implementation. Subject to change
uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits) {
    uint32_t mask = (1 << n_bits) - 1;  // Mask with the n_bits LSBs set to 1
    return (bitstring >> start) & mask; // Shift and apply the mask
}

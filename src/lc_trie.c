#include "lc_trie.h"
#include <stdio.h>
// ==== Data Structures ====



// ==== Function Prototypes ====


// ---- Dependency functions ----

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

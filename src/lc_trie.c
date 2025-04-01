#include "lc_trie.h"
#include <stdio.h>
// ==== Data Structures ====

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
    uint8_t   prefix_len;

    /// Outgoing interface associated with this rule.
    uint32_t  out_iface;
} Rule;


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


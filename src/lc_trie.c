#include "lc_trie.h"
#include "io.h"
#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

// ==== Constants ====

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
TrieNode *create_subtrie(Rule *group, size_t group_size, uint8_t pre_skip,
                         TrieNode *node_ptr, Rule *default_rule) {
    // Base case: single rule in the group
    if (group_size == 1) {
        node_ptr->branch = 0;
        node_ptr->skip = 0;
        node_ptr->pointer = (TrieNode *)group; // Store rule directly
        return node_ptr;
    }

    // Compute skip and branch values
    uint8_t skip = compute_skip(group, group_size, pre_skip);
    uint8_t branch = compute_branch(group, group_size, pre_skip + skip);

    // Update default_rule if a suitable one is found
    Rule *new_default = compute_default(group, group_size, pre_skip);
    if (new_default) {
        default_rule = new_default;
    }

    // Allocate memory for child nodes
    size_t num_children = 1 << branch;
    TrieNode *children = malloc(num_children * sizeof(TrieNode));
    if (!children)
        return NULL;

    // Set current node's properties
    node_ptr->branch = branch;
    node_ptr->skip = skip;
    node_ptr->pointer = children;

    // Recursively create children
    uint8_t children_skip = pre_skip + skip + branch;
    size_t current_pos = 0;

    for (size_t child_n = 0; child_n < num_children; child_n++) {
        size_t subgroup_size = 0;
        while (current_pos + subgroup_size < group_size) {
            uint32_t current_prefix = extract_msb(
                group[current_pos + subgroup_size].prefix,
                pre_skip + skip,
                branch);

            if (current_prefix != child_n)
                break;
            subgroup_size++;
        }

        // Build subtrie for this child
        if (subgroup_size == 0) {
            create_subtrie(default_rule, 1, 0, &children[child_n], default_rule);
        }
        else {
            create_subtrie(
                &group[current_pos], subgroup_size, children_skip,
                &children[child_n], default_rule);
        }

        current_pos += subgroup_size;
    }

    return node_ptr;
}

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
uint8_t compute_skip(const Rule *group, size_t group_size, uint8_t pre_skip) {
    if (group_size == 0)
        return 0;
    if (group_size == 1)
        return group[0].prefix_len - pre_skip;

    uint32_t mask;
    uint8_t min_len = (group[0].prefix_len < group[group_size - 1].prefix_len) ? group[0].prefix_len : group[group_size - 1].prefix_len;

    getNetmask(min_len, &mask);

    ip_addr_t first = group[0].prefix & mask;
    ip_addr_t last = group[group_size - 1].prefix & mask;

    uint8_t skip = pre_skip;
    while (skip < min_len) {
        uint32_t mask_bit = 1 << (31 - skip);
        if ((first & mask_bit) != (last & mask_bit))
            break;
        skip++;
    }

    return skip - pre_skip;
}

/** Get the branch factor for the given group. Depends on FILL_FACTOR.
 *
 *  @param group the memory address of the group's first member (a memory
 *      address in a SORTED base vector)
 *  @param group_size the number of actions in this group, including the one
 *      at `group`.
 *  @param pre_skip the number of bits already skipped and read by parent groups
 *
 *  @return the branching factor. The absolute maximum value is 32.
 */
uint8_t compute_branch(const Rule *group, size_t group_size, uint8_t pre_skip) {
    if (group_size <= 1) return 0;

    uint8_t branch = 1;
    
    while (1) {
        const uint16_t max_branch_prefixes = 1 << branch; //2^branch
        uint16_t unique_branch_prefixes = 1; //Start with 1 (group isn't empty)
        uint32_t last_branch_prefix = extract_msb(group[0].prefix, pre_skip, branch);

        //Count unique branch prefixes at this branch level
        for (size_t i = 1; i < group_size; i++) {
            uint32_t current_prefix = extract_msb(group[i].prefix, pre_skip, branch);
            
            if (current_prefix != last_branch_prefix) {
                unique_branch_prefixes++;
            }
            
            last_branch_prefix = current_prefix; 
        }

        //Return when fill factor condition is no longer met
        if ((float)unique_branch_prefixes / max_branch_prefixes < FILL_FACTOR) {
            return branch;
        }

        branch++;
    }
    return branch;
}

// Comparison function for sorting rules
int compare_rules(const void *a, const void *b) {
    const Rule *rule_a = (const Rule *)a;
    const Rule *rule_b = (const Rule *)b;

    if (rule_a->prefix_len == 0 && rule_a->prefix == 0)
        return -1;
    if (rule_b->prefix_len == 0 && rule_b->prefix == 0)
        return 1;

    if (rule_a->prefix < rule_b->prefix)
        return -1;
    if (rule_a->prefix > rule_b->prefix)
        return 1;

    if (rule_a->prefix_len < rule_b->prefix_len)
        return -1;
    if (rule_a->prefix_len > rule_b->prefix_len)
        return 1;

    return rule_a->out_iface - rule_b->out_iface;
}

/** Sort an array of Rules.
 *
 *  @param rules the Rule array to sort
 *
 *  @return a pointer to a sorted copy of the array. Its size is the same as the
 *  input.
 */
Rule *sort_rules(Rule *rules, size_t num_rules) {
    Rule *sorted = malloc(num_rules * sizeof(Rule));
    if (!sorted)
        return NULL;

    memcpy(sorted, rules, num_rules * sizeof(Rule));
    qsort(sorted, num_rules, sizeof(Rule), compare_rules);

    return sorted;
}

/** Get the most specific action which applies to all possible subgroups.
 *
 *  @param group the memory address of the group's first member (a memory
 *  address in a SORTED base vector)
 *
 * @param group_size the number of actions in
 *  this group, including the one at `group`.
 *
 * @param pre_skip the number of
 *  bits already skipped and read by parent groups
 *
 *  @return a pointer to the action with the most specific rule which can be
 *  applied to all possible subgroups, or 0 if there is none.
 */
Rule *compute_default(const Rule *group, size_t group_size, uint8_t pre_skip) {
    if (group_size == 0)
        return NULL;

    Rule *default_rule = NULL;

    for (size_t i = 0; i < group_size; i++) {
        if (group[i].prefix_len <= pre_skip) {
            default_rule = (Rule *)&group[i];
            break;
        }
    }

    return default_rule;
}

/** Check if an IP address matches a given rule.
 *
 *  @param action the rule to check against
 *  @param address the IP address to check
 *
 *  @return true if the address matches the rule, false otherwise
 */
bool prefix_match(const Rule *rule, ip_addr_t address) {
    uint32_t mask = 0xFFFFFFFF << (32 - rule->prefix_len);
    return (address & mask) == (rule->prefix & mask);
}

// ---- Trie initialization ----

TrieNode *create_trie(Rule *rules, size_t num_rules) {
    if (rules == NULL || num_rules == 0)
        return NULL;

    TrieNode *root = malloc(sizeof(TrieNode));
    if (!root)
        return NULL;

    create_subtrie(rules, num_rules, 0, root, NULL);

    return root;
}

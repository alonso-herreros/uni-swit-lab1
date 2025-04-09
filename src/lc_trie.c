#include "lc_trie.h"
#include "io.h"
#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

#define MAX_BRANCH 5 // Limits the branching factor to control memory usage

// ==== Trie Construction ====

/**
 * Recursively builds a compressed trie (LC-trie) from a sorted array of rules.
 *
 * @param group        Pointer to the first rule of a sorted group.
 * @param group_size   Number of rules in the group.
 * @param pre_skip     Number of bits already skipped in the path.
 * @param node_ptr     Pointer to the allocated TrieNode to fill.
 * @param default_rule Pointer to the current default rule, used for empty subgroups.
 *
 * @return Pointer to the initialized TrieNode, or NULL on failure.
 */
TrieNode *create_subtrie(Rule *group, size_t group_size, uint8_t pre_skip,
                         TrieNode *node_ptr, Rule *default_rule)
{
    // Base case: single rule
    if (group_size == 1)
    {
        node_ptr->branch = 0;
        node_ptr->skip = 0;
        node_ptr->pointer = (TrieNode *)group; // Store rule directly
        return node_ptr;
    }

    // Determine how many bits can be skipped and how many to branch on
    uint8_t skip = compute_skip(group, group_size, pre_skip);
    uint8_t branch = compute_branch(group, group_size, pre_skip + skip);

    // Update default rule if a more specific one is found
    Rule *new_default = compute_default(group, group_size, pre_skip);
    if (new_default)
        default_rule = new_default;

    // Allocate space for child nodes
    size_t num_children = 1 << branch;
    TrieNode *children = malloc(num_children * sizeof(TrieNode));
    if (!children)
        return NULL;

    // Initialize current node
    node_ptr->branch = branch;
    node_ptr->skip = skip;
    node_ptr->pointer = children;

    uint8_t children_skip = pre_skip + skip + branch;
    size_t current_pos = 0;

    // Recursively construct children
    for (size_t child_n = 0; child_n < num_children; child_n++)
    {
        size_t subgroup_size = 0;

        while (current_pos + subgroup_size < group_size)
        {
            uint32_t current_prefix = extract_bits(
                group[current_pos + subgroup_size].prefix,
                pre_skip + skip,
                branch);

            if (current_prefix != child_n)
                break;
            subgroup_size++;
        }

        if (subgroup_size == 0)
        {
            // Empty subgroup: use default rule
            create_subtrie(default_rule, 1, 0, &children[child_n], default_rule);
        }
        else
        {
            // Recursively create subtree
            create_subtrie(
                &group[current_pos], subgroup_size, children_skip,
                &children[child_n], default_rule);
        }

        current_pos += subgroup_size;
    }

    return node_ptr;
}

// ==== Supporting Functions ====

/**
 * Computes how many bits can be skipped (common prefix length).
 *
 * @param group      Pointer to the first rule in the group.
 * @param group_size Number of rules in the group.
 * @param pre_skip   Bits already skipped in the current path.
 *
 * @return Number of additional bits that can be skipped.
 */
uint8_t compute_skip(const Rule *group, size_t group_size, uint8_t pre_skip)
{
    if (group_size == 0)
        return 0;
    if (group_size == 1)
        return group[0].prefix_len - pre_skip;

    uint8_t min_len = (group[0].prefix_len < group[group_size - 1].prefix_len)
                          ? group[0].prefix_len
                          : group[group_size - 1].prefix_len;

    uint32_t mask;
    getNetmask(min_len, &mask);

    ip_addr_t first = group[0].prefix & mask;
    ip_addr_t last = group[group_size - 1].prefix & mask;

    uint8_t skip = pre_skip;
    while (skip < min_len)
    {
        uint32_t bit_mask = 1 << (31 - skip);
        if ((first & bit_mask) != (last & bit_mask))
            break;
        skip++;
    }

    return skip - pre_skip;
}

/**
 * Determines the optimal branching factor based on fill density.
 *
 * @param group      Pointer to the first rule in the group.
 * @param group_size Number of rules in the group.
 * @param pre_skip   Bits already skipped in the current path.
 *
 * @return Number of bits to branch on (max MAX_BRANCH).
 */
uint8_t compute_branch(const Rule *group, size_t group_size, uint8_t pre_skip)
{
    if (group_size <= 1)
        return 0;

    uint8_t branch = 1;

    while (branch <= (IP_ADDRESS_LENGTH - pre_skip))
    {
        size_t max_combinations = 1 << branch;
        size_t unique_prefixes = 1;
        uint32_t last_prefix = extract_bits(group[0].prefix, pre_skip, branch);

        for (size_t i = 1; i < group_size; i++)
        {
            uint32_t current = extract_bits(group[i].prefix, pre_skip, branch);
            if (current != last_prefix)
            {
                unique_prefixes++;
                last_prefix = current;
            }
        }

        if ((float)unique_prefixes / (float)max_combinations > FILL_FACTOR)
        {
            branch++;
        }
        else
        {
            break;
        }
    }

    return (branch > MAX_BRANCH) ? MAX_BRANCH : branch;
}

/**
 * Compares two rules for sorting.
 *
 * @param a Pointer to the first rule.
 * @param b Pointer to the second rule.
 *
 * @return Comparison result for qsort.
 */
int compare_rules(const void *a, const void *b)
{
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

/**
 * Returns a new sorted copy of the rules array.
 *
 * @param rules      Input array of rules.
 * @param num_rules  Number of rules.
 *
 * @return A newly allocated and sorted rule array.
 */
Rule *sort_rules(Rule *rules, size_t num_rules)
{
    Rule *sorted = malloc(num_rules * sizeof(Rule));
    if (!sorted)
        return NULL;

    memcpy(sorted, rules, num_rules * sizeof(Rule));
    qsort(sorted, num_rules, sizeof(Rule), compare_rules);
    return sorted;
}

/**
 * Finds the most specific rule that can serve as a default for all subgroups.
 *
 * @param group      Pointer to the first rule in the group.
 * @param group_size Number of rules in the group.
 * @param pre_skip   Bits already skipped in the current path.
 *
 * @return Pointer to the most applicable default rule, or NULL.
 */
Rule *compute_default(const Rule *group, size_t group_size, uint8_t pre_skip)
{
    if (group_size == 0)
        return NULL;

    Rule *default_rule = NULL;

    for (size_t i = 0; i < group_size; i++)
    {
        if (group[i].prefix_len <= pre_skip)
        {
            default_rule = (Rule *)&group[i];
            break;
        }
    }

    return default_rule;
}

/**
 * Checks if an IP address matches a rule.
 *
 * @param rule    Pointer to the rule to check.
 * @param address IP address to evaluate.
 *
 * @return True if the address matches the rule, false otherwise.
 */
bool prefix_match(const Rule *rule, ip_addr_t address)
{
    uint32_t mask = 0xFFFFFFFF << (32 - rule->prefix_len);
    return (address & mask) == (rule->prefix & mask);
}

/**
 * Extracts a number of bits from a bitstring starting at a given position.
 *
 * @param bitstring Bitstring to extract from.
 * @param start     Starting bit position (0-indexed).
 * @param n_bits    Number of bits to extract.
 *
 * @return Extracted bits as an unsigned integer.
 */
uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits)
{
    uint32_t mask = (1ULL << n_bits) - 1;
    return (bitstring >> start) & (uint32_t)mask;
}

// ==== Trie Entry Point ====

/**
 * Builds a full trie from a sorted set of rules.
 *
 * @param rules      Pointer to the rule array (assumed sorted).
 * @param num_rules  Number of rules.
 *
 * @return Pointer to the root TrieNode, or NULL on failure.
 */
TrieNode *create_trie(Rule *rules, size_t num_rules)
{
    if (rules == NULL || num_rules == 0)
        return NULL;

    TrieNode *root = malloc(sizeof(TrieNode));
    if (!root)
        return NULL;

    create_subtrie(rules, num_rules, 0, root, NULL);
    return root;
}

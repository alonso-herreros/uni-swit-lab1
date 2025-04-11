#include "lc_trie.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

// Macro for debug printing
#ifdef DEBUG
#include <stdio.h>
#define DEBUG_PRINT(fmt, ...) fprintf(stderr, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(...) do {} while (0)
#endif

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
        DEBUG_PRINT("Creating leaf node with rule %p\n", group);
        node_ptr->branch = 0;
        node_ptr->skip = 0;
        node_ptr->pointer = (TrieNode *)group; // Store rule directly
        DEBUG_PRINT("--Done creating leaf node at %p\n", node_ptr);
        return node_ptr;
    }

    DEBUG_PRINT("Creating subtrie with %zu rules at %p\n", group_size, group);
    DEBUG_PRINT("  Pre-skip is %hhu, default is %p\n", pre_skip, default_rule);

    // Compute skip and branch values
    uint8_t skip = compute_skip(group, group_size, pre_skip);
    uint8_t branch = compute_branch(group, group_size, pre_skip + skip);
    DEBUG_PRINT("  skip = %hhu, branch = %hhu\n", skip, branch);

    // Update default_rule if a suitable one is found
    Rule *new_default = compute_default(group, group_size, pre_skip);
    if (new_default) {
        DEBUG_PRINT("  Updating default, now at %p:\n", new_default);
        DEBUG_PRINT("    0x%08X/%hhu -> %d\n", new_default->prefix,
                new_default->prefix_len, new_default->out_iface);
        default_rule = new_default;
    }

    // Edge case! All rules are actually the same but with different prefix lengths
    if ( pre_skip + skip >= IP_ADDRESS_LENGTH ) {
        DEBUG_PRINT("  Full skip encountered, forcing leaf node\n");
        create_subtrie(default_rule, 1, 0, node_ptr, default_rule);
        return node_ptr;
    }

    // Allocate memory for child nodes
    size_t num_children = 1 << branch;
    TrieNode *children = malloc(num_children * sizeof(TrieNode));
    if (!children)
        return NULL;
    DEBUG_PRINT("  Allocated %zu children at %p\n", num_children, new_default);

    // Set current node's properties
    node_ptr->branch = branch;
    node_ptr->skip = skip;
    node_ptr->pointer = children;

    // Recursively create children
    uint8_t children_skip = pre_skip + skip + branch;
    size_t current_pos = 0;

    for (size_t child_n = 0; child_n < num_children; child_n++) {
        DEBUG_PRINT("  Preparing child %zu\n", child_n);
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
        DEBUG_PRINT("    Subgroup size: %zu\n", subgroup_size);

        // Build subtrie for this child
        if (subgroup_size == 0) {
            DEBUG_PRINT("    RECURSING for child at %p\n", &children[child_n]);
            create_subtrie(default_rule, 1, 0, &children[child_n], default_rule);
        }
        else {
            DEBUG_PRINT("    RECURSING for child at %p\n", &children[child_n]);
            create_subtrie(
                &group[current_pos], subgroup_size, children_skip,
                &children[child_n], default_rule);
        }

        current_pos += subgroup_size;
    }
    DEBUG_PRINT("--Done creating subtrie at %p\n", node_ptr);

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
    DEBUG_PRINT("Computing skip for %zu rules at %p with pre-skip %hhu\n",
            group_size, group, pre_skip);
    if (group_size == 0){
        DEBUG_PRINT("--Group is empty. Skip is 0.\n");
        return 0;
    }
    if (group_size == 1) {
        DEBUG_PRINT("--Group has 1 member. Skip is prefix_len - pre_skip.\n");
        return group[0].prefix_len - pre_skip;
    }

    ip_addr_t first = group[0].prefix;
    ip_addr_t last = group[group_size - 1].prefix;
    DEBUG_PRINT("  First IP: 0x%08X; Last IP: 0x%08X\n", first, last);

    uint8_t skip = pre_skip;
    while (skip <= IP_ADDRESS_LENGTH && prefix_match(first, last, skip)) {
        skip++;
    } // At this point, skip is one too big
    skip--;

    DEBUG_PRINT("--Done computing skip: %hhu\n", skip - pre_skip);
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
    DEBUG_PRINT("Computing branch for %zu rules at %p with pre-skip %hhu\n",
            group_size, group, pre_skip);
    DEBUG_PRINT("  FILL_FACTOR is %f\n", FILL_FACTOR);
    if (group_size <= 1) {
        DEBUG_PRINT("--Group too small. Branch is 0.");
        return 0;
    }

    uint8_t branch = 1;

    while (1) {
        const uint16_t max_branch_prefixes = 1 << branch; //2^branch
        uint16_t unique_branch_prefixes = 1; //Start with 1 (group isn't empty)
        uint32_t last_branch_prefix = extract_msb(group[0].prefix, pre_skip, branch);
        DEBUG_PRINT("  Trying branch=%hhu: %hu prefixes available\n", branch,
                max_branch_prefixes);

        //Count unique branch prefixes at this branch level
        for (size_t i = 1; i < group_size; i++) {
            uint32_t current_prefix = extract_msb(group[i].prefix, pre_skip, branch);
            DEBUG_PRINT("    Rule with prefix 0x%08X branches as 0x%X\n",
                    group[i].prefix, current_prefix);

            if (current_prefix != last_branch_prefix) {
                unique_branch_prefixes++;
            }

            last_branch_prefix = current_prefix;
        }
        DEBUG_PRINT("    %u prefixes found\n", unique_branch_prefixes);

        //Return when fill factor condition is no longer met
        if ((float)unique_branch_prefixes / max_branch_prefixes < FILL_FACTOR) {
            DEBUG_PRINT("--Done computing branch: %hhu\n", branch-1);
            return branch - 1; // This branch is too large
        }

        branch++;
    }
    DEBUG_PRINT("--Abnormal end of compute_branch: %hhu\n", branch-1);
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
    DEBUG_PRINT("Computing default for %zu rules at %p\n", group_size, group);
    if (group_size == 0) {
        DEBUG_PRINT("--Group is empty, returning null");
        return NULL;
    }

    Rule *default_rule = NULL;
    Rule last_rule = group[group_size - 1];

    DEBUG_PRINT("  Last rule at %p: 0x%08X/%hhu\n",
            &last_rule, last_rule.prefix, last_rule.prefix_len);
    for (uint8_t i = 0; i < group_size; i++) {
        DEBUG_PRINT("  Checking rule %hhu: 0x%08X/%hhu\n",
                i, group[i].prefix, group[i].prefix_len);
        // Since rules should be ordered, if the last rule is encompassed by
        // the current one, all rules in between are as well
        if (rule_match(&group[i], last_rule.prefix)) {
            DEBUG_PRINT("    Match. This is a default for the rest.\n", i);
            default_rule = (Rule *)&group[i];
        } else {
            DEBUG_PRINT("    No match.\n", i);
            break;
        }
    }

    DEBUG_PRINT("--Done computing default: %p\n", default_rule);
    return default_rule;
}

/** Check if an IP address matches a given rule.
 *
 *  @param action the rule to check against
 *  @param address the IP address to check
 *
 *  @return true if the address matches the rule, false otherwise
 */
inline bool rule_match(const Rule *rule, ip_addr_t address) {
    return prefix_match(rule->prefix, address, rule->prefix_len);
}

/** Check if two IP addresses share a length of prefix.
 *
 *  @param ip1 the first IP address for comparison
 *  @param ip2 the second IP address for comparison
 *  @param len the length of the prefix to check
 *
 *  @return true if the addresses share the prefix, false otherwise
 */
inline bool prefix_match(ip_addr_t ip1, ip_addr_t ip2, uint8_t len) {
    if (len == 0)
        return true; // Empty prefix matches everything

    uint32_t mask = 0xFFFFFFFF << (32 - len);
    return (ip1 & mask) == (ip2 & mask);
}

// ---- Trie initialization ----

TrieNode *create_trie(Rule *rules, size_t num_rules) {
    DEBUG_PRINT("Creating trie with %zu rules at %p\n", num_rules, rules);
    if (rules == NULL || num_rules == 0)
        return NULL;

    TrieNode *root = malloc(sizeof(TrieNode));
    if (!root)
        return NULL;
    DEBUG_PRINT("  Allocated root node at %p\n", root);

    create_subtrie(rules, num_rules, 0, root, NULL);

    DEBUG_PRINT("--Done creating trie at %p\n", root);
    return root;
}

// ---- Count nodes ----

uint32_t count_nodes_trie(TrieNode *trie) {
    if (trie == NULL) {
        return 0;
    }

    // Si es un nodo hoja (no tiene hijos, apunta a una Rule)
    if (trie->branch == 0) {
        return 1;
    }

    // Si es un nodo interno (tiene hijos)
    uint32_t count = 1; // Contamos este nodo
    TrieNode **children = (TrieNode **)trie->pointer; //Pointer to the first child

    // Calculamos cuántos hijos tiene este nodo: 2^branch
    uint32_t num_children = 1 << trie->branch;

    for (uint32_t i = 0; i < num_children; i++) {
        if (children[i]) {
            count += count_nodes_trie(children[i]);
        }
    }

    return count;
}

// ---- Address lookup ----

uint32_t lookup_ip(ip_addr_t ip_addr, TrieNode *trie, int *access_count) {
    DEBUG_PRINT("Looking up IP 0x%08X in trie at %p\n", ip_addr, trie);
    int black_hole = 0; // Temporary variable to avoid dereferencing NULL
    if (access_count == NULL) {
        access_count = &black_hole;
        DEBUG_PRINT("  Dumping access count to %p\n", access_count);
    }

    *access_count = 0; // Initialize access count

    TrieNode *current = trie;
    uint8_t bit_pos = current->skip;
    uint8_t read_bits = current->branch;

    // Traverse the trie until reaching a leaf node
    while (read_bits != 0) {
        uint32_t bits = extract_msb(ip_addr, bit_pos, read_bits);
        DEBUG_PRINT("  Reading %hhu bits from position %hhu: %u\n",
                read_bits, bit_pos, bits);
        TrieNode *next = ((TrieNode *)current->pointer) + bits;
        DEBUG_PRINT("    Next node is at %p\n", next);

        bit_pos += read_bits + next->skip;
        read_bits = next->branch;
        current = next;

        (*access_count)++;
    } // We'll exit when we reach a leaf node, which has branch=0

    DEBUG_PRINT("  Reached a leaf node in %u accesses\n", *access_count);

    // Check the leaf node's prefix
    Rule *match = (Rule *)current->pointer;
    if (match == NULL) {
        return 0;
    }

    DEBUG_PRINT("  Checking against 0x%08X/%hhu (rule at %p)\n",
            match->prefix, match->prefix_len, match);

    uint32_t out_iface = rule_match(match, ip_addr) ? match->out_iface : 0;

    DEBUG_PRINT("    %s\n", out_iface ? "Match" : "No match");
    DEBUG_PRINT("--Done looking IP up: 0x%08X -> %d\n", ip_addr, out_iface);

    return out_iface;
}

// ---- Trie cleanup ----

void free_children(TrieNode *root) {
    DEBUG_PRINT("Freeing children of %p\n", root);
    if (root->branch == 0) {
        DEBUG_PRINT("--Leaf node, nothing to free\n");
        return; // Leaf node, nothing to free
    }
    TrieNode *children = root->pointer;
    int num_children = 1 << root->branch;
    DEBUG_PRINT("  %d children to free at %p\n", num_children, children);

    for (int i = 0; i < num_children; i++) {
        free_children(&children[i]);
    }
    free(children);

    DEBUG_PRINT("--Done freeing children of %p\n", root);
}

void free_trie(TrieNode *root) {
    DEBUG_PRINT("Freeing tree at %p\n", root);
    if (root == NULL) {
        DEBUG_PRINT("--Nothing to free\n");
        return;
    }

    DEBUG_PRINT("  Freeing children\n");
    free_children(root);

    DEBUG_PRINT("  Freeing root\n");
    free(root);
    DEBUG_PRINT("--Done freeing tree\n");
}

// ---- Mock implementations for testing ----

#ifdef MOCK
// WARNING: MOCK IMPLEMENTATION
uint32_t count_nodes_trie(TrieNode *trie) {
    if (trie == NULL) {
        return 0;
    }
    return 42;  // Indeed
}
#endif // MOCK



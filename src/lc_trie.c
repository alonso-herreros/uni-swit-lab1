#include "lc_trie.h"
#include "io.h"
#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

// ==== Constants ====
#define MAX_BRANCH 5 // Maximum branching factor to limit memory usage

// ==== Auxiliary functions ====
int compare_rules(const void *a, const void *b)
{
    const Rule *rule_a = (const Rule *)a;
    const Rule *rule_b = (const Rule *)b;

    // Special case: default route (0.0.0.0/0) always comes first
    if (rule_a->prefix_len == 0 && rule_a->prefix == 0)
        return -1;
    if (rule_b->prefix_len == 0 && rule_b->prefix == 0)
        return 1;

    // First compare the prefix values
    if (rule_a->prefix < rule_b->prefix)
        return -1;
    if (rule_a->prefix > rule_b->prefix)
        return 1;

    // For equal prefixes, shorter prefix lengths come first
    if (rule_a->prefix_len < rule_b->prefix_len)
        return -1;
    if (rule_a->prefix_len > rule_b->prefix_len)
        return 1;

    // If prefix and length are equal, compare interfaces (stable sort)
    return rule_a->out_iface - rule_b->out_iface;
}

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
TrieNode *create_subtrie(Rule *group, size_t group_size, uint8_t pre_skip,
                         TrieNode *node_ptr, Rule *default_rule)
{
    // Caso base: grupo con un solo elemento
    if (group_size == 1)
    {
        node_ptr->branch = 0;
        node_ptr->skip = 0;
        node_ptr->pointer = (TrieNode *)group; // Almacenamos la regla directamente
        return node_ptr;
    }

    // Calculamos skip y branch
    uint8_t skip = compute_skip(group, group_size, pre_skip);
    uint8_t branch = compute_branch(group, group_size, pre_skip + skip);

    // Calculamos el nuevo default_rule si existe
    Rule *new_default = compute_default(group, group_size, pre_skip);
    if (new_default)
    {
        default_rule = new_default;
    }

    // Reservamos memoria para los hijos
    size_t num_children = 1 << branch;
    TrieNode *children = malloc(num_children * sizeof(TrieNode));
    if (!children)
        return NULL;

    // Configuramos el nodo actual
    node_ptr->branch = branch;
    node_ptr->skip = skip;
    node_ptr->pointer = children;

    // Preparamos para crear los subgrupos
    uint8_t children_skip = pre_skip + skip + branch;
    size_t current_pos = 0;

    for (size_t child_n = 0; child_n < num_children; child_n++)
    {
        // Encontramos el subgrupo para este hijo
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

        // Creamos el subtrie
        if (subgroup_size == 0)
        {
            // Subgrupo vacío, usamos la regla por defecto
            create_subtrie(default_rule, 1, 0, &children[child_n], default_rule);
        }
        else
        {
            // Subgrupo no vacío, llamada recursiva
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
uint8_t compute_skip(const Rule *group, size_t group_size, uint8_t pre_skip)
{
    // --- Casos base ---
    if (group_size == 0)
        return 0;
    if (group_size == 1)
        return group[0].prefix_len - pre_skip;

    // --- 1. Obtener prefijos enmascarados ---
    uint32_t mask;
    uint8_t min_len = (group[0].prefix_len < group[group_size - 1].prefix_len) ? group[0].prefix_len : group[group_size - 1].prefix_len;

    getNetmask(min_len, &mask); // Usamos la longitud mínima para la máscara

    ip_addr_t first = group[0].prefix & mask;
    ip_addr_t last = group[group_size - 1].prefix & mask;

    // --- 2. Comparar bits desde pre_skip hasta min_len ---
    uint8_t skip = pre_skip;
    while (skip < min_len)
    {
        // Extraer el bit (31 - skip) de cada prefijo (MSB = bit 31)
        uint32_t mask_bit = 1 << (31 - skip);
        if ((first & mask_bit) != (last & mask_bit))
            break;
        skip++;
    }

    return skip - pre_skip;
}

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
uint8_t compute_branch(const Rule *group, size_t group_size, uint8_t pre_skip)
{
    // Base case: no branching needed for single rule
    if (group_size <= 1)
        return 0;

    uint8_t branch = 1;

    // Find largest branch where unique prefixes exceed FILL_FACTOR threshold
    while (branch <= (IP_ADDRESS_LENGTH - pre_skip))
    {
        size_t max_combinations = 1 << branch; // 2^branch possible prefixes
        size_t unique_prefixes = 1;
        uint32_t last_prefix = extract_bits(group[0].prefix, pre_skip, branch);

        // Count unique prefixes in this branch configuration
        for (size_t i = 1; i < group_size; i++)
        {
            uint32_t current = extract_bits(group[i].prefix, pre_skip, branch);
            if (current != last_prefix)
            {
                unique_prefixes++;
                last_prefix = current;
            }
        }

        // Check if current branch meets density requirement
        if ((float)unique_prefixes / (float)max_combinations > FILL_FACTOR)
        {
            branch++;
        }
        else
        {
            break;
        }
    }

    // Limit branch size to prevent excessive memory usage
    return (branch > MAX_BRANCH) ? MAX_BRANCH : branch;
}

/** Sort an array of Rules.
 *
 *  @param rules the Rule array to sort
 *
 *  @return a pointer to a sorted copy of the array. Its size is the same as the
 *  input.
 */
// Función de comparación para qsort
// Función de comparación con getNetmask

Rule *sort_rules(Rule *rules, size_t num_rules)
{
    // Create copy to avoid modifying original array
    Rule *sorted = malloc(num_rules * sizeof(Rule));
    if (!sorted)
        return NULL;

    memcpy(sorted, rules, num_rules * sizeof(Rule));

    // Sort using standard library qsort
    qsort(sorted, num_rules, sizeof(Rule), compare_rules);

    return sorted;
}

/** Get the most specific action which applies to all possible subgroups.
 *
 *  @param group the memory address of the group's first member (a memory
 *  address in a SORTED base vector) @param group_size the number of actions in
 *  this group, including the one at `group`.  @param pre_skip the number of
 *  bits already skipped and read by parent groups
 *
 *  @return a pointer to the action with the most specific rule which can be
 *  applied to all possible subgroups, or 0 if there is none.
 */
Rule *compute_default(const Rule *group, size_t group_size, uint8_t pre_skip)
{
    if (group_size == 0)
        return NULL;

    Rule *default_rule = NULL;

    // Check for rules that cover entire current address space
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

/** Check if an IP address matches a given rule.
 *
 *  @param action the rule to check against
 *  @param address the IP address to check
 *
 *  @return true if the address matches the rule, false otherwise
 */
bool prefix_match(const Rule *rule, ip_addr_t address)
{
    // Create a mask for the prefix length
    uint32_t mask = (rule->prefix_len == 32) ? 0xFFFFFFFF : (1 << (32 - rule->prefix_len)) - 1 ^ 0xFFFFFFFF;

    // Compare the masked portions
    return (address & mask) == (rule->prefix & mask);
}

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
uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits)
{
    if (n_bits == 0 || n_bits > 32 || start >= 32 || start + n_bits > 32)
    {
        return 0; // Validación básica para evitar errores
    }
    uint32_t mask = (n_bits == 32) ? 0xFFFFFFFF : ((1U << n_bits) - 1);
    return (bitstring >> start) & mask;
}

// ---- Lookup ----
bool check_prefix(ip_addr_t ip_addr, ip_addr_t target_prefix, uint8_t prefix_len);
ip_addr_t get_prefix(ip_addr_t bitstring, uint8_t prefix_len);

// Assuming the Rule matrix introduced is already sorted
TrieNode *create_trie(Rule *rules, size_t num_rules)
{
    if (rules == NULL || num_rules == 0)
        return NULL;

    // Create root node
    TrieNode *root = malloc(sizeof(TrieNode));
    if (!root)
        return NULL;

    // Build trie recursively (assuming create_subtrie is implemented)
    create_subtrie(rules, num_rules, 0, root, NULL);

    return root;
}

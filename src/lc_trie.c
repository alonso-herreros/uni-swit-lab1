#include "lc_trie.h"
#include <stdlib.h>
#include "utils.h"
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
uint8_t compute_skip(const Rule *group, size_t group_size, uint8_t pre_skip) {
    // --- Casos base ---
    if (group_size == 0) return 0;
    if (group_size == 1) return group[0].prefix_len - pre_skip;

    // --- 1. Obtener prefijos enmascarados ---
    uint32_t mask;
    uint8_t min_len = (group[0].prefix_len < group[group_size-1].prefix_len) ? 
                      group[0].prefix_len : group[group_size-1].prefix_len;
    
    getNetmask(min_len, &mask);  // Usamos la longitud mínima para la máscara
    
    ip_addr_t first = group[0].prefix & mask;
    ip_addr_t last = group[group_size-1].prefix & mask;

    // --- 2. Comparar bits desde pre_skip hasta min_len ---
    uint8_t skip = pre_skip;
    while (skip < min_len) {
        // Extraer el bit (31 - skip) de cada prefijo (MSB = bit 31)
        uint32_t mask_bit = 1 << (31 - skip);
        if ((first & mask_bit) != (last & mask_bit)) break;
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
uint8_t compute_branch(const Rule *group, size_t group_size, uint8_t pre_skip);

/** Sort an array of Rules.
 *
 *  @param rules the Rule array to sort
 *
 *  @return a pointer to a sorted copy of the array. Its size is the same as the
 *  input.
 */
// Función de comparación para qsort
// Función de comparación con getNetmask

//Tested sample, think kinda works
int compare_rules(const void *a, const void *b) {
    const Rule *ra = (const Rule*)a;
    const Rule *rb = (const Rule*)b;
    
    // Aplicar máscara usando tu función
    int netmask_a, netmask_b;
    getNetmask(ra->prefix_len, &netmask_a);
    getNetmask(rb->prefix_len, &netmask_b);
    
    ip_addr_t masked_a = ra->prefix & netmask_a;
    ip_addr_t masked_b = rb->prefix & netmask_b;

    if (masked_a < masked_b) return -1;
    if (masked_a > masked_b) return 1;
    
    // Orden descendente por longitud de máscara
    return rb->prefix_len - ra->prefix_len;
}

Rule* sort_rules(Rule *rules, size_t num_rules) {
    if (!rules || num_rules == 0) {
        return NULL;
    }
    
    qsort(rules, num_rules, sizeof(Rule), compare_rules);
    
    return rules;
}



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

//Tested example, think it kinda works
Rule* parseFibFile(const char* filename, size_t* count) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Error opening FIB file");
        return NULL;
    }

    // Primera pasada: contar el número de reglas
    *count = 0;
    int dummy_prefix_len, dummy_out_iface;
    while (fscanf(file, "%*d.%*d.%*d.%*d/%d\t%d\n", &dummy_prefix_len, &dummy_out_iface) == 2) {
        (*count)++;
    }
    //printf("%zu\n",*count);
    rewind(file);

    // Reservar memoria para las reglas
    Rule* rules = malloc(*count * sizeof(Rule));
    if (!rules) {
        fclose(file);
        perror("Memory allocation failed");
        return NULL;
    }

    // Segunda pasada: leer las reglas
    size_t index = 0;
    int octets[4];
    uint32_t netmask;
    
    while (index < *count && 
           fscanf(file, "%d.%d.%d.%d/%hhu\t%u\n", 
                  &octets[0], &octets[1], &octets[2], &octets[3],
                  &rules[index].prefix_len, &rules[index].out_iface) == 6) {                    
        
        // Construir el prefijo en formato binario
        rules[index].prefix = (octets[0] << 24) | (octets[1] << 16) | 
                            (octets[2] << 8) | octets[3];
        
        // Aplicar la máscara de red para asegurar que solo los bits del prefijo son significativos
        getNetmask(rules[index].prefix_len, (int*)&netmask);
        rules[index].prefix &= netmask;
        
        index++;
    }
    //printf("%zu\n", index);

    fclose(file);

    // Verificar que se leyeron todas las reglas esperadas
    if (index != *count) {
        fprintf(stderr, "Warning: File format mismatch. Expected %zu rules, read %zu\n", 
               *count, index);
        *count = index; // Actualizar el conteo real
        
        // Reajustar la memoria si es necesario
        Rule* tmp = realloc(rules, *count * sizeof(Rule));
        if (tmp) {
            rules = tmp;
        }
    }

    return rules;
}

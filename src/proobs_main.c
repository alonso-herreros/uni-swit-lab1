#include "io.h"
#include "lc_trie.h"
#include <stdlib.h>

void printRule(const Rule* rule) {
    // Extrae cada octeto del prefijo enmascarado (ya procesado)
    uint8_t oct1 = (rule->prefix >> 24) & 0xFF;
    uint8_t oct2 = (rule->prefix >> 16) & 0xFF;
    uint8_t oct3 = (rule->prefix >> 8)  & 0xFF;
    uint8_t oct4 = rule->prefix & 0xFF;

    // Formato de salida: "IP/len -> iface"
    printf("%u.%u.%u.%u/%u -> %u\n", 
           oct1, oct2, oct3, oct4,
           rule->prefix_len,
           rule->out_iface);
}

//PRUEBA PARA CALCULAR EL SKIP DE LAS ARRAYS ORDENADAS DE RULES
int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <FIB_FILE>\n", argv[0]);
        return 1;
    }

    size_t rule_count;
    Rule* rules = parseFibFile(argv[1], &rule_count);
    if (!rules) return 1;

    printf("\n=== Loaded Rules ===\n");
    for (size_t i = 0; i < rule_count; i++) {
        printf("[%3zu] ", i + 1);
        printRule(&rules[i]);
    }

    Rule *sorted = sort_rules(rules, rule_count);
    
    printf("\n=== Reglas ordenadas ===\n");
    for (size_t i = 0; i < rule_count; i++) {
        printRule(&sorted[i]);
    }
    printf("%zu\n",rule_count);
    
    uint8_t skip = compute_skip(sorted, rule_count, 0);
    printf("\nResultado compute_skip: %u bits comunes\n", skip);

    
    free(sorted);    
    return 0;
    
}

//PRUEBA CONSTRUCCIÓN ARRAY DE RULES
/*int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <FIB_FILE>\n", argv[0]);
        return 1;
    }

    size_t rule_count;
    Rule* rules = parseFibFile(argv[1], &rule_count);
    if (!rules) return 1;

    printf("\n=== Loaded Rules ===\n");
    for (size_t i = 0; i < rule_count; i++) {
        printf("[%3zu] ", i + 1);
        printRule(&rules[i]);
    }

    for (size_t i = 0; i < rule_count; i++) {
        printf("%u\n", rules[i].out_iface);
    }

    free(rules);
    return 0;
}*/


//PRUEBA ORDENACIÓN DEL ARRAY DE RULES
/*int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <FIB_FILE>\n", argv[0]);
        return 1;
    }

    size_t num_rules;
    Rule* rules = parseFibFile(argv[1], &num_rules);
    if (!rules) return 1;
    
    printf("=== Reglas originales ===\n");
    for (size_t i = 0; i < num_rules; i++) {
        printRule(&rules[i]);
    }
    
    

    Rule *sorted = sort_rules(rules, num_rules);
    
    printf("\n=== Reglas ordenadas ===\n");
    for (size_t i = 0; i < num_rules; i++) {
        printRule(&sorted[i]);
    }
    printf("%zu\n",num_rules);

    free(sorted);
    return 0;
}*/

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

    Rule *sorted_rules = sort_rules(rules);
    
    printf("\n=== Reglas ordenadas ===\n");
    for(size_t i=0; i<rule_count; i++){
        printf("[%3zu] ", i + 1);
        printRule(&sorted_rules[i]);
    }

    free(sorted_rules);
    return 0;

    /*Rule *sorted_rules = sort_rules(rules);  // ¡Usa tu implementación!
    
    // 3. Mostrar reglas ordenadas
    printf("\n=== Reglas ordenadas ===\n");
    for (size_t i = 0; i < num_rules; i++) {
        printf("%u.%u.%u.%u/%u -> if%u\n",
               (sorted_rules[i].prefix >> 24) & 0xFF,
               (sorted_rules[i].prefix >> 16) & 0xFF,
               (sorted_rules[i].prefix >> 8) & 0xFF,
               sorted_rules[i].prefix & 0xFF,
               sorted_rules[i].prefix_len,
               sorted_rules[i].out_iface);
    }

    // 4. Liberar memoria
    free(sorted_rules);
    free(rules);
    return 0;
}*/

int main(int argc, char* argv[]) {
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

    free(sorted);
    return 0;
}

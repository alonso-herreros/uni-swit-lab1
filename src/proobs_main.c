#include "io.h"
#include "lc_trie.h"
#include <stdlib.h>

void printRule(const Rule* rule) {
    printf("%u.%u.%u.%u/%u -> %u\n",
           (rule->prefix >> 24) & 0xFF,
           (rule->prefix >> 16) & 0xFF,
           (rule->prefix >> 8) & 0xFF,
           rule->prefix & 0xFF,
           rule->prefix_len,
           rule->out_iface);
}

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

    for (size_t i = 0; i < rule_count; i++) {
        printf("%u\n", rules[i].out_iface);
    }

    free(rules);
    return 0;
}
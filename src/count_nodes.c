#include "lc_trie.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

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
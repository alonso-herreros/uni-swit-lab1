#include "../src/lc_trie.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// ==== Auxiliary functions ====

// Funciones auxiliares (para crear nodos)
TrieNode *create_leaf(uint32_t prefix, uint8_t len, uint32_t iface)
{
    Rule *rule = malloc(sizeof(Rule));
    rule->prefix = prefix;
    rule->prefix_len = len;
    rule->out_iface = iface;

    TrieNode *leaf = malloc(sizeof(TrieNode));
    leaf->branch = 0;
    leaf->skip = 0;
    leaf->pointer = rule;
    return leaf;
}

TrieNode *create_internal(uint8_t branch, uint8_t skip, TrieNode **children)
{
    TrieNode *node = malloc(sizeof(TrieNode));
    node->branch = branch;
    node->skip = skip;

    // Cantidad de hijos esperados
    uint32_t n = 1 << branch;

    // Asignamos el array de punteros a hijos
    TrieNode **child_array = malloc(n * sizeof(TrieNode *));
    for (uint32_t i = 0; i < n; i++)
    {
        child_array[i] = children[i]; // apuntamos directamente, no copiamos
    }

    node->pointer = child_array;
    return node;
}

void free_trie(TrieNode *node)
{
    if (node->branch == 0)
    {
        free(node->pointer); // Libera la Rule
    }
    else
    {
        int n_children = 1 << node->branch;
        TrieNode **children = (TrieNode **)node->pointer;
        for (int i = 0; i < n_children; i++)
        {
            if (children[i])
            {
                free_trie(children[i]);
            }
        }
    }
    free(node);
}

int main()
{
    /*
    // Creamos las hojas
    TrieNode *leaf1 = create_leaf(0x0A010000, 16, 1); // 10.1.0.0/16
    TrieNode *leaf2 = create_leaf(0x0A020000, 16, 2); // 10.2.0.0/16
    TrieNode *leaf3 = create_leaf(0x0B000000, 8, 3);  // 11.0.0.0/8
    TrieNode *leaf4 = create_leaf(0xC0A80000, 24, 4); // 192.168.0.0/24
    TrieNode *leaf5 = create_leaf(0xC0A80100, 24, 5); // 192.168.1.0/24

    // Nodo interno 1 (branch=1 → 2 hijos)
    TrieNode *children1[2] = {leaf1, leaf2};
    TrieNode *internal1 = create_internal(1, 8, children1);

    // Nodo interno 2 (branch=1 → 2 hijos)
    TrieNode *children2[2] = {leaf4, leaf5};
    TrieNode *internal2 = create_internal(1, 8, children2);

    // Raíz (branch=2 → 4 hijos)
    TrieNode *root_children[4] = {internal1, leaf3, internal2, NULL}; // Agrega NULL para completar los 4
    TrieNode *root = create_internal(2, 0, root_children);

    // Contar nodos
    uint32_t count = count_nodes_trie(root);
    printf("Total nodes: %u (expected: 8)\n", count);

    // Liberar memoria (recursivamente)
    free_trie(root);
    return 0;*/
    TrieNode *leaf1 = create_leaf(0x64400000, 10, 1);  // 100.64.0.0/10
    TrieNode *leaf2 = create_leaf(0x64800000, 9, 2);   // 100.128.0.0/9
    TrieNode *leaf3 = create_leaf(0x64000000, 8, 3);   // 100.0.0.0/8
    TrieNode *leaf5 = create_leaf(0xC0000000, 8, 4);   // 192.0.0.0/8

    // Nodo interno 2 con leaf2 y leaf3
    TrieNode *children2[2] = {leaf2, leaf3};
    TrieNode *internal2 = create_internal(1, 2, children2);

    // Nodo interno 1 con leaf1 y internal2
    TrieNode *children1[2] = {leaf1, internal2};
    TrieNode *internal1 = create_internal(1, 4, children1);

    // Nodo raíz con internal1 y leaf5
    TrieNode *root_children[2] = {internal1, leaf5};
    TrieNode *root = create_internal(1, 0, root_children);

    // Contar nodos
    uint32_t count = count_nodes_trie(root);
    printf("Total nodes: %u (expected: 7)\n", count);

    // Liberar
    free_trie(root);
    return 0;
}

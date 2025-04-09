#include "lookup.h"
#include "io.h"
#include <stdlib.h>
#include "utils.h"
#include <stdio.h>


int lookup(uint32_t ip_addr, TrieNode *trie){
    
}


uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits)
{
    uint32_t mask = (1ULL << n_bits) - 1;
    return (bitstring >> start) & (uint32_t)mask;
}
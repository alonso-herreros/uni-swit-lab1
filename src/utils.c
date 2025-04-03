#include "utils.h"

/********************************************************************
 * Generate a netmask of length prefixLength
 ********************************************************************/
void getNetmask(int prefixLength, int *netmask){

	*netmask = (0xFFFFFFFF << (IP_ADDRESS_LENGTH - prefixLength));

}

/********************************************************************
 * Example of a very simple hash function using the modulus operator
 * For more info: https://gist.github.com/cpq/8598442
 ********************************************************************/
int hash(uint32_t IPAddress, int sizeHashTable){

	//Map the key (IPAddress) to the appropriate index of the hash table
  int index = IPAddress % sizeHashTable;
  return (index);

}

uint32_t extract_bits(uint32_t bitstring, uint8_t start, uint8_t n_bits) {
    uint32_t mask = (1 << n_bits) - 1;  // Mask with the n_bits LSBs set to 1
    return (bitstring >> start) & mask; // Shift and apply the mask
}

//RL Lab 2020 Switching UC3M

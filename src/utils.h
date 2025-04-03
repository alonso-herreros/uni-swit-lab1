#include <stdint.h>

/********************************************************************
 * Constant definitions
 ********************************************************************/
#define IP_ADDRESS_LENGTH 32


/********************************************************************
 * Generate a netmask of length prefixLength
 ********************************************************************/
void getNetmask (int prefixLength, int *netmask);

/********************************************************************
 * Example of a very simple hash function using the modulus operator
 * For more info: https://gist.github.com/cpq/8598442
 ********************************************************************/
int hash(uint32_t IPAddress, int sizeHashTable);

/** Extract `n_bits` bits starting at position `start`, LSB being 0.
 *
 * @param bitstring the bitstring to extract from
 * @param start the starting position (0-indexed, 0 being the LSB)
 * @param n_bits the number of bits to extract
 *
 * @return the extracted bits as an unsigned integer
 */
uint32_t extract_lsb(uint32_t bitstring, uint8_t start, uint8_t n_bits);

/** Extract `n_bits` bits starting at position `start`, MSB being 0.
 *
 * @param bitstring the bitstring to extract from
 * @param start the starting position (0-indexed, 0 being the MSB)
 * @param n_bits the number of bits to extract
 *
 * @return the extracted bits as an unsigned integer
 */
uint32_t extract_msb(uint32_t bitstring, uint8_t start, uint8_t n_bits);

//RL Lab 2020 Switching UC3M

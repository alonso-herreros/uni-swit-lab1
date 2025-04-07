#include "lc_trie.h"
#include "io.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h> // For time measurements

// ==== Constants ====
#define OUT_PREFIX ".out"
#define OUT_PREFIX_LEN 4

// Helper function to convert IP address string to uint32_t
ip_addr_t ip_to_uint32(const char *ip_str);
// Abstraction to read the FIB file and create the LC-trie
TrieNode *trie_from_file(const char *fib_file);


int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s FIB InputPacketFile\n", argv[0]);
        return 1;
    }

    int status;     // Used at various points for return status checking
    TrieNode *root; // Root of the trie we'll use through the program

    char *fib_filename = argv[1];
    char *input_filename = argv[2];
    const size_t input_file_len = strlen(input_filename);
    const size_t output_file_len = input_file_len + OUT_PREFIX_LEN;
    char *output_file = malloc( sizeof(char) * (output_file_len+1) );
    snprintf(output_file, sizeof(output_file), "%s.out", input_filename);

    // Initialize the I/O library
    if ( (status=initializeIO(fib_filename, input_filename)) != OK ) {
        printIOExplanationError(status);
        return 1;
    }

    // Attempt to create the trie from the FIB file
    root = trie_from_file(fib_filename);
    if (!root) {
        printIOExplanationError(PARSE_ERROR);
        return 1;
    }

    FILE *in_fp = fopen(input_filename, "r");
    if (!in_fp) {
        perror("Error opening input file");
        // You'll need a function to free the trie memory
        return 1;
    }

    FILE *out_fp = fopen(output_file, "w");
    if (!out_fp) {
        perror("Error opening output file");
        fclose(in_fp);
        // Free trie memory
        return 1;
    }

    char ip_str[4];
    unsigned long long total_time = 0;
    unsigned long long total_nodes = 0;
    unsigned int packets_processed = 0;

    struct timeval start, end;

    while (fgets(ip_str, sizeof(ip_str), in_fp) != NULL) {
        // Remove trailing newline
        ip_str[strcspn(ip_str, "\n")] = 0;
        ip_addr_t ip_address = ip_to_uint32(ip_str);

        gettimeofday(&start, NULL);
        uint32_t out_ifc = lookup_ip(ip_address, root);
        gettimeofday(&end, NULL);

        long long elapsed_time = (end.tv_sec - start.tv_sec) * 1000000LL + (end.tv_usec - start.tv_usec);
        total_time += elapsed_time;
        packets_processed++;
        // In a real implementation, you would need to track the number of accessed nodes during lookup

        fprintf(out_fp, "%s;%s;%d;%lld\n", ip_str, (out_ifc > 0) ? (char[]){out_ifc + '0', 0} : "MISS", 1, elapsed_time * 1000); // Example AccessedNodes and time in nanoseconds
    }

    // Print summary information as per the lab guide [18]
    fprintf(out_fp, "\n");
    fprintf(out_fp, "Number of nodes in the tree = %d\n", 0); // You need to implement counting nodes
    fprintf(out_fp, "Packets processed = %u\n", packets_processed);
    fprintf(out_fp, "Average node accesses = %.2f\n", (double)total_nodes / packets_processed);
    fprintf(out_fp, "Average packet processing time (nsecs) = %.2f\n", (double)total_time * 1000 / packets_processed);
    fprintf(out_fp, "Memory (Kbytes) = %d\n", 0); // You need to track memory usage
    fprintf(out_fp, "CPU Time (secs) = %.6f\n", (double)0); // You might need to use clock() for CPU time

    fclose(in_fp);
    fclose(out_fp);

    // Implement a function to free the entire trie structure to avoid memory leaks
    // free_trie(root);

    return 0;
}

// Basic IP string to uint32_t conversion (you might need a more robust one)
ip_addr_t ip_to_uint32(const char *ip_str) {
    unsigned int a, b, c, d;
    if (sscanf(ip_str, "%u.%u.%u.%u", &a, &b, &c, &d) == 4) {
        return (a << 24) | (b << 16) | (c << 8) | d;
    }
    return 0; // Error case
}

TrieNode *trie_from_file(const char *fib_file) {
    return NULL; // Placeholder for the actual implementation
}

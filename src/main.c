#include "lc_trie.h"
#include "io.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h> // For time measurements

// ==== Constants ====
#define OUT_PREFIX ".out"
#define OUT_PREFIX_LEN 4

// Abstraction to read the FIB file and create the LC-trie
TrieNode *trie_from_file(const char *fib_file);

/** Look up an IP address in the trie, measure, and log the result
 *
 * @param ip_address The IP address to look up
 * @param root The root of the trie to look up in
 * @param[out] accumSearchTime Pointer where the time spent will be ADDED
 * @param[out] accumAccessCount Pointer where the node access count will be
 *      ADDED
 *
 * @return 0 on success, -1 on failure
 */
int profiled_lookup(
    ip_addr_t ip_address, TrieNode *root,
    double *accumSearchTime, int *accumAccessCount
);


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

    // Accumulators for search time and memory accesses
    double total_search_time = 0;  // Total time spent in lookups
    int total_access_count = 0;    // Total number of 'table accesses'
    int i = 0;                     // Total number of addresses processed

    // Process the input packet file
    ip_addr_t addr;
    for (i=0; (status=readInputPacketFileLine(&addr)) != REACHED_EOF; i++) {
        if (status != OK) {
            printIOExplanationError(status); // Could be BAD_INPUT_FILE
            return 1;
        }
        if (profiled_lookup(addr, root,
                &total_search_time, &total_access_count) != 0) {
            fprintf(stderr, "Error during lookup\n");
            return 1;
        }
    }

    // Print the summary information
    int node_count = count_nodes_trie(root);
    double avg_access_count = (double)total_access_count / i;
    double avg_search_time = total_search_time / i;
    printSummary(node_count, i, avg_access_count, avg_search_time);

    // Clean up
    freeIO();
    free_trie(root);

    return 0;
}

TrieNode *trie_from_file(const char *fib_file) {
    return NULL; // Placeholder for the actual implementation
}

int profiled_lookup(
        ip_addr_t ip_address, TrieNode *root,
        double *accumSearchTime, int *accumAccessCount
    ) {
    // Placeholder for the actual implementation
    struct timespec initialTime, finalTime; // Performance measurement
    uint32_t outInterface = 0; // Set by lookup_ip
    int tableAccessCount = 0;  // Set by lookup_ip

    // TODO: Pass tableAccessCount to lookup_ip (check #16)
    // Timed IP lookup
    clock_gettime(CLOCK_MONOTONIC_RAW, &initialTime);
    outInterface = lookup_ip(ip_address, root);
    clock_gettime(CLOCK_MONOTONIC_RAW, &finalTime);

    double searchingTime; // Set by printOutputLine

    // Print output and performance to stdout and output file
    printOutputLine(
        ip_address, outInterface, &initialTime, &finalTime,
        &searchingTime, tableAccessCount
    );

    // Update the accumulators
    *accumSearchTime += searchingTime;
    *accumAccessCount += tableAccessCount;

    return 0;
}

#include "lc_trie.h"
#include "io.h"
#include <stdio.h>
#include <stdint.h>
#include <time.h> // For time measurements

// Macro for debug printing
#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) fprintf(stdout, "[DEBUG] " fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(...) do {} while (0)
#endif

// ==== Constants ====
#define OUT_PREFIX ".out"
#define OUT_PREFIX_LEN 4

/** Read the FIB file and create a trie
 *
 * @return A pointer to the root of the trie, or NULL on failure
 *
 * @warning The FIB file is read using the IO library, which is assumed to be
 *      initialized.
 * @warning The caller is responsible for freeing the memory using free_trie().
 */
TrieNode *read_trie();

/** Read the FIB file and return a heap-allocated array of rules
 *
 * @param[out] rule_count Pointer where the number of rules will be stored
 *
 * @return A pointer to the (unsorted) array of rules, or NULL on failure
 *
 * @warning The FIB file is read using the IO library, which is assumed to be
 *      initialized.
 */
Rule *read_rules(int *rule_count);

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

    // Initialize the I/O library
    if ( (status=initializeIO(fib_filename, input_filename)) != OK ) {
        printIOExplanationError(status);
        return 1;
    }
    DEBUG_PRINT("I/O init done\n");

    DEBUG_PRINT("Reading FIB start\n");
    // Attempt to create the trie from the FIB file
    root = read_trie();
    if (!root) {
        printIOExplanationError(PARSE_ERROR);
        return 1;
    }
    DEBUG_PRINT("FIB read done\n");

    // Accumulators for search time and memory accesses
    double total_search_time = 0;  // Total time spent in lookups
    int total_access_count = 0;    // Total number of 'table accesses'
    int i = 0;                     // Total number of addresses processed

    DEBUG_PRINT("Ready to process Input\n");
    // Process the input packet file
    ip_addr_t addr;
    for (i=0; (status=readInputPacketFileLine(&addr)) != REACHED_EOF; i++) {
        DEBUG_PRINT("Processing input line %d\n", i);
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
    DEBUG_PRINT("Input processing done\n");

    DEBUG_PRINT("Summary start\n");
    // Print the summary information
    int node_count = count_nodes_trie(root);
    double avg_access_count = (double)total_access_count / i;
    double avg_search_time = total_search_time / i;
    printSummary(node_count, i, avg_access_count, avg_search_time);
    DEBUG_PRINT("Summary done\n");

    DEBUG_PRINT("Clean up start\n");
    // Clean up
    freeIO();
    free_trie(root);

    DEBUG_PRINT("Clean up done\n");

    return 0;
}

TrieNode *read_trie() {
    DEBUG_PRINT("Read trie enter\n");
    int rule_count = 0;
    Rule *rules = read_rules(&rule_count);
    DEBUG_PRINT("Read rules done (%d rules)\n", rule_count);

    Rule *sorted = sort_rules(rules, rule_count);
    free(rules);
    DEBUG_PRINT("Sort rules done\n");

    TrieNode *root = create_trie(sorted, rule_count);
    DEBUG_PRINT("Create trie done, root at %p\n", root);
    // Since create_trie doesn't create a copy of the rules yet
    // free(sorted);
    // DEBUG_PRINT("Free rules done\n");

    return root;
}

Rule *read_rules(int *rule_count) {
    // Min chars per line: 11
    // Max chars per line: 24
    int status;

    // Temporarily using a basic dynamic array with exponential growth
    size_t capacity = 2;
    size_t size = 0;
    Rule* rules = malloc(sizeof(Rule) * capacity);
    DEBUG_PRINT("Malloc rules done, capacity %zu\n", capacity);

    ip_addr_t addr;
    int prefix_len;
    int out_iface;

    while ((status=readFIBLine(&addr, &prefix_len, &out_iface)) != REACHED_EOF){
        if (status != OK) {
            free(rules);
            return NULL;
        }
        DEBUG_PRINT("Read rule %zu: %u/%d %d\n", size, addr, prefix_len, out_iface);

        if (size == capacity) { // Next element would overflow
            capacity *= 2;
            rules = realloc(rules, sizeof(Rule) * capacity);
            DEBUG_PRINT("Realloc rules done, new capacity %zu\n", capacity);
        }

        rules[size].prefix = addr;
        rules[size].prefix_len = prefix_len;
        rules[size].out_iface = out_iface;

        size++;
    }

    // Should we trim the array to the actual size, or leave some slack?
    rules = realloc(rules, sizeof(Rule) * size);
    *rule_count = size;

    return rules;
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
    outInterface = lookup_ip(ip_address, root, &tableAccessCount);
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

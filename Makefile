# Sources
SRC_DIR    = src
TEST_DIR   = test
BUILD_DIR  = build

PROD_FILES   = main.c utils.c io.c lc_trie.c
PROOBS_FILES = proobs.c

# PROD   = $(addprefix $(SRC_DIR)/, $(PROD_FILES))
PROD_OBJS   = $(addprefix $(BUILD_DIR)/, $(PROD_FILES:.c=.o))
PROOBS_OBJS = $(addprefix $(BUILD_DIR)/, $(PROOBS_FILES:.c=.o))
SHARED_OBJS = $(PROD_OBJS:$(BUILD_DIR)/main.o=)

PROD_BIN   = my_route_lookup
PROOBS_BIN = $(BUILD_DIR)/proobs_runner

# Deployment test
COMPARE_BIN   = $(TEST_DIR)/compare_algorithms.sh
REFERENCE_BIN = $(TEST_DIR)/linearSearch
TEST_DATA_DIR = $(TEST_DIR)/data

TEST_FIB_1 = $(TEST_DATA_DIR)/routing_table_simple.txt
TEST_FIB_2 = $(TEST_DATA_DIR)/routing_table.txt

COMPARE_CMD = $(COMPARE_BIN) ./$(PROD_BIN) ./$(REFERENCE_BIN)


# Compilation
CC = gcc
CFLAGS += -Wall -O3 -I$(SRC_DIR)

all: $(PROD_BIN)

test: $(PROD_BIN)
	@echo "==== Testing $(PROD_BIN)... against $(REFERENCE_BIN) ===="
	@echo "---- Testing with $(TEST_FIB_1) ----"
	@$(COMPARE_CMD) $(TEST_FIB_1) $(TEST_DATA_DIR)/prueba0.txt \
		$(TEST_FIB_1) $(TEST_DATA_DIR)/prueba1.txt \
		$(TEST_FIB_1) $(TEST_DATA_DIR)/prueba2.txt \
		$(TEST_FIB_1) $(TEST_DATA_DIR)/prueba3.txt
	@echo "---- Testing with $(TEST_FIB_2) ----"
	@$(COMPARE_CMD) $(TEST_FIB_2) $(TEST_DATA_DIR)/prueba0.txt \
		$(TEST_FIB_2) $(TEST_DATA_DIR)/prueba1.txt \
		$(TEST_FIB_2) $(TEST_DATA_DIR)/prueba2.txt \
		$(TEST_FIB_2) $(TEST_DATA_DIR)/prueba3.txt
	@echo "==== Done testing $(PROD_BIN) ===="

proobs: $(PROOBS_BIN)
	@echo "==== Running *proobs*... ===="
	@$(PROOBS_BIN)
	@echo "==== Finished *proobs* ===="

$(PROD_BIN): $(PROD_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(PROOBS_BIN): $(PROOBS_OBJS) $(SHARED_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

# I wish this worked, but it doesn't due to the way pattern matching works
# $(BUILD_DIR)/%: | $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(TEST_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $@

.PHONY: clean test all

clean:
	@echo "==== Cleaning up... ===="
	@rm -rf $(PROD_BIN) $(BUILD_DIR)
	@echo "==== Done ===="

#RL Lab 2020 Switching UC3M

SRC_DIR    = src
TEST_DIR   = test
BUILD_DIR  = build

PROD_FILES = main.c utils.c io.c
TEST_FILES = test.c

# PROD   = $(addprefix $(SRC_DIR)/, $(PROD_FILES))
# TEST   = $(addprefix $(TEST_DIR)/, $(TEST_FILES))
PROD_OBJS   = $(addprefix $(BUILD_DIR)/, $(PROD_FILES:.c=.o))
TEST_OBJS   = $(addprefix $(BUILD_DIR)/, $(TEST_FILES:.c=.o))
SHARED_OBJS = $(PROD_OBJS:$(BUILD_DIR)/main.o=)

PROD_BIN   = my_route_lookup
TEST_BIN   = $(BUILD_DIR)/test_runner

CC = gcc
CFLAGS += -Wall -O3 -I$(SRC_DIR)

all: $(PROD_BIN)

test: $(TEST_BIN)
	@echo "==== Running tests... ===="
	@$(TEST_BIN)
	@echo "==== Finished tests ===="

$(PROD_BIN): $(PROD_OBJS)
	$(CC) $(CFLAGS) $^ -o $@ -lm

$(TEST_BIN): $(TEST_OBJS) $(SHARED_OBJS)
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

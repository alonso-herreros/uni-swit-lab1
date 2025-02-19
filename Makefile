SRC_DIR := src
BIN_DIR := bin

SRC = $(wildcard $(SRC_DIR)/*.c)
EXE := $(BIN_DIR)/my_route_lookup

CFLAGS = -Wall -O3

.PHONY: all my_route_lookup
all: $(EXE)
my_route_lookup: $(EXE)
	
$(EXE): $(SRC_FILES) | $(BIN_DIR)
	gcc $(CFLAGS) $(SRC_FILES) -o $@ -lm

$(BIN_DIR):
	mkdir -p $@

test: my_route_lookup
	@echo Not yet implemented

.PHONY: clean
clean:
	rm -f my_route_lookup

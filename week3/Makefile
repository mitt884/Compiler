CC = gcc
CFLAGS = -Wall -Wextra -O2

# Source folders
EX1_DIR = Ex1/incompleted
EX2_DIR = Ex2/incompleted

# Auto-detect all .c files
EX1_SRCS = $(wildcard $(EX1_DIR)/*.c)
EX2_SRCS = $(wildcard $(EX2_DIR)/*.c)

# Output binaries
EX1_BIN = ex1
EX2_BIN = ex2

# Default target
all: $(EX1_BIN) $(EX2_BIN)

$(EX1_BIN): $(EX1_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

$(EX2_BIN): $(EX2_SRCS)
	$(CC) $(CFLAGS) -o $@ $^

# Clean build
clean:
	rm -f $(EX1_BIN) $(EX2_BIN)

# Clean object files (if any)
clean_obj:
	rm -f $(EX1_DIR)/*.o $(EX2_DIR)/*.o

.PHONY: all clean clean_obj

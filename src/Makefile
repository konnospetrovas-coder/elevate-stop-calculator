CC = gcc
CFLAGS = -Os -Wall -Wextra -Werror -pedantic

# Common logic shared by both programs
COMMON_OBJ = recurse.o brute.o memoize.o dp.o

# The two different targets
TARGETS = elevate mcp

# Default rule: builds everything in the TARGETS list
all: $(TARGETS)

# Link elevate: uses common objects + elevate.o
elevate: $(COMMON_OBJ) elevate.o
	$(CC) $(COMMON_OBJ) elevate.o -o elevate

# Link mcp: uses common objects + mcp.o
mcp: $(COMMON_OBJ) mcp.o
	$(CC) $(COMMON_OBJ) mcp.o -o mcp

$(COMMON_OBJ) elevate.o mcp.o: elevate.h

# Pattern rule for all .c to .o compilation
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
CC ?= cc
AR ?= ar
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -ffunction-sections -fdata-sections -Iinclude
LDFLAGS ?= -Wl,--gc-sections

BUILD_DIR := build
OBJ_DIR := $(BUILD_DIR)/obj
LIB_DIR := $(BUILD_DIR)/lib
EXAMPLE_DIR := $(BUILD_DIR)/examples

CORE_OBJ := $(OBJ_DIR)/core.o
MEMORY_OBJ := $(OBJ_DIR)/memory.o
STRING_OBJ := $(OBJ_DIR)/string.o
IO_OBJ := $(OBJ_DIR)/io.o
CONTAINERS_OBJ := $(OBJ_DIR)/containers.o
RUNTIME_OBJ := $(OBJ_DIR)/runtime.o
ABI_OBJ := $(OBJ_DIR)/abi.o

ALL_OBJS := $(CORE_OBJ) $(MEMORY_OBJ) $(STRING_OBJ) $(IO_OBJ) $(CONTAINERS_OBJ) $(RUNTIME_OBJ) $(ABI_OBJ)

.PHONY: all clean examples minimal

all: $(LIB_DIR)/libcbang_core.a $(LIB_DIR)/libcbang_memory.a $(LIB_DIR)/libcbang_string.a \
     $(LIB_DIR)/libcbang_io.a $(LIB_DIR)/libcbang_containers.a $(LIB_DIR)/libcbang_runtime.a \
     $(LIB_DIR)/libcbang_abi.a $(LIB_DIR)/libcbang.a $(LIB_DIR)/libcbang.so examples

minimal: $(LIB_DIR)/libcbang_core.a $(LIB_DIR)/libcbang_memory.a

$(OBJ_DIR) $(LIB_DIR) $(EXAMPLE_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(LIB_DIR)/libcbang_core.a: $(CORE_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang_memory.a: $(MEMORY_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang_string.a: $(STRING_OBJ) $(MEMORY_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang_io.a: $(IO_OBJ) $(MEMORY_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang_containers.a: $(CONTAINERS_OBJ) $(MEMORY_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang_runtime.a: $(RUNTIME_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang_abi.a: $(ABI_OBJ) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang.a: $(ALL_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $^

$(LIB_DIR)/libcbang.so: $(ALL_OBJS) | $(LIB_DIR)
	$(CC) -shared -o $@ $(ALL_OBJS) $(LDFLAGS)

$(EXAMPLE_DIR)/minimal_example: examples/minimal_example.c $(CORE_OBJ) $(MEMORY_OBJ) | $(EXAMPLE_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(CORE_OBJ) $(MEMORY_OBJ) $(LDFLAGS)

$(EXAMPLE_DIR)/full_example: examples/full_example.c $(ALL_OBJS) | $(EXAMPLE_DIR)
	$(CC) $(CFLAGS) $< -o $@ $(ALL_OBJS) $(LDFLAGS)

examples: $(EXAMPLE_DIR)/minimal_example $(EXAMPLE_DIR)/full_example

clean:
	rm -rf $(BUILD_DIR)

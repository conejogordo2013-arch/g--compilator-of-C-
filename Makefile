CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g

OBJS := main.o lexer.o parser.o ast.o codegen.o
GEE := gee

SELFHOST_DIR := selfhost
SELFHOST_SRCS := $(SELFHOST_DIR)/lexer.cb $(SELFHOST_DIR)/parser.cb $(SELFHOST_DIR)/ast.cb $(SELFHOST_DIR)/codegen.cb $(SELFHOST_DIR)/main.cb
SELFHOST_ASM := $(SELFHOST_SRCS:.cb=.s)

all: stage0

stage0: $(GEE)

# Explicit convenience targets:
# - original: stage0 compiler written in C (gee)
# - advanced: self-hosted stage1 compiler (gee_stage1)
original: stage0
advanced: bootstrap

$(GEE): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

main.o: main.c parser.h ast.h codegen.h
lexer.o: lexer.c lexer.h token.h
parser.o: parser.c parser.h lexer.h ast.h token.h
ast.o: ast.c ast.h
codegen.o: codegen.c codegen.h ast.h

# Bootstrapping sequence:
# 1) Build stage0 GEE from C sources.
# 2) Use stage0 GEE to compile C! modules (*.cb) into assembly (*.s).
# 3) Assemble+link stage1 executable from generated assembly and stdlib stubs.
bootstrap: stage0 $(SELFHOST_ASM)
	$(CC) -no-pie -o gee_stage1 $(SELFHOST_ASM) stdlib/io.s stdlib/memory.s stdlib/net.s stdlib/system.s

$(SELFHOST_DIR)/%.s: $(SELFHOST_DIR)/%.cb $(GEE)
	./$(GEE) $< $@

clean:
	rm -f $(OBJS) $(GEE) gee_stage1 out.s $(SELFHOST_ASM)

.PHONY: all stage0 original advanced bootstrap clean

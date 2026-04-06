CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g

OBJS := main.o lexer.o parser.o ast.o codegen.o
GEE := gee

SELFHOST_DIR := selfhost
SELFHOST_SRCS := $(SELFHOST_DIR)/lexer.cb $(SELFHOST_DIR)/parser.cb $(SELFHOST_DIR)/ast.cb $(SELFHOST_DIR)/codegen.cb $(SELFHOST_DIR)/main.cb
SELFHOST_ASM := $(SELFHOST_SRCS:.cb=.s)

all: stage0

stage0: $(GEE)

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

# NAS advanced pipeline:
#   - Use stage0 backend in NASM mode to generate .asm
#   - Assemble with nasm and link with ld
bootstrap_nasm: stage0
	./$(GEE) -f nasm $(SELFHOST_DIR)/lexer.cb $(SELFHOST_DIR)/lexer.asm
	./$(GEE) -f nasm $(SELFHOST_DIR)/parser.cb $(SELFHOST_DIR)/parser.asm
	./$(GEE) -f nasm $(SELFHOST_DIR)/ast.cb $(SELFHOST_DIR)/ast.asm
	./$(GEE) -f nasm $(SELFHOST_DIR)/codegen.cb $(SELFHOST_DIR)/codegen.asm
	./$(GEE) -f nasm $(SELFHOST_DIR)/main.cb $(SELFHOST_DIR)/main.asm
	nasm -felf64 $(SELFHOST_DIR)/lexer.asm -o $(SELFHOST_DIR)/lexer.nasm.o
	nasm -felf64 $(SELFHOST_DIR)/parser.asm -o $(SELFHOST_DIR)/parser.nasm.o
	nasm -felf64 $(SELFHOST_DIR)/ast.asm -o $(SELFHOST_DIR)/ast.nasm.o
	nasm -felf64 $(SELFHOST_DIR)/codegen.asm -o $(SELFHOST_DIR)/codegen.nasm.o
	nasm -felf64 $(SELFHOST_DIR)/main.asm -o $(SELFHOST_DIR)/main.nasm.o
	$(CC) -c stdlib/io.s -o stdlib/io.o
	$(CC) -c stdlib/memory.s -o stdlib/memory.o
	$(CC) -c stdlib/net.s -o stdlib/net.o
	$(CC) -c stdlib/system.s -o stdlib/system.o
	$(CC) -no-pie -o gee_stage1_nasm $(SELFHOST_DIR)/lexer.nasm.o $(SELFHOST_DIR)/parser.nasm.o $(SELFHOST_DIR)/ast.nasm.o $(SELFHOST_DIR)/codegen.nasm.o $(SELFHOST_DIR)/main.nasm.o stdlib/io.o stdlib/memory.o stdlib/net.o stdlib/system.o

$(SELFHOST_DIR)/%.s: $(SELFHOST_DIR)/%.cb $(GEE)
	./$(GEE) $< $@

clean:
	rm -f $(OBJS) $(GEE) gee_stage1 gee_stage1_nasm out.s $(SELFHOST_ASM) $(SELFHOST_DIR)/*.asm $(SELFHOST_DIR)/*.nasm.o stdlib/*.o

.PHONY: all stage0 bootstrap bootstrap_nasm clean

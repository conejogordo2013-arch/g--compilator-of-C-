CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -g
PREFIX ?= /data/data/com.termux/files/usr
BINDIR ?= $(PREFIX)/bin

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

# Build ARM64 stdlib objects (for cross-linking workflows)
arm-stdlib:
	@if command -v aarch64-linux-gnu-gcc >/dev/null 2>&1; then \
		aarch64-linux-gnu-gcc -c -o stdlib/io_arm64.o stdlib/io_arm64.s && \
		aarch64-linux-gnu-gcc -c -o stdlib/memory_arm64.o stdlib/memory_arm64.s && \
		aarch64-linux-gnu-gcc -c -o stdlib/net_arm64.o stdlib/net_arm64.s && \
		aarch64-linux-gnu-gcc -c -o stdlib/system_arm64.o stdlib/system_arm64.s ; \
	else \
		echo "aarch64-linux-gnu-gcc not found; skipping ARM stdlib object build."; \
	fi

$(SELFHOST_DIR)/%.s: $(SELFHOST_DIR)/%.cb $(GEE)
	./$(GEE) $< $@

clean:
	rm -f $(OBJS) $(GEE) gee_stage1 out.s $(SELFHOST_ASM)
	rm -f stdlib/io_arm64.o stdlib/memory_arm64.o stdlib/net_arm64.o stdlib/system_arm64.o

install: stage0
	mkdir -p "$(BINDIR)"
	cp "$(GEE)" "$(BINDIR)/gee-core"
	cp "scripts/gee-frontend.sh" "$(BINDIR)/gee"
	cp "scripts/gee-target.sh" "$(BINDIR)/gee-target"
	cp "scripts/gee-doctor.sh" "$(BINDIR)/gee-doctor"
	cp "scripts/gee-new.sh" "$(BINDIR)/gee-new"
	mkdir -p "$(PREFIX)/etc/gee"
	printf "x86-64\n" > "$(PREFIX)/etc/gee/target"
	chmod 755 "$(BINDIR)/gee-core" "$(BINDIR)/gee" "$(BINDIR)/gee-target" "$(BINDIR)/gee-doctor" "$(BINDIR)/gee-new"

uninstall:
	rm -f "$(BINDIR)/gee" "$(BINDIR)/gee-core" "$(BINDIR)/gee-target" "$(BINDIR)/gee-doctor" "$(BINDIR)/gee-new" "$(BINDIR)/gee-x86" "$(BINDIR)/gee-x86-64" "$(BINDIR)/gee-arm-v7" "$(BINDIR)/gee-arm-64"
	rm -f "$(PREFIX)/etc/gee/target"

install-menu:
	bash scripts/install_menu.sh "$(if $(ARCH),$(ARCH),2)"

.PHONY: all stage0 original advanced bootstrap arm-stdlib clean install uninstall install-menu

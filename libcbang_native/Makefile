ROOT_DIR := ..
BUILD_DIR := build
LIB_DIR := $(BUILD_DIR)/lib
TARGET ?= x86-64
OUT_SO ?= $(LIB_DIR)/libcbang.so

.PHONY: all clean minimal examples test

all: $(OUT_SO)

$(LIB_DIR):
	mkdir -p $@

$(OUT_SO): | $(LIB_DIR)
	bash $(ROOT_DIR)/scripts/build-libcbang-native-shared.sh $(TARGET) $@

minimal: all

examples:
	@echo "C! portable library: sin ejemplos C nativos en libcbang_native/examples"

test: all
	bash tests/test_build_and_run.sh

clean:
	python -c "import shutil; shutil.rmtree('build', ignore_errors=True)"

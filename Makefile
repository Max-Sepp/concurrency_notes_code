CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pthread
BUILD    := build

# Every <program>.cc gets a <program>_<mode> target that compiles and runs it,
# e.g. `make blocking_ring_queue_tsan`.
PROGS := $(basename $(wildcard *.cc))

MODES := debug release ubsan tsan
FLAGS_debug   := -O0 -g
FLAGS_release := -O2 -DNDEBUG
FLAGS_ubsan   := -O1 -g -fsanitize=undefined -fno-omit-frame-pointer
FLAGS_tsan    := -O1 -g -fsanitize=thread -fno-omit-frame-pointer

CLANG_FORMAT ?= clang-format
FORMAT_SRCS  ?= $(wildcard *.cc *.h)

RUN_TARGETS := $(foreach p,$(PROGS),$(addprefix $(p)_,$(MODES)))

.PHONY: all $(MODES) uban utan $(RUN_TARGETS) format-check format clean

all: release

define PROG_MODE_RULES
$(1)_$(2): $(BUILD)/$(1)_$(2)
	./$(BUILD)/$(1)_$(2)

$(BUILD)/$(1)_$(2): $(1).cc | $(BUILD)
	$(CXX) $(CXXFLAGS) $(FLAGS_$(2)) -o $$@ $$<
endef
$(foreach p,$(PROGS),$(foreach m,$(MODES),$(eval $(call PROG_MODE_RULES,$(p),$(m)))))

# `make <mode>` compiles and runs every program in that mode.
$(MODES): %: $(addsuffix _%,$(PROGS))

uban: ubsan
utan: tsan

$(BUILD):
	mkdir -p $@

format-check:
	$(CLANG_FORMAT) --style=file --dry-run --Werror $(FORMAT_SRCS)

format:
	$(CLANG_FORMAT) --style=file -i $(FORMAT_SRCS)

clean:
	rm -rf $(BUILD)

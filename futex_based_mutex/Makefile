CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pthread
BUILD    := build
ROOT     := ..

LOCKS   := simple sleeping
THREADS ?= 8
ITERS   ?= 1000000
WORK    ?= 100

SRCS := main.cc $(wildcard *.h) $(ROOT)/lock.h
FORMAT_SRCS := main.cc $(wildcard *.h)

MODES := debug release tsan
FLAGS_debug   := -O0 -g
FLAGS_release := -O2 -g -DNDEBUG
FLAGS_tsan    := -O1 -g -fsanitize=thread -fno-omit-frame-pointer

# `make run_<lock>`, `make strace_<lock>` and `make tsan_<lock>` for each lock,
# e.g. `make strace_simple THREADS=4 ITERS=100000`.
RUN_TARGETS    := $(addprefix run_,$(LOCKS))
STRACE_TARGETS := $(addprefix strace_,$(LOCKS))
TSAN_TARGETS   := $(addprefix tsan_,$(LOCKS))

.PHONY: all run strace tsan $(RUN_TARGETS) $(STRACE_TARGETS) $(TSAN_TARGETS) \
        format clean

all: run

define MODE_RULES
$(BUILD)/mutex_bench_$(1): $(SRCS) | $(BUILD)
	$(CXX) $(CXXFLAGS) -I$(ROOT) $(FLAGS_$(1)) -o $$@ main.cc
endef
$(foreach m,$(MODES),$(eval $(call MODE_RULES,$(m))))

define LOCK_RULES
run_$(1): $(BUILD)/mutex_bench_release
	./$$< $(1) $(THREADS) $(ITERS) $(WORK)

strace_$(1): $(BUILD)/mutex_bench_release
	strace -f -c -e trace=futex ./$$< $(1) $(THREADS) 100000 $(WORK)

tsan_$(1): $(BUILD)/mutex_bench_tsan
	./$$< $(1) $(THREADS) 100000 $(WORK)
endef
$(foreach l,$(LOCKS),$(eval $(call LOCK_RULES,$(l))))

# Run every lock: correctness + timing, futex syscall counts, or TSan.
run:    $(RUN_TARGETS)
strace: $(STRACE_TARGETS)
tsan:   $(TSAN_TARGETS)

# Format the sources in place with the repo's .clang-format (Google style).
format:
	clang-format -i $(FORMAT_SRCS)

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

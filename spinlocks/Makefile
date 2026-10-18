CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pthread
BUILD    := build

LOCKS   := simple local
THREADS ?= 8
ITERS   ?= 1000000
WORK    ?= 100
EVENTS  ?= cache-references,cache-misses,L1-dcache-loads,L1-dcache-load-misses,cycles,instructions

SRCS := main.cc $(wildcard *.h)

MODES := debug release tsan
FLAGS_debug   := -O0 -g
FLAGS_release := -O2 -g -DNDEBUG
FLAGS_tsan    := -O1 -g -fsanitize=thread -fno-omit-frame-pointer

# `make run_<lock>`, `make perf_<lock>`, `make c2c_<lock>` and `make tsan_<lock>`
# for each lock, e.g. `make perf_local THREADS=4 WORK=200`.
RUN_TARGETS  := $(addprefix run_,$(LOCKS))
PERF_TARGETS := $(addprefix perf_,$(LOCKS))
C2C_TARGETS  := $(addprefix c2c_,$(LOCKS))
TSAN_TARGETS := $(addprefix tsan_,$(LOCKS))

.PHONY: all run perf c2c tsan $(RUN_TARGETS) $(PERF_TARGETS) $(C2C_TARGETS) \
        $(TSAN_TARGETS) clean

all: run

define MODE_RULES
$(BUILD)/spinlock_bench_$(1): $(SRCS) | $(BUILD)
	$(CXX) $(CXXFLAGS) $(FLAGS_$(1)) -o $$@ main.cc
endef
$(foreach m,$(MODES),$(eval $(call MODE_RULES,$(m))))

define LOCK_RULES
run_$(1): $(BUILD)/spinlock_bench_release
	./$$< $(1) $(THREADS) $(ITERS) $(WORK)

perf_$(1): $(BUILD)/spinlock_bench_release
	perf stat -e $(EVENTS) ./$$< $(1) $(THREADS) $(ITERS) $(WORK)

c2c_$(1): $(BUILD)/spinlock_bench_release
	perf c2c record -o $(BUILD)/c2c_$(1).data ./$$< $(1) $(THREADS) $(ITERS) $(WORK)
	perf c2c report -i $(BUILD)/c2c_$(1).data --stdio

tsan_$(1): $(BUILD)/spinlock_bench_tsan
	./$$< $(1) $(THREADS) 100000 $(WORK)
endef
$(foreach l,$(LOCKS),$(eval $(call LOCK_RULES,$(l))))

# Run every lock: correctness + timing, perf counters, c2c, or TSan.
run:  $(RUN_TARGETS)
perf: $(PERF_TARGETS)
c2c:  $(C2C_TARGETS)
tsan: $(TSAN_TARGETS)

$(BUILD):
	mkdir -p $@

clean:
	rm -rf $(BUILD)

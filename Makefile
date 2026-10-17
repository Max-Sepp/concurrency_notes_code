CXX      ?= g++
CXXFLAGS ?= -std=c++20 -Wall -Wextra -pthread
SRC      ?= sum_in_parallel.cc
BIN      := $(basename $(SRC))

CLANG_FORMAT ?= clang-format
FORMAT_SRCS  ?= $(wildcard *.cc *.h)

.PHONY: all debug release ubsan tsan uban utan format-check format clean

all: release

debug: $(SRC)
	$(CXX) $(CXXFLAGS) -O0 -g -o $(BIN)_debug $<
	./$(BIN)_debug

release: $(SRC)
	$(CXX) $(CXXFLAGS) -O2 -DNDEBUG -o $(BIN)_release $<
	./$(BIN)_release

ubsan: $(SRC)
	$(CXX) $(CXXFLAGS) -O1 -g -fsanitize=undefined -fno-omit-frame-pointer -o $(BIN)_ubsan $<
	./$(BIN)_ubsan

tsan: $(SRC)
	$(CXX) $(CXXFLAGS) -O1 -g -fsanitize=thread -fno-omit-frame-pointer -o $(BIN)_tsan $<
	./$(BIN)_tsan

format-check:
	$(CLANG_FORMAT) --style=file --dry-run --Werror $(FORMAT_SRCS)

format:
	$(CLANG_FORMAT) --style=file -i $(FORMAT_SRCS)

clean:
	rm -f $(BIN)_debug $(BIN)_release $(BIN)_ubsan $(BIN)_tsan

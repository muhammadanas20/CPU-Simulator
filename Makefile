CXX ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -O2
HEADERS := $(shell find src -name '*.h')

all: cpu_ds cpu_ds_tests

cpu_ds: src/main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ src/main.cpp

cpu_ds_tests: tests/test_main.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ tests/test_main.cpp

test: cpu_ds_tests
	./cpu_ds_tests

run: cpu_ds
	./cpu_ds

clean:
	rm -f cpu_ds cpu_ds_tests
	rm -rf tests/tmp

.PHONY: all test run clean

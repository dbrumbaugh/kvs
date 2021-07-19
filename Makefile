# Makefile adapted from Zed Shaw's book,
# Learn C the Hard Way, Ex. 28 (pg 153-154)

CXXFLAGS = -Wall -Wextra -Iinclude -faligned-new -ggdb $(OPTFLAGS)
LDFLAGS = $(OPTLIBS)
LDLIBS = -lcheck -lm -pthread -lrt -lsubunit

SOURCES = $(wildcard src/**/*.cpp src/*.cpp)
OBJECTS = $(patsubst src/%.cpp,build/%.o,$(SOURCES))

TEST_SRC = $(wildcard tests/*_tests.cpp)
TESTS = $(patsubst %.cpp,%,$(TEST_SRC))

BENCH_SRC = $(wildcard benchmarks/*_bench.cpp)
BENCHMARKS = $(patsubst %.cpp,%,$(BENCH_SRC))

TARGET = lib/libkvs.a

all: $(TARGET) tests benchmarks

.PHONY: build
build:
	mkdir -p build/io
	mkdir -p bin
	mkdir -p lib

build/%.o: src/%.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LDLIBS) -c $< -o $@

$(TARGET): build $(OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@

.PHONY: tests
tests: LDLIBS += $(TARGET)
tests: $(TESTS)
	sh ./tests/unit-tests.sh

.PHONY: benchmarks
benchmarks: $(BENCHMARKS)
	ksh ./benchmarks/bench_run.sh

clean:
	rm -rf $(TARGET)
	rm -rf build $(OBJECTS) $(TESTS) $(BENCHMARKS)
	rm -f tests/tests.log
	rm -f benchmarks/*.log
	rm -rf tests/data

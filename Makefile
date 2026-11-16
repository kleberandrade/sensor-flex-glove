CXX ?= c++
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -pedantic -Iinclude
.PHONY: test example clean
build:
	mkdir -p build
build/core_test: tests/core_test.cpp include/flex/core.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
test: build/core_test
	./build/core_test
build/readings: examples/readings.cpp include/flex/core.hpp | build
	$(CXX) $(CXXFLAGS) $< -o $@
example: build/readings
	./build/readings
clean:
	rm -rf build

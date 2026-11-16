CXX ?= c++
BUILD_DIR ?= build
TEST_SOURCES := $(wildcard tests/*_test.cpp)
TEST_BINS := $(patsubst tests/%.cpp,$(BUILD_DIR)/%,$(TEST_SOURCES))
CPPFLAGS = -Itests/support
CXXFLAGS = -std=c++11 -Wall -Wextra -Werror -pedantic -Iinclude
.PHONY: test example clean
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)
$(BUILD_DIR)/%_test: tests/%_test.cpp $(wildcard include/flex/*.hpp) $(wildcard tests/support/*.h) firmware/receiver/main.cpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@
test: $(TEST_BINS)
	@set -e; for test_bin in $(TEST_BINS); do ./$$test_bin; done
$(BUILD_DIR)/readings: examples/readings.cpp include/flex/core.hpp | $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $< -o $@
example: $(BUILD_DIR)/readings
	./$(BUILD_DIR)/readings
clean:
	rm -rf build

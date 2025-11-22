# Makefile for GS308EP CLI
# Supports native builds and cross-compilation

# Project configuration
PROJECT = gs308ep
VERSION = 0.5.0

# Directories
SRC_DIR = src
BUILD_DIR = build
TEST_DIR = test
INSTALL_PREFIX = /usr/local

# Compiler and flags
CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2
LDFLAGS = -lcurl -lssl -lcrypto
TEST_LDFLAGS = -lssl -lcrypto

# Source files
SOURCES = $(SRC_DIR)/main.cpp $(SRC_DIR)/GS308EP_CLI.cpp
HEADERS = $(SRC_DIR)/GS308EP_CLI.h
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
TARGET = $(BUILD_DIR)/$(PROJECT)

# Test files
TEST_SOURCES = $(TEST_DIR)/test_gs308ep_cli.cpp
TEST_OBJECTS = $(patsubst $(TEST_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(TEST_SOURCES))
TEST_TARGET = $(BUILD_DIR)/test_runner

# Platform detection
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    # macOS-specific flags (check both Intel and ARM paths)
    BREW_PREFIX := $(shell brew --prefix 2>/dev/null || echo "/usr/local")
    OPENSSL_PREFIX := $(shell brew --prefix openssl 2>/dev/null || echo "/usr/local/opt/openssl")
    CURL_PREFIX := $(shell brew --prefix curl 2>/dev/null || echo "/usr/local/opt/curl")
    CXXFLAGS += -I$(OPENSSL_PREFIX)/include -I$(CURL_PREFIX)/include
    LDFLAGS += -L$(OPENSSL_PREFIX)/lib -L$(CURL_PREFIX)/lib
    TEST_LDFLAGS += -L$(OPENSSL_PREFIX)/lib
endif

# Default target
.PHONY: all
all: $(TARGET)

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Compile object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp $(HEADERS) | $(BUILD_DIR)
	@echo "CXX     $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Link executable
$(TARGET): $(OBJECTS)
	@echo "LINK    $@"
	@$(CXX) $(OBJECTS) $(LDFLAGS) -o $@
	@echo "Build complete: $@"

# Compile test object files
$(BUILD_DIR)/%.o: $(TEST_DIR)/%.cpp $(HEADERS) | $(BUILD_DIR)
	@echo "CXX     $<"
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Link test executable
$(TEST_TARGET): $(TEST_OBJECTS)
	@echo "LINK    $@"
	@$(CXX) $(TEST_OBJECTS) $(TEST_LDFLAGS) -o $@
	@echo "Test build complete: $@"

# Install target
.PHONY: install
install: $(TARGET)
	@echo "Installing $(PROJECT) to $(INSTALL_PREFIX)/bin"
	@install -d $(INSTALL_PREFIX)/bin
	@install -m 755 $(TARGET) $(INSTALL_PREFIX)/bin/$(PROJECT)
	@echo "Installation complete"

# Uninstall target
.PHONY: uninstall
uninstall:
	@echo "Removing $(INSTALL_PREFIX)/bin/$(PROJECT)"
	@rm -f $(INSTALL_PREFIX)/bin/$(PROJECT)
	@echo "Uninstall complete"

# Build tests
.PHONY: build-tests
build-tests: $(TEST_TARGET)

# Run tests
.PHONY: test-run
test-run: $(TEST_TARGET)
	@echo "Running unit tests..."
	@$(TEST_TARGET)

# Clean build artifacts
.PHONY: clean
clean:
	@echo "Cleaning build artifacts"
	@rm -rf $(BUILD_DIR)
	@echo "Clean complete"

# Static analysis with cppcheck (if available)
.PHONY: check
check:
	@if command -v cppcheck >/dev/null 2>&1; then \
		echo "Running cppcheck..."; \
		cppcheck --enable=warning,style,performance --std=c++17 $(SRC_DIR); \
	else \
		echo "cppcheck not found, skipping static analysis"; \
	fi

# Run the program (requires GS308EP_HOST and GS308EP_PASSWORD)
.PHONY: run
run: $(TARGET)
	@$(TARGET) --help

# Test build and run (full test suite)
.PHONY: test
test: build-tests test-run

# Display help
.PHONY: help
help:
	@echo "GS308EP CLI Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all         - Build the executable (default)"
	@echo "  test        - Build and run unit tests"
	@echo "  build-tests - Build test executable only"
	@echo "  test-run    - Run tests (requires build-tests)"
	@echo "  install     - Install to $(INSTALL_PREFIX)/bin (may require sudo)"
	@echo "  uninstall   - Remove from $(INSTALL_PREFIX)/bin"
	@echo "  clean       - Remove build artifacts"
	@echo "  check       - Run static analysis (requires cppcheck)"
	@echo "  run         - Build and display help"
	@echo "  help        - Display this help message"
	@echo ""
	@echo "Variables:"
	@echo "  CXX            - C++ compiler (default: g++)"
	@echo "  CXXFLAGS       - Compiler flags"
	@echo "  LDFLAGS        - Linker flags"
	@echo "  INSTALL_PREFIX - Installation prefix (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make                          - Build"
	@echo "  make CXX=clang++              - Build with clang"
	@echo "  make INSTALL_PREFIX=~/.local  - Install to ~/.local/bin"
	@echo "  sudo make install             - Install system-wide"

# Print version
.PHONY: version
version:
	@echo "$(PROJECT) version $(VERSION)"

# Dependencies (automatically generated)
-include $(OBJECTS:.o=.d)

# Generate dependency files
$(BUILD_DIR)/%.d: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	@$(CXX) $(CXXFLAGS) -MM -MT $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$<) $< > $@

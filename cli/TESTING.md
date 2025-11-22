# GS308EP CLI Testing

Unit test suite for the GS308EP CLI tool.

## Overview

The test suite validates HTML parsing, port validation, and data extraction logic without requiring network access or a real switch.

**Test Coverage:**
- HTML parsing (rand tokens, cookies, client hashes)
- Port power extraction from status pages
- Port number validation (1-8)
- Edge cases (malformed HTML, empty values, whitespace)
- Multiple port handling

**Test Count:** 30 tests

## Running Tests

### Quick Test
```bash
cd cli
make test
```

### Build Tests Only
```bash
make build-tests
```

### Run Pre-built Tests
```bash
make test-run
# or directly:
./build/test_runner
```

### Clean and Test
```bash
make clean test
```

## Test Categories

### HTML Parsing Tests (10 tests)
- Extract rand token from login page (double/single/mixed quotes)
- Extract SID cookie from HTTP headers
- Extract client hash from config page
- Handle missing fields and malformed HTML

### Port Power Extraction Tests (7 tests)
- Extract power consumption (watts) from status page
- Handle multiple ports in single response
- Parse zero and high power values
- Handle invalid formats and missing data

### Port Validation Tests (4 tests)
- Validate port numbers 1-8 (valid)
- Reject 0, negative, and >8 values (invalid)

### Complex/Edge Case Tests (9 tests)
- Surrounding HTML context
- Multiple cookies in headers
- Whitespace in values
- Decimal precision
- Empty values
- Malformed HTML structures

## Test Output

**Success:**
```
==================================
GS308EP CLI Unit Tests
==================================

Running test: extract_rand_double_quotes... PASSED
Running test: extract_rand_single_quotes... PASSED
...
==================================
Test Results:
  Passed: 30
  Failed: 0
  Total:  30
==================================
```

**Failure Example:**
```
Running test: extract_rand_double_quotes... FAILED: Expected 1735414426 but got 1234567890
```

Exit code: 0 for success, 1 for any failures

## CI Integration

Tests run automatically in CI for:
- Native builds (Ubuntu, macOS)
- Docker builds (validates in container)

See `.github/workflows/build-cli.yml` for CI configuration.

## Adding New Tests

1. Add test function in `test/test_gs308ep_cli.cpp`:
```cpp
TEST(my_new_test) {
    std::string result = someFunction("input");
    ASSERT_EQ(std::string("expected"), result);
}
```

2. Register test in `main()`:
```cpp
run_test_my_new_test();
```

3. Run tests:
```bash
make test
```

## Test Macros

- `ASSERT_TRUE(condition)` - Condition must be true
- `ASSERT_FALSE(condition)` - Condition must be false
- `ASSERT_EQ(expected, actual)` - Values must be equal
- `ASSERT_NEAR(expected, actual, epsilon)` - Floats within epsilon
- `ASSERT_CONTAINS(haystack, needle)` - String contains substring

## Dependencies

**Required:**
- C++17 compiler
- OpenSSL (for MD5, even though tests don't hash)
- Standard library

**Not required for tests:**
- libcurl (tests don't make HTTP calls)
- Network access
- Real GS308EP switch

## Test Architecture

Tests use a `GS308EP_CLI_Testable` wrapper class that exposes private parsing methods for unit testing without modifying the main implementation.

This approach:
- ✅ Keeps production code clean
- ✅ Tests actual implementation logic
- ✅ No network mocking required
- ✅ Fast execution (~0.1s)
- ✅ Portable across platforms

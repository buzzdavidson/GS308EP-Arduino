# GS308EP Library Testing Guide

This document explains how to run the test suite for the GS308EP library.

## Test Framework

The library uses [PlatformIO](https://platformio.org/) with the Unity test framework for unit testing. Tests validate:

- HTML parsing logic (extracting rand tokens, cookies, hashes)
- MD5 hashing and merge_hash algorithm
- Port validation
- Authentication state management
- Edge cases and error handling

## Quick Start

### Using Docker (Recommended for CI)

The easiest way to run tests is using Docker:

```bash
# Build and run tests
docker build -t gs308ep-test .
docker run --rm gs308ep-test
```

Or use the test runner script:

```bash
chmod +x test_runner.sh
./test_runner.sh --docker
```

### Using Local PlatformIO

If you have PlatformIO installed locally:

```bash
# Install dependencies
pio platform install native espressif32

# Run tests
pio test -e native

# Run with verbose output
pio test -e native -v
```

Or use the test runner script:

```bash
chmod +x test_runner.sh
./test_runner.sh --local
```

## Architecture

The testing infrastructure is designed to validate library logic without requiring Arduino hardware:

- **Test Header (`test/GS308EP_test.h`)**: Standalone implementation of parsing and validation logic with minimal dependencies (OpenSSL for MD5)
- **Test File (`test/test_gs308ep.cpp`)**: 31 unit tests covering all critical functionality
- **Native Testing**: Uses standard C++ without Arduino framework for fast CI execution
- **Containerization**: Docker provides consistent environment across all platforms

### Why Separate Test Implementation?

The main library (`src/GS308EP.*`) depends on Arduino/ESP32-specific libraries (WiFiClient, HTTPClient, MD5Builder). For unit testing, we created a test-specific version that:
- Uses standard C++ `std::string` instead of Arduino `String`
- Uses OpenSSL's MD5 instead of `MD5Builder`
- Exposes the same parsing logic for validation
- Runs natively without embedded hardware

## Test Organization

Tests are organized in `/test/test_gs308ep.cpp` with the following sections:

### 1. HTML Parsing Tests
- `test_extractRand_*` - Tests for extracting rand tokens from login pages
- `test_extractCookie_*` - Tests for extracting SID cookies from headers
- `test_extractClientHash_*` - Tests for extracting client hashes from forms

### 2. Cryptographic Tests
- `test_md5Hash_*` - MD5 hashing validation
- `test_mergeHash_*` - merge_hash algorithm testing (password + rand)

### 3. Port Validation Tests
- `test_isValidPort_*` - Port number validation (1-8 range)

### 4. Authentication Tests
- `test_isAuthenticated_*` - Authentication state tracking
- `test_begin_*` - Initialization tests

### 5. Integration Tests
- `test_fullLoginPage_parsing` - Complete login page parsing
- `test_fullPoEConfig_parsing` - Complete PoE config page parsing

### 6. Edge Cases
- Malformed HTML handling
- Special characters in passwords
- Multiple cookie handling

## Running Tests in CI/CD

### GitHub Actions

The repository includes a GitHub Actions workflow (`.github/workflows/test.yml`) that:

1. Builds a Docker container with all dependencies
2. Runs the test suite
3. Reports results

Tests run automatically on:
- Push to `main` or `develop` branches
- Pull requests to `main` or `develop` branches

### GitLab CI

Create a `.gitlab-ci.yml` file:

```yaml
test:
  image: python:3.11-slim
  before_script:
    - pip install platformio
    - pio platform install native
  script:
    - pio test -e native -v
```

### Jenkins

Use the Docker image in a Jenkins pipeline:

```groovy
pipeline {
    agent {
        docker {
            image 'python:3.11-slim'
        }
    }
    stages {
        stage('Test') {
            steps {
                sh 'pip install platformio'
                sh 'pio platform install native'
                sh 'pio test -e native -v'
            }
        }
    }
}
```

## Test Environments

### Native Environment (`env:native`)
- Runs tests on the host machine
- Fast execution
- Best for development and CI
- Uses native C++ compiler

### ESP32 Environment (`env:esp32`)
- Compiles for actual ESP32 hardware
- Requires ESP32 board connected
- Run with: `pio test -e esp32`

## Writing New Tests

To add new tests:

1. Add test function to `test/test_gs308ep.cpp`:
```cpp
void test_myNewFeature(void) {
    // Arrange
    GS308EPTestable* sw = new GS308EPTestable("192.168.1.1", "pass");
    
    // Act
    bool result = sw->someMethod();
    
    // Assert
    TEST_ASSERT_TRUE(result);
    
    delete sw;
}
```

2. Register test in `setup()`:
```cpp
void setup() {
    UNITY_BEGIN();
    // ... existing tests ...
    RUN_TEST(test_myNewFeature);
    UNITY_END();
}
```

3. Run tests to verify:
```bash
pio test -e native
```

## Test Coverage

Current test coverage includes:

✅ HTML parsing (rand, cookies, hashes)
✅ MD5 hashing algorithms  
✅ Port validation logic
✅ Authentication state
✅ Edge cases and error handling

Not covered (requires hardware/network):
- Actual HTTP requests
- Real switch communication
- Network error scenarios
- Cookie persistence

## Debugging Failed Tests

### View Detailed Output
```bash
pio test -e native -vv
```

### Run Specific Test
Modify `setup()` to only run one test:
```cpp
void setup() {
    UNITY_BEGIN();
    RUN_TEST(test_specific_function);
    UNITY_END();
}
```

### Enable Debug Prints
Add debug output in test functions:
```cpp
void test_something(void) {
    String result = testSwitch->someMethod();
    Serial.println("Result: " + result);  // Debug output
    TEST_ASSERT_NOT_EQUAL_STRING("", result.c_str());
}
```

## Docker Container Details

The test Docker container:
- Based on `python:3.11-slim`
- Includes PlatformIO and all dependencies
- Installs native and ESP32 platforms
- Can be used for local testing or CI

### Build Container Locally
```bash
docker build -t gs308ep-test .
```

### Run Tests Interactively
```bash
docker run -it --rm gs308ep-test /bin/bash
# Inside container:
pio test -e native -vv
```

### Mount Local Code
For development:
```bash
docker run -it --rm -v $(pwd):/workspace gs308ep-test pio test -e native
```

## Continuous Integration Status

Tests should pass before merging:
- ✅ All HTML parsing tests
- ✅ All hash algorithm tests  
- ✅ All validation tests
- ✅ Docker build succeeds
- ✅ No compiler warnings

## Troubleshooting

### PlatformIO Not Found
```bash
pip install platformio
```

### Platform Install Fails
```bash
pio platform uninstall native
pio platform install native --force
```

### Tests Fail in Docker but Pass Locally
- Check Docker image is up to date
- Verify Dockerfile matches local environment
- Check for path or dependency differences

### Permission Denied on test_runner.sh
```bash
chmod +x test_runner.sh
```

## Further Reading

- [PlatformIO Unit Testing](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html)
- [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity)
- [Arduino Unit Testing Best Practices](https://docs.arduino.cc/learn/contributions/arduino-library-style-guide/)

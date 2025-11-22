#!/bin/bash
# Test runner script for GS308EP library
# Can be used locally or in CI/CD pipelines

set -e  # Exit on error

echo "=================================="
echo "GS308EP Library Test Suite"
echo "=================================="
echo ""

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Check if Docker is available
if command -v docker &> /dev/null; then
    echo -e "${GREEN}✓${NC} Docker found"
    USE_DOCKER=true
else
    echo -e "${YELLOW}⚠${NC} Docker not found, will use local PlatformIO"
    USE_DOCKER=false
fi

# Function to run tests with Docker
run_tests_docker() {
    echo ""
    echo "Building Docker image..."
    docker build -t gs308ep-test . || {
        echo -e "${RED}✗${NC} Docker build failed"
        exit 1
    }
    
    echo ""
    echo "Running tests in Docker container..."
    docker run --rm gs308ep-test || {
        echo -e "${RED}✗${NC} Tests failed"
        exit 1
    }
    
    echo -e "${GREEN}✓${NC} All tests passed!"
}

# Function to run tests with local PlatformIO
run_tests_local() {
    # Check if PlatformIO is installed
    if ! command -v pio &> /dev/null; then
        echo -e "${RED}✗${NC} PlatformIO not found. Please install it:"
        echo "  pip install platformio"
        exit 1
    fi
    
    echo -e "${GREEN}✓${NC} PlatformIO found"
    
    echo ""
    echo "Installing dependencies..."
    pio platform install native espressif32 || {
        echo -e "${RED}✗${NC} Failed to install platforms"
        exit 1
    }
    
    echo ""
    echo "Running tests..."
    pio test -e native -v || {
        echo -e "${RED}✗${NC} Tests failed"
        exit 1
    }
    
    echo -e "${GREEN}✓${NC} All tests passed!"
}

# Parse command line arguments
FORCE_LOCAL=false
VERBOSE=false

while [[ $# -gt 0 ]]; do
    case $1 in
        --local)
            FORCE_LOCAL=true
            shift
            ;;
        --docker)
            USE_DOCKER=true
            FORCE_LOCAL=false
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo ""
            echo "Options:"
            echo "  --docker    Force Docker-based testing"
            echo "  --local     Force local PlatformIO testing"
            echo "  -v, --verbose  Verbose output"
            echo "  -h, --help  Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use --help for usage information"
            exit 1
            ;;
    esac
done

# Run tests
if [ "$FORCE_LOCAL" = true ]; then
    run_tests_local
elif [ "$USE_DOCKER" = true ]; then
    run_tests_docker
else
    run_tests_local
fi

echo ""
echo "=================================="
echo "Test suite completed successfully!"
echo "=================================="

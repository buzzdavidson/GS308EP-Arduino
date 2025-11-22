#!/usr/bin/env bash
# Build script for GS308EP CLI
# Supports native builds, Docker builds, and cross-compilation

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Default values
BUILD_TYPE="native"
VERBOSE=false
CLEAN=false
SKIP_TESTS=false

# Print usage
usage() {
    cat << EOF
Usage: $0 [OPTIONS]

Build GS308EP CLI tool using various methods.

OPTIONS:
    -t, --type TYPE     Build type: native, docker, platformio (default: native)
    -c, --clean         Clean before building
    -s, --skip-tests    Skip running tests
    -v, --verbose       Enable verbose output
    -h, --help          Display this help message

BUILD TYPES:
    native      - Use system compiler and Makefile
    docker      - Build using Docker (produces standalone image)
    platformio  - Use PlatformIO build system

EXAMPLES:
    $0                          # Native build with make
    $0 --type docker            # Build Docker image
    $0 --clean --type native    # Clean and rebuild natively
    $0 --type platformio -v     # Build with PlatformIO (verbose)

EOF
    exit 0
}

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -t|--type)
            BUILD_TYPE="$2"
            shift 2
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -s|--skip-tests)
            SKIP_TESTS=true
            shift
            ;;
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo -e "${RED}Error: Unknown option $1${NC}"
            usage
            ;;
    esac
done

# Logging functions
log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Clean function
do_clean() {
    if [ "$CLEAN" = true ]; then
        log_info "Cleaning build artifacts..."
        case $BUILD_TYPE in
            native)
                make clean 2>/dev/null || true
                ;;
            docker)
                docker rmi gs308ep-cli:latest 2>/dev/null || true
                rm -rf build/
                ;;
            platformio)
                pio run --target clean 2>/dev/null || true
                ;;
        esac
    fi
}

# Native build with Makefile
build_native() {
    log_info "Building with native compiler..."
    
    # Check for required dependencies
    if ! command -v g++ >/dev/null 2>&1 && ! command -v clang++ >/dev/null 2>&1; then
        log_error "C++ compiler not found. Please install build-essential or equivalent."
        exit 1
    fi
    
    # On macOS, check for Homebrew packages
    if [[ "$OSTYPE" == "darwin"* ]]; then
        if ! brew list openssl &>/dev/null && ! brew list openssl@3 &>/dev/null; then
            log_warn "OpenSSL not found via Homebrew. Attempting build anyway..."
        fi
        if ! brew list curl &>/dev/null; then
            log_warn "curl not found via Homebrew. Attempting build anyway..."
        fi
    else
        # On Linux, check pkg-config
        if command -v pkg-config >/dev/null 2>&1; then
            if ! pkg-config --exists libcurl 2>/dev/null; then
                log_error "libcurl not found. Please install libcurl4-openssl-dev or equivalent."
                exit 1
            fi
            
            if ! pkg-config --exists openssl 2>/dev/null; then
                log_error "OpenSSL not found. Please install libssl-dev or equivalent."
                exit 1
            fi
        fi
    fi
    
    # Build
    if [ "$VERBOSE" = true ]; then
        make
    else
        make -s
    fi
    
    log_info "Build complete: build/gs308ep"
    
    # Test the binary
    if [ -f build/gs308ep ]; then
        ./build/gs308ep --version
    else
        log_error "Binary not found after build"
        exit 1
    fi
    
    # Run unit tests
    if [ "$SKIP_TESTS" = false ]; then
        log_info "Running unit tests..."
        if make test-run; then
            log_info "All tests passed"
        else
            log_error "Tests failed"
            exit 1
        fi
    else
        log_info "Skipping tests (--skip-tests)"
    fi
}

# Docker build
build_docker() {
    log_info "Building Docker image..."
    
    if ! command -v docker >/dev/null 2>&1; then
        log_error "Docker not found. Please install Docker."
        exit 1
    fi
    
    # Build image
    if [ "$VERBOSE" = true ]; then
        docker build -t gs308ep-cli:latest .
    else
        docker build -t gs308ep-cli:latest . > /dev/null
    fi
    
    log_info "Docker image built: gs308ep-cli:latest"
    
    # Test the image
    log_info "Testing Docker image..."
    docker run --rm gs308ep-cli:latest --version
    
    log_info ""
    log_info "To use the Docker image:"
    log_info "  docker run --rm gs308ep-cli:latest --host 192.168.1.1 --password admin --help"
    log_info ""
    log_info "Or with environment variables:"
    log_info "  docker run --rm -e GS308EP_HOST=192.168.1.1 -e GS308EP_PASSWORD=admin gs308ep-cli:latest -W"
}

# PlatformIO build
build_platformio() {
    log_info "Building with PlatformIO..."
    
    if ! command -v pio >/dev/null 2>&1; then
        log_error "PlatformIO not found. Please install PlatformIO."
        exit 1
    fi
    
    # Build
    if [ "$VERBOSE" = true ]; then
        pio run
    else
        pio run -s
    fi
    
    log_info "Build complete: .pio/build/native/program"
    
    # Test the binary
    if [ -f .pio/build/native/program ]; then
        .pio/build/native/program --version
    else
        log_error "Binary not found after build"
        exit 1
    fi
}

# Main execution
main() {
    log_info "GS308EP CLI Build Script"
    log_info "Build type: $BUILD_TYPE"
    
    do_clean
    
    case $BUILD_TYPE in
        native)
            build_native
            ;;
        docker)
            build_docker
            ;;
        platformio)
            build_platformio
            ;;
        *)
            log_error "Unknown build type: $BUILD_TYPE"
            usage
            ;;
    esac
    
    log_info "Build successful!"
}

# Run main
main

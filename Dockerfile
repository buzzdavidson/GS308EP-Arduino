# Dockerfile for GS308EP library testing
# Provides a complete PlatformIO environment for CI/CD testing

FROM python:3.11-slim

LABEL maintainer="GS308EP Library"
LABEL description="Docker container for testing GS308EP Arduino library"

# Install system dependencies
RUN apt-get update && apt-get install -y \
 git \
 build-essential \
 curl \
 libssl-dev \
 && rm -rf /var/lib/apt/lists/*

# Install PlatformIO
RUN pip install --no-cache-dir platformio

# Create working directory
WORKDIR /workspace

# Copy library files
COPY . /workspace/

# Set PlatformIO home directory
ENV PLATFORMIO_CORE_DIR=/workspace/.platformio

# Install PlatformIO dependencies and platforms
RUN pio platform install native espressif32

# Default command runs tests
CMD ["pio", "test", "-e", "native", "-v"]

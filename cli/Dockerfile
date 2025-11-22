# Multi-stage Dockerfile for building GS308EP CLI
# Produces a minimal runtime image with just the compiled binary

# Build stage
FROM debian:bookworm-slim AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
 build-essential \
 g++ \
 make \
 libcurl4-openssl-dev \
 libssl-dev \
 pkg-config \
 && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /build

# Copy source files
COPY src/ ./src/
COPY test/ ./test/
COPY Makefile ./

# Build the binary and run tests
RUN make clean && make
RUN make test

# Verify the binary was created
RUN test -f build/gs308ep && echo "Build successful"

# Runtime stage
FROM debian:bookworm-slim

# Install runtime dependencies only
RUN apt-get update && apt-get install -y \
 libcurl4 \
 libssl3 \
 ca-certificates \
 && rm -rf /var/lib/apt/lists/*

# Copy the compiled binary from builder stage
COPY --from=builder /build/build/gs308ep /usr/local/bin/gs308ep

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/gs308ep"]

# Default command shows help
CMD ["--help"]

# Labels
LABEL maintainer="Steve"
LABEL version="0.5.0"
LABEL description="Netgear GS308EP PoE Switch Control CLI"

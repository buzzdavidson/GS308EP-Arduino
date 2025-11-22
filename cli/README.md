# GS308EP CLI Tool

Command-line interface for controlling Netgear GS308EP PoE switch.

## Building

### Requirements
- C++ compiler with C++17 support
- libcurl development headers
- OpenSSL development headers
- PlatformIO (optional, for automated building)

### Install Dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install libcurl4-openssl-dev libssl-dev
```

**macOS:**
```bash
brew install curl openssl
```

**Fedora/RHEL:**
```bash
sudo dnf install libcurl-devel openssl-devel
```

### Build with Make

```bash
cd cli
make
```

The binary will be in `build/gs308ep`

### Run Tests

```bash
cd cli
make test
```

All 30 unit tests should pass. See [TESTING.md](TESTING.md) for details.

### Build with PlatformIO

```bash
cd cli
pio run
```

The binary will be in `.pio/build/native/program`

## Installation

Copy the binary to a directory in your PATH:

```bash
sudo cp .pio/build/native/program /usr/local/bin/gs308ep
sudo chmod +x /usr/local/bin/gs308ep
```

## Usage

### Basic Examples

**Turn on port 3:**
```bash
gs308ep --host 192.168.1.1 --password admin --port 3 --on
```

**Turn off port 5:**
```bash
gs308ep -h 192.168.1.1 -p admin -P 5 -f
```

**Power cycle port 2 (with 3 second delay):**
```bash
gs308ep -h 192.168.1.1 -p admin -P 2 -c 3000
```

**Check port 4 status:**
```bash
gs308ep -h 192.168.1.1 -p admin -P 4 -s
```

**Show port 1 power consumption:**
```bash
gs308ep -h 192.168.1.1 -p admin -P 1 -w
```

**Show total power consumption:**
```bash
gs308ep -h 192.168.1.1 -p admin -W
```

**Show all port statistics:**
```bash
gs308ep -h 192.168.1.1 -p admin -S
```

### Environment Variables

Set credentials as environment variables to avoid typing them repeatedly:

```bash
export GS308EP_HOST=192.168.1.1
export GS308EP_PASSWORD=admin

# Now commands are simpler
gs308ep -P 3 -o
gs308ep -W
gs308ep -S
```

### JSON Output

Use `--json` for machine-readable output:

```bash
gs308ep -h 192.168.1.1 -p admin -P 3 -s --json
# Output: {"port":3,"status":"on"}

gs308ep -h 192.168.1.1 -p admin -W --json
# Output: {"total_power":15.6,"max_power":65.0}

gs308ep -h 192.168.1.1 -p admin -S --json
# Output: {"ports":[...], "total_power":15.6}
```

### Verbose Mode

Enable verbose output for debugging:

```bash
gs308ep -h 192.168.1.1 -p admin -P 3 -o -v
```

## Command Reference

### Required Options

| Option | Description |
|--------|-------------|
| `-h, --host=HOST` | Switch IP address or hostname |
| `-p, --password=PASS` | Administrator password |

### Port Control

| Option | Description |
|--------|-------------|
| `-P, --port=NUM` | Port number (1-8) |
| `-o, --on` | Turn port ON |
| `-f, --off` | Turn port OFF |
| `-c, --cycle[=DELAY]` | Power cycle port (optional delay in ms, default 2000) |
| `-s, --status` | Show port status |

### Power Monitoring

| Option | Description |
|--------|-------------|
| `-w, --power` | Show power consumption for specified port |
| `-W, --total-power` | Show total power consumption |
| `-S, --stats` | Show comprehensive statistics for all ports |

### Output Format

| Option | Description |
|--------|-------------|
| `-j, --json` | Output in JSON format |
| `-q, --quiet` | Suppress non-essential output |
| `-v, --verbose` | Enable verbose output |

### Other Options

| Option | Description |
|--------|-------------|
| `--help` | Display help and exit |
| `--version` | Output version information and exit |

## Shell Integration

### Bash Completion

Create `/etc/bash_completion.d/gs308ep`:

```bash
_gs308ep() {
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"
    opts="-h --host -p --password -P --port -o --on -f --off -c --cycle -s --status -w --power -W --total-power -S --stats -j --json -q --quiet -v --verbose --help --version"

    case "${prev}" in
        -h|--host|-p|--password)
            return 0
            ;;
        -P|--port)
            COMPREPLY=( $(compgen -W "1 2 3 4 5 6 7 8" -- ${cur}) )
            return 0
            ;;
        *)
            ;;
    esac

    COMPREPLY=( $(compgen -W "${opts}" -- ${cur}) )
    return 0
}

complete -F _gs308ep gs308ep
```

### Systemd Service

Monitor and control ports automatically with systemd:

```ini
[Unit]
Description=GS308EP Port 3 Auto-on
After=network.target

[Service]
Type=oneshot
Environment="GS308EP_HOST=192.168.1.1"
Environment="GS308EP_PASSWORD=admin"
ExecStart=/usr/local/bin/gs308ep -P 3 -o

[Install]
WantedBy=multi-user.target
```

## Exit Codes

| Code | Meaning |
|------|---------|
| 0 | Success |
| 1 | Error (authentication failed, invalid arguments, operation failed) |

## License

MIT License - See LICENSE file in project root

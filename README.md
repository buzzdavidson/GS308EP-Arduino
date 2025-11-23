# Overview

This is an early attempt to use Claude Sonnet 4.5 to create a missing API.  The Netgear GS308EP PoE switch provides a web UI, but no formal API as is available in higher end switches.  Ultimately I just wanted to be able to control power to individual ports from an ESP32, but the switch also provides power consumption and other diagnostic information that are also useful. There is an excellent python library available (https://github.com/foxey/py-netgear-plus), so I used Sonnet to reverse engineer the GS308EP specific functionality into an Arduino library.  Added a simple CLI for good measure as well as full docker-based build and test environments.

## Comments, contributions, and conversations

Nov 23 2025: To be super clear: this is an experiment.  This library and code has not yet been thoroughly reviewed.  It's probably not super useful to many people, but if you do find it useful you probably don't want to use it anywhere important.  I'll release an official 1.0 version once i've thoroughly evaluated the code.  THAT SAID, if it's interesting enough to download, please share feedback!   

# GS308EP Arduino Library

Control Netgear GS308EP PoE switch from ESP32 via web scraping.

## Description

This Arduino library enables ESP32 devices to control per-port Power over Ethernet (PoE) on Netgear GS308EP switches. Since the switch lacks an official API, this library uses web scraping techniques similar to the [py-netgear-plus](https://github.com/foxey/py-netgear-plus) Python library.

**Key Features:**
- Turn PoE ports on/off remotely
- Check PoE port status
- Power cycle ports
- Session-based authentication with cookie management
- MD5 password hashing (merge_hash algorithm)

## Hardware Requirements

- **ESP32 development board** (tested on ESP32-WROOM-32)
- **Netgear GS308EP PoE switch** (8-port model with PoE)
- Both devices on the same network

## Installation

### Arduino IDE
1. Download this repository as ZIP
2. In Arduino IDE: **Sketch** → **Include Library** → **Add .ZIP Library**
3. Select the downloaded ZIP file

### PlatformIO
Add to `platformio.ini`:
```ini
lib_deps =
    GS308EP
```

## Usage

### Basic Example

```cpp
#include <WiFi.h>
#include <GS308EP.h>

const char* ssid = "YourWiFiSSID";
const char* password = "YourWiFiPassword";

const char* switchIP = "192.168.1.1";
const char* switchPassword = "admin";

GS308EP poeSwitch(switchIP, switchPassword);

void setup() {
  Serial.begin(115200);
  
  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  // Initialize and login
  poeSwitch.begin();
  if (poeSwitch.login()) {
    Serial.println("Logged in successfully!");
    
    // Turn port 1 ON
    poeSwitch.turnOnPoEPort(1);
    
    // Check port status
    bool isOn = poeSwitch.getPoEPortStatus(1);
    Serial.print("Port 1 is: ");
    Serial.println(isOn ? "ON" : "OFF");
    
    // Power cycle port 1
    poeSwitch.cyclePoEPort(1, 2000);  // OFF for 2 seconds
  }
}

void loop() {
  // Your code here
}
```

## API Reference

### Constructor
```cpp
GS308EP(const char* ip, const char* password)
```
Create a switch controller instance.
- `ip`: IP address of the switch (e.g., "192.168.1.1")
- `password`: Administrator password

### Methods

#### `bool begin()`
Initialize the library. Call once in `setup()`.

#### `bool login()`
Authenticate with the switch. Returns `true` on success.

#### `bool isAuthenticated()`
Check if currently authenticated.

#### `bool turnOnPoEPort(uint8_t port)`
Enable PoE power on specified port (1-8).

#### `bool turnOffPoEPort(uint8_t port)`
Disable PoE power on specified port (1-8).

#### `bool getPoEPortStatus(uint8_t port)`
Get current PoE status. Returns `true` if enabled.

#### `bool cyclePoEPort(uint8_t port, uint16_t delayMs = 2000)`
Power cycle a port (turn off, wait, turn on).
- `port`: Port number (1-8)
- `delayMs`: Delay between off and on (default 2000ms)

#### `int getLastResponseCode()`
Get the last HTTP response code for debugging.

## How It Works

This library replicates the web UI workflow:

1. **Fetch login page** (`GET /login.cgi`) to extract the `rand` token
2. **Hash password** using MD5 merge_hash algorithm: `MD5(password + rand)`
3. **POST credentials** to `/login.cgi` to obtain SID cookie
4. **Fetch PoE config page** (`GET /PoEPortConfig.cgi`) to extract client hash
5. **POST port changes** to `/PoEPortConfig.cgi` with form data:
   ```
   ACTION=Apply&portID=0&ADMIN_MODE=1&PORT_PRIO=0&POW_MOD=3&
   POW_LIMT_TYP=0&DETEC_TYP=2&DISCONNECT_TYP=2&hash=<client_hash>
   ```

Based on reverse-engineering by [py-netgear-plus](https://github.com/foxey/py-netgear-plus).

## Troubleshooting

**Login fails:**
- Verify switch IP address
- Check password (default is on switch label)
- Ensure ESP32 and switch are on same network
- Check serial output for HTTP response codes

**Port control doesn't work:**
- Verify login succeeded (`isAuthenticated()`)
- Check port number is valid (1-8)
- Verify switch firmware version (tested with firmware 1.x)

**Connection timeouts:**
- Increase `HTTP_TIMEOUT` in `GS308EP.h`
- Check WiFi signal strength
- Verify switch web UI is accessible via browser

## Limitations

- **ESP32 only** - Uses ESP32 HTTP/WiFi libraries
- **No HTTPS** - Switch uses HTTP only
- **Single session** - One connection at a time
- **Firmware dependent** - Tested with GS308EP firmware 1.x (HTML may change)

## Examples

See `examples/BasicPoEControl/` for a complete working example.

## Testing

This library includes a comprehensive unit test suite. See [README_TESTING.md](README_TESTING.md) for details.

### Quick Test
```bash
# Using Docker (recommended for CI)
docker build -t gs308ep-test .
docker run --rm gs308ep-test

# Or using the test runner script
chmod +x test_runner.sh
./test_runner.sh
```

### Test Coverage
- HTML parsing (rand tokens, cookies, hashes)
- MD5 and merge_hash algorithms
- Port validation
- Authentication state management
- Edge cases and error handling

## Contributing

Contributions welcome! Please:
1. Run the test suite before submitting PRs
2. Add tests for new features
3. Test changes with actual hardware when possible

## Command-Line Interface (CLI)

A native CLI tool is available in the `cli/` directory for non-embedded use cases.

**Features:**
- GNU-style command-line interface
- All library features accessible from terminal
- JSON output support
- Environment variable configuration
- Docker support for easy deployment

**Quick start:**
```bash
cd cli
make
./build/gs308ep --host 192.168.1.1 --password admin --stats
```

See [cli/README.md](cli/README.md) for complete CLI documentation.

## License

MIT License - see LICENSE file

## Acknowledgments

- Based on [py-netgear-plus](https://github.com/foxey/py-netgear-plus) by foxey
- Inspired by the need for affordable ESP32-based switch control

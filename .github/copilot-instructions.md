# GitHub Copilot Instructions

This file contains workspace-level instructions for GitHub Copilot.

## Project Context

This is an Arduino library for controlling the **Netgear GS308EP PoE switch** from ESP32 devices. The switch has no official API, so this library uses **web scraping** to control per-port PoE power.

### Architecture Overview
- **Target Platform**: ESP32 (uses ESP32-specific WiFi/HTTP libraries)
- **Approach**: HTTP-based web scraping, modeled after [py-netgear-plus](https://github.com/foxey/py-netgear-plus)
- **Key Components**:
  - `src/GS308EP.h` - Class interface
  - `src/GS308EP.cpp` - Implementation (HTTP client, authentication, port control)
  - `examples/BasicPoEControl/` - Usage demonstration

### Standard Arduino Library Structure
- `src/` - Library source files (.h and .cpp)
- `examples/` - Example sketches (.ino) demonstrating library usage
- `library.properties` - Arduino library metadata (name, version, dependencies)
- `keywords.txt` - Syntax highlighting definitions for Arduino IDE

## Coding Standards

### Arduino Library Conventions
- **Header files**: Place class declarations in `src/GS308EP.h` with include guards
- **Implementation**: Place method definitions in `src/GS308EP.cpp`
- **Naming**: Use PascalCase for class names, camelCase for methods, UPPER_CASE for constants
- **Memory**: Be mindful of RAM constraints - use `const` and `PROGMEM` for constant data
- **Documentation**: Add JSDoc-style comments (`/** */`) for all public APIs

### File Structure Patterns
```cpp
// src/GS308EP.h - Header pattern
#ifndef GS308EP_H
#define GS308EP_H

#include <Arduino.h>

class GS308EP {
public:
    GS308EP(const char* ip, const char* password);
    bool begin();
    bool turnOnPoEPort(uint8_t port);
    // Public API methods
private:
    String _ip;
    String _cookieSID;
    // Private members
};

#endif
```

### Example Sketches
- Place in `examples/` with descriptive folder names (e.g., `examples/BasicPoEControl/BasicPoEControl.ino`)
- Include setup() and loop() functions with clear comments
- Keep examples simple and focused on one feature

### Keywords File
Update `keywords.txt` with three categories:
- KEYWORD1: Class names (orange highlighting) - `GS308EP`
- KEYWORD2: Methods and functions (brown highlighting) - `begin`, `login`, `turnOnPoEPort`
- LITERAL1: Constants (blue highlighting) - `MAX_PORTS`, `HTTP_TIMEOUT`

## GS308EP-Specific Implementation Details

### Authentication Flow (from py-netgear-plus)
1. **GET** `/login.cgi` to extract `rand` token from hidden form field:
   ```html
   <input type=hidden id="rand" name="rand" value='1735414426'>
   ```
2. **Calculate password hash** using merge_hash algorithm:
   ```cpp
   String merged = password + rand;
   String hash = MD5(merged);
   ```
3. **POST** to `/login.cgi` with `password=<hash>`
4. **Extract SID cookie** from `Set-Cookie: SID=xxxxx` header
5. **Include cookie** in all subsequent requests: `Cookie: SID=xxxxx`

### PoE Port Control Workflow
1. **GET** `/PoEPortConfig.cgi` with SID cookie to fetch current config
2. **Extract client hash** from hidden form field:
   ```html
   <input type=hidden name='hash' id='hash' value="3483299a0487987b90483a70c5d3d2dd">
   ```
3. **POST** to `/PoEPortConfig.cgi` with form data:
   ```
   ACTION=Apply&portID=0&ADMIN_MODE=1&PORT_PRIO=0&POW_MOD=3&
   POW_LIMT_TYP=0&DETEC_TYP=2&DISCONNECT_TYP=2&hash=<client_hash>
   ```
   - `portID`: Zero-indexed (0-7 for ports 1-8)
   - `ADMIN_MODE`: 1=enabled, 0=disabled
   - `POW_MOD`: 3=802.3at mode
   - `DETEC_TYP`: 2=IEEE 802 detection
4. **Check response** for "SUCCESS" string or HTTP 200

### PoE Status Parsing
**GET** `/getPoePortStatus.cgi` returns HTML with port status:
```html
<input type="hidden" class="port" value="1">
<input type="hidden" class="hidPortPwr" id="hidPortPwr" value="1">
```
Parse to extract port enable status (1=on, 0=off).

### Critical Implementation Notes
- **Port numbering**: User-facing 1-8, internal 0-7 (zero-indexed in POST)
- **HTML parsing**: Simple string search (indexOf) - no full HTML parser needed
- **Session management**: Store SID cookie, include in all authenticated requests
- **Error handling**: Check HTTP response codes, parse for "SUCCESS" in responses
- **Firmware dependency**: HTML structure may change across firmware versions

### ESP32-Specific Considerations
- Use `HTTPClient` and `WiFiClient` classes
- Use `MD5Builder` for password hashing
- Typical memory: 320KB RAM, sufficient for HTTP responses
- Use `String` class for dynamic text (Arduino String, not C strings)
- Set `HTTPClient` timeout: `_http.setTimeout(5000)` for network stability

## Common Tasks

### Adding a New Method
1. Declare in `src/GS308EP.h` public section with JSDoc comment
2. Implement in `src/GS308EP.cpp`
3. Add to `keywords.txt` under KEYWORD2
4. Document in README.md API Reference section
5. Create example sketch if significant feature

### Debugging Web Scraping Issues
1. Enable verbose Serial output in examples
2. Print HTTP response codes: `Serial.println(getLastResponseCode())`
3. Print response bodies: `Serial.println(response)`
4. Compare with browser Network tab (Developer Tools)
5. Check py-netgear-plus for reference patterns

### Testing Changes
1. Upload to ESP32 hardware
2. Monitor Serial output at 115200 baud
3. Verify actual PoE port behavior on connected devices
4. Test login failure scenarios (wrong password)
5. Test network failure scenarios (disconnect WiFi)

## Preferences

- **Testing**: Create example sketches that serve as integration tests
- **Dependencies**: Minimal - use ESP32 built-in libraries when possible
- **Documentation**: Keep README.md updated with API reference and usage examples
- **Compatibility**: Target ESP32 only (specified in `library.properties: architectures=esp32`)
- **Error messages**: Use Serial.println() for debugging, return bool for success/failure

## References

- **py-netgear-plus**: https://github.com/foxey/py-netgear-plus (Python reference implementation)
- **ESP32 HTTPClient**: https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient
- **Arduino Library Spec**: https://arduino.github.io/arduino-cli/library-specification/

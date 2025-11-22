/**
 * @file GS308EP.h
 * @brief Arduino library for controlling Netgear GS308EP PoE switch
 *
 * This library provides web scraping-based control of the Netgear GS308EP
 * PoE switch, allowing ESP32 devices to control per-port power remotely.
 * Based on the py-netgear-plus library approach.
 *
 * @author
 * @version 0.5.0
 */

#ifndef GS308EP_H
#define GS308EP_H

#include <Arduino.h>
#include <WiFiClient.h>
#include <HTTPClient.h>

/**
 * @struct PoEPortStats
 * @brief Comprehensive statistics for a single PoE port
 */
struct PoEPortStats
{
 uint8_t port;      ///< Port number (1-8)
 bool enabled;      ///< Whether PoE is enabled on this port
 String status;     ///< Status text ("Delivering Power", "Disabled", "Searching", etc.)
 float voltage;     ///< Output voltage in volts (V)
 float current;     ///< Output current in milliamps (mA)
 float power;       ///< Output power in watts (W)
 float temperature; ///< Temperature in Celsius (°C)
 String fault;      ///< Fault status ("No Error", or error description)
 String powerClass; ///< PoE class ("Class 3", "Class 4", "Unknown", etc.)
};

/**
 * @class GS308EP
 * @brief Main class for communicating with Netgear GS308EP switch
 */
class GS308EP
{
public:
 /**
  * @brief Construct a new GS308EP object
  * @param ip IP address of the switch (e.g., "192.168.1.1")
  * @param password Administrator password for the switch
  */
 GS308EP(const char *ip, const char *password);

 /**
  * @brief Destructor
  */
 ~GS308EP();

 /**
  * @brief Initialize the library and establish connection
  * @return true if initialization successful, false otherwise
  */
 bool begin();

 /**
  * @brief Authenticate with the switch and obtain session cookie
  * @return true if login successful, false otherwise
  */
 bool login();

 /**
  * @brief Check if currently authenticated
  * @return true if authenticated, false otherwise
  */
 bool isAuthenticated();

 /**
  * @brief Turn on PoE power for a specific port
  * @param port Port number (1-8)
  * @return true if successful, false otherwise
  */
 bool turnOnPoEPort(uint8_t port);

 /**
  * @brief Turn off PoE power for a specific port
  * @param port Port number (1-8)
  * @return true if successful, false otherwise
  */
 bool turnOffPoEPort(uint8_t port);

 /**
  * @brief Get PoE port status
  * @param port Port number (1-8)
  * @return true if port is enabled, false if disabled or error
  */
 bool getPoEPortStatus(uint8_t port);

 /**
  * @brief Power cycle a PoE port (off then on)
  * @param port Port number (1-8)
  * @param delayMs Delay in milliseconds between off and on (default 2000)
  * @return true if successful, false otherwise
  */
 bool cyclePoEPort(uint8_t port, uint16_t delayMs = 2000);

 /**
  * @brief Get PoE power consumption for a specific port
  * @param port Port number (1-8)
  * @return Power consumption in watts, or -1.0 on error
  */
 float getPoEPortPower(uint8_t port);

 /**
  * @brief Get total PoE power consumption across all ports
  * @return Total power consumption in watts, or -1.0 on error
  */
 float getTotalPoEPower();

 /**
  * @brief Get comprehensive statistics for all PoE ports in a single call
  * @param stats Array of PoEPortStats structures (must be size 8)
  * @return true if successful, false on error
  */
 bool getAllPoEPortStats(PoEPortStats stats[8]);

 /**
  * @brief Get the last HTTP response code
  * @return HTTP response code
  */
 int getLastResponseCode();

private:
 // Configuration
 String _ip;
 String _password;
 String _cookieSID;
 String _clientHash;

 // HTTP client
 HTTPClient _http;
 WiFiClient _client;

 // State tracking
 bool _authenticated;
 int _lastResponseCode;

 // Constants
 static const uint8_t MAX_PORTS = 8;
 static const uint16_t HTTP_TIMEOUT = 5000;
 static const char *LOGIN_URL_TEMPLATE;
 static const char *POE_CONFIG_URL_TEMPLATE;
 static const char *POE_STATUS_URL_TEMPLATE;

 // Helper methods
 bool fetchLoginPage();
 String extractRand(const String &html);
 String mergeHash(const String &password, const String &rand);
 String md5Hash(const String &input);
 bool extractCookie(const String &headers);
 bool extractClientHash(const String &html);
 bool setPoEPortState(uint8_t port, bool enabled);
 float extractPortPower(const String &html, uint8_t port);
 bool extractPortStats(const String &html, uint8_t port, PoEPortStats &stats);
 String httpGet(const String &url);
 String httpPost(const String &url, const String &data);
 bool isValidPort(uint8_t port);
};

#endif // GS308EP_H

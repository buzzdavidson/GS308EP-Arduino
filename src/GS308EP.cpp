/**
 * @file GS308EP.cpp
 * @brief Implementation of the GS308EP library
 */

#include "GS308EP.h"
#include <MD5Builder.h>

// Static constants
const char *GS308EP::LOGIN_URL_TEMPLATE = "http://%s/login.cgi";
const char *GS308EP::POE_CONFIG_URL_TEMPLATE = "http://%s/PoEPortConfig.cgi";
const char *GS308EP::POE_STATUS_URL_TEMPLATE = "http://%s/getPoePortStatus.cgi";

/**
 * @brief Constructor
 */
GS308EP::GS308EP(const char *ip, const char *password)
    : _ip(ip), _password(password), _authenticated(false), _lastResponseCode(0)
{
}

/**
 * @brief Destructor
 */
GS308EP::~GS308EP()
{
 _http.end();
}

/**
 * @brief Initialize the library
 */
bool GS308EP::begin()
{
 _http.setTimeout(HTTP_TIMEOUT);
 return true;
}

/**
 * @brief Authenticate with the switch
 */
bool GS308EP::login()
{
 // Step 1: Fetch the login page to get the 'rand' value
 if (!fetchLoginPage())
 {
  return false;
 }

 // Step 2: Build the login URL
 char url[128];
 snprintf(url, sizeof(url), LOGIN_URL_TEMPLATE, _ip.c_str());

 // Step 3: Prepare POST data with hashed password
 String postData = "password=" + _clientHash;

 // Step 4: Send login request
 String response = httpPost(url, postData);

 // Step 5: Check if we got a cookie
 _authenticated = !_cookieSID.isEmpty();

 return _authenticated;
}

/**
 * @brief Check authentication status
 */
bool GS308EP::isAuthenticated()
{
 return _authenticated;
}

/**
 * @brief Turn on PoE port
 */
bool GS308EP::turnOnPoEPort(uint8_t port)
{
 return setPoEPortState(port, true);
}

/**
 * @brief Turn off PoE port
 */
bool GS308EP::turnOffPoEPort(uint8_t port)
{
 return setPoEPortState(port, false);
}

/**
 * @brief Get PoE port status
 */
bool GS308EP::getPoEPortStatus(uint8_t port)
{
 if (!isValidPort(port) || !_authenticated)
 {
  return false;
 }

 char url[128];
 snprintf(url, sizeof(url), POE_STATUS_URL_TEMPLATE, _ip.c_str());

 String response = httpGet(url);

 // Parse the HTML response to find port status
 // Look for: <input type="hidden" class="hidPortPwr" id="hidPortPwr" value="1">
 // This is a simplified parser - production code should be more robust
 String searchPattern = "\"port\" value=\"" + String(port) + "\"";
 int portIndex = response.indexOf(searchPattern);

 if (portIndex == -1)
 {
  return false;
 }

 // Search forward for the hidPortPwr value
 int pwrIndex = response.indexOf("hidPortPwr\" value=\"", portIndex);
 if (pwrIndex == -1)
 {
  return false;
 }

 pwrIndex += 19; // Move past the search string
 char value = response.charAt(pwrIndex);

 return (value == '1');
}

/**
 * @brief Power cycle a PoE port
 */
bool GS308EP::cyclePoEPort(uint8_t port, uint16_t delayMs)
{
 if (!turnOffPoEPort(port))
 {
  return false;
 }

 delay(delayMs);

 return turnOnPoEPort(port);
}

/**
 * @brief Get last HTTP response code
 */
int GS308EP::getLastResponseCode()
{
 return _lastResponseCode;
}

// Private helper methods

/**
 * @brief Fetch login page and extract rand value
 */
bool GS308EP::fetchLoginPage()
{
 char url[128];
 snprintf(url, sizeof(url), LOGIN_URL_TEMPLATE, _ip.c_str());

 String response = httpGet(url);

 if (response.isEmpty())
 {
  return false;
 }

 // Extract rand value from the login page
 String rand = extractRand(response);

 if (rand.isEmpty())
 {
  // No rand value means older firmware - use plain MD5
  _clientHash = md5Hash(_password);
 }
 else
 {
  // Use merge_hash algorithm (password + rand)
  _clientHash = mergeHash(_password, rand);
 }

 return true;
}

/**
 * @brief Extract rand value from login page HTML
 */
String GS308EP::extractRand(const String &html)
{
 // Look for: <input type=hidden id="rand" name="rand" value='1735414426'>
 int randIndex = html.indexOf("id=\"rand\"");
 if (randIndex == -1)
 {
  randIndex = html.indexOf("id='rand'");
 }

 if (randIndex == -1)
 {
  return "";
 }

 int valueIndex = html.indexOf("value", randIndex);
 if (valueIndex == -1)
 {
  return "";
 }

 // Find the quote after value=
 int startQuote = html.indexOf("'", valueIndex);
 if (startQuote == -1)
 {
  startQuote = html.indexOf("\"", valueIndex);
 }

 if (startQuote == -1)
 {
  return "";
 }

 startQuote++; // Move past the quote

 // Find closing quote
 int endQuote = html.indexOf("'", startQuote);
 if (endQuote == -1)
 {
  endQuote = html.indexOf("\"", startQuote);
 }

 if (endQuote == -1)
 {
  return "";
 }

 return html.substring(startQuote, endQuote);
}

/**
 * @brief Merge hash algorithm (password + rand)
 */
String GS308EP::mergeHash(const String &password, const String &rand)
{
 String combined = password + rand;
 return md5Hash(combined);
}

/**
 * @brief Calculate MD5 hash
 */
String GS308EP::md5Hash(const String &input)
{
 MD5Builder md5;
 md5.begin();
 md5.add(input);
 md5.calculate();
 return md5.toString();
}

/**
 * @brief Extract SID cookie from response headers
 */
bool GS308EP::extractCookie(const String &headers)
{
 // Look for: Set-Cookie: SID=xxxxx
 int cookieIndex = headers.indexOf("Set-Cookie:");
 if (cookieIndex == -1)
 {
  return false;
 }

 int sidIndex = headers.indexOf("SID=", cookieIndex);
 if (sidIndex == -1)
 {
  return false;
 }

 sidIndex += 4; // Move past "SID="

 int endIndex = headers.indexOf(";", sidIndex);
 if (endIndex == -1)
 {
  endIndex = headers.indexOf("\r", sidIndex);
 }
 if (endIndex == -1)
 {
  endIndex = headers.indexOf("\n", sidIndex);
 }

 if (endIndex == -1)
 {
  return false;
 }

 _cookieSID = headers.substring(sidIndex, endIndex);
 return !_cookieSID.isEmpty();
}

/**
 * @brief Extract client hash from response
 */
bool GS308EP::extractClientHash(const String &html)
{
 // Look for hash parameter in forms
 // <input type=hidden name='hash' id='hash' value="xxxx">
 int hashIndex = html.indexOf("name='hash'");
 if (hashIndex == -1)
 {
  hashIndex = html.indexOf("name=\"hash\"");
 }

 if (hashIndex == -1)
 {
  return false;
 }

 int valueIndex = html.indexOf("value", hashIndex);
 if (valueIndex == -1)
 {
  return false;
 }

 int startQuote = html.indexOf("\"", valueIndex);
 if (startQuote == -1)
 {
  startQuote = html.indexOf("'", valueIndex);
 }

 if (startQuote == -1)
 {
  return false;
 }

 startQuote++; // Move past the quote

 int endQuote = html.indexOf("\"", startQuote);
 if (endQuote == -1)
 {
  endQuote = html.indexOf("'", startQuote);
 }

 if (endQuote == -1)
 {
  return false;
 }

 _clientHash = html.substring(startQuote, endQuote);
 return !_clientHash.isEmpty();
}

/**
 * @brief Set PoE port state
 */
bool GS308EP::setPoEPortState(uint8_t port, bool enabled)
{
 if (!isValidPort(port) || !_authenticated)
 {
  return false;
 }

 // Get the current configuration to extract the hash
 char url[128];
 snprintf(url, sizeof(url), POE_CONFIG_URL_TEMPLATE, _ip.c_str());

 String response = httpGet(url);

 if (!extractClientHash(response))
 {
  return false;
 }

 // Build POST data according to GS308EP format
 // Based on py-netgear-plus: ACTION=Apply&portID=0&ADMIN_MODE=1&PORT_PRIO=0&POW_MOD=3&POW_LIMT_TYP=0&DETEC_TYP=2&DISCONNECT_TYP=2&hash=xxxxx
 String postData = "ACTION=Apply";
 postData += "&portID=" + String(port - 1); // Zero-indexed
 postData += "&ADMIN_MODE=" + String(enabled ? "1" : "0");
 postData += "&PORT_PRIO=0";
 postData += "&POW_MOD=3"; // 802.3at mode
 postData += "&POW_LIMT_TYP=0";
 postData += "&DETEC_TYP=2"; // IEEE 802
 postData += "&DISCONNECT_TYP=2";
 postData += "&hash=" + _clientHash;

 // Send the request
 response = httpPost(url, postData);

 // Check for SUCCESS response
 return (response.indexOf("SUCCESS") != -1) || (_lastResponseCode == 200);
}

/**
 * @brief Perform HTTP GET request
 */
String GS308EP::httpGet(const String &url)
{
 _http.begin(_client, url);

 // Add cookie if authenticated
 if (!_cookieSID.isEmpty())
 {
  _http.addHeader("Cookie", "SID=" + _cookieSID);
 }

 _lastResponseCode = _http.GET();

 String response = "";
 if (_lastResponseCode == 200)
 {
  response = _http.getString();

  // Check for cookies in response
  String headers = _http.header("Set-Cookie");
  if (!headers.isEmpty())
  {
   extractCookie(headers);
  }
 }

 _http.end();
 return response;
}

/**
 * @brief Perform HTTP POST request
 */
String GS308EP::httpPost(const String &url, const String &data)
{
 _http.begin(_client, url);

 // Add headers
 _http.addHeader("Content-Type", "application/x-www-form-urlencoded");

 // Add cookie if authenticated
 if (!_cookieSID.isEmpty())
 {
  _http.addHeader("Cookie", "SID=" + _cookieSID);
 }

 _lastResponseCode = _http.POST(data);

 String response = "";
 if (_lastResponseCode == 200)
 {
  response = _http.getString();

  // Check for cookies in response
  String headers = _http.header("Set-Cookie");
  if (!headers.isEmpty())
  {
   extractCookie(headers);
  }
 }

 _http.end();
 return response;
}

/**
 * @brief Extract power consumption for a specific port from HTML
 * @param html HTML response from getPoePortStatus.cgi
 * @param port Port number (1-8)
 * @return Power in watts, or -1.0 if not found
 */
float GS308EP::extractPortPower(const String &html, uint8_t port)
{
 // Find the port entry in the HTML
 // Each port has: <input type="hidden" class="port" value="X">
 // Followed by spans with ml574 (power in watts)

 String portMarker = "value=\"" + String(port) + "\"";
 int portPos = html.indexOf(portMarker);

 if (portPos == -1)
 {
  return -1.0;
 }

 // Look for ml574 span after the port marker (within 1000 chars)
 // <span class='hid-txt wid-full'>ml574</span>
 // </div>
 // <div>
 // <span>5.8</span>

 int ml574Pos = html.indexOf("ml574", portPos);
 if (ml574Pos == -1 || ml574Pos > portPos + 1000)
 {
  return -1.0;
 }

 // Find the next <span> after ml574 which contains the power value
 int spanStart = html.indexOf("<span>", ml574Pos + 5);
 if (spanStart == -1)
 {
  return -1.0;
 }

 spanStart += 6; // Move past "<span>"
 int spanEnd = html.indexOf("</span>", spanStart);

 if (spanEnd == -1)
 {
  return -1.0;
 }

 String powerStr = html.substring(spanStart, spanEnd);
 powerStr.trim();

 return powerStr.toFloat();
}

/**
 * @brief Get PoE power consumption for a specific port
 */
float GS308EP::getPoEPortPower(uint8_t port)
{
 if (!isValidPort(port))
 {
  return -1.0;
 }

 if (!_authenticated)
 {
  return -1.0;
 }

 // Fetch PoE port status page
 String url = String("http://") + _ip + POE_STATUS_URL_TEMPLATE;
 String response = httpGet(url);

 if (_lastResponseCode != 200)
 {
  return -1.0;
 }

 return extractPortPower(response, port);
}

/**
 * @brief Get total PoE power consumption across all ports
 */
float GS308EP::getTotalPoEPower()
{
 if (!_authenticated)
 {
  return -1.0;
 }

 // Fetch PoE port status page once
 String url = String("http://") + _ip + POE_STATUS_URL_TEMPLATE;
 String response = httpGet(url);

 if (_lastResponseCode != 200)
 {
  return -1.0;
 }

 // Sum power across all ports
 float totalPower = 0.0;
 for (uint8_t port = 1; port <= MAX_PORTS; port++)
 {
  float portPower = extractPortPower(response, port);
  if (portPower >= 0.0)
  {
   totalPower += portPower;
  }
 }

 return totalPower;
}

/**
 * @brief Extract comprehensive statistics for a specific port from HTML
 * @param html HTML response from getPoePortStatus.cgi
 * @param port Port number (1-8)
 * @param stats Reference to PoEPortStats structure to populate
 * @return true if successful, false if port data not found
 */
bool GS308EP::extractPortStats(const String &html, uint8_t port, PoEPortStats &stats)
{
 // Initialize the stats structure
 stats.port = port;
 stats.enabled = false;
 stats.status = "Unknown";
 stats.voltage = 0.0;
 stats.current = 0.0;
 stats.power = 0.0;
 stats.temperature = 0.0;
 stats.fault = "Unknown";
 stats.powerClass = "Unknown";

 // Find the port entry in the HTML
 // Each port has: <input type="hidden" class="port" value="X">
 String portMarker = "value=\"" + String(port) + "\"";
 int portPos = html.indexOf(portMarker);

 if (portPos == -1)
 {
  return false;
 }

 // Search backwards for status text (within 500 chars before port marker)
 // <span class="pull-right poe-power-mode">
 // <span>Delivering Power</span>
 int searchStart = (portPos > 500) ? portPos - 500 : 0;
 String searchArea = html.substring(searchStart, portPos);

 int statusModePos = searchArea.lastIndexOf("poe-power-mode");
 if (statusModePos != -1)
 {
  int spanStart = searchArea.indexOf("<span>", statusModePos);
  if (spanStart != -1)
  {
   spanStart += 6; // Move past "<span>"
   int spanEnd = searchArea.indexOf("</span>", spanStart);
   if (spanEnd != -1)
   {
    stats.status = searchArea.substring(spanStart, spanEnd);
    stats.status.trim();
    stats.enabled = (stats.status == "Delivering Power");
   }
  }
 }

 // Search backwards for power class (within same area)
 // <span class="powClassShow">ml003@4@</span> or <span class="powClassShow">Unknown</span>
 int classPos = searchArea.lastIndexOf("powClassShow");
 if (classPos != -1)
 {
  int spanStart = searchArea.indexOf(">", classPos);
  if (spanStart != -1)
  {
   spanStart++; // Move past ">"
   int spanEnd = searchArea.indexOf("</span>", spanStart);
   if (spanEnd != -1)
   {
    String classText = searchArea.substring(spanStart, spanEnd);
    classText.trim();

    // Parse format like "ml003@4@" -> "Class 4"
    if (classText.startsWith("ml003@"))
    {
     int atPos = classText.indexOf("@", 6);
     if (atPos != -1 && atPos > 6)
     {
      String classNum = classText.substring(6, atPos);
      stats.powerClass = "Class " + classNum;
     }
    }
    else
    {
     stats.powerClass = classText;
    }
   }
  }
 }

 // Extract ml570 (voltage in V)
 int ml570Pos = html.indexOf("ml570", portPos);
 if (ml570Pos != -1 && ml570Pos < portPos + 1000)
 {
  int spanStart = html.indexOf("<span>", ml570Pos + 5);
  if (spanStart != -1)
  {
   spanStart += 6;
   int spanEnd = html.indexOf("</span>", spanStart);
   if (spanEnd != -1)
   {
    String voltageStr = html.substring(spanStart, spanEnd);
    voltageStr.trim();
    stats.voltage = voltageStr.toFloat();
   }
  }
 }

 // Extract ml572 (current in mA)
 int ml572Pos = html.indexOf("ml572", portPos);
 if (ml572Pos != -1 && ml572Pos < portPos + 1000)
 {
  int spanStart = html.indexOf("<span>", ml572Pos + 5);
  if (spanStart != -1)
  {
   spanStart += 6;
   int spanEnd = html.indexOf("</span>", spanStart);
   if (spanEnd != -1)
   {
    String currentStr = html.substring(spanStart, spanEnd);
    currentStr.trim();
    stats.current = currentStr.toFloat();
   }
  }
 }

 // Extract ml574 (power in W)
 stats.power = extractPortPower(html, port);

 // Extract ml575 (temperature in °C)
 int ml575Pos = html.indexOf("ml575", portPos);
 if (ml575Pos != -1 && ml575Pos < portPos + 1000)
 {
  int spanStart = html.indexOf("<span>", ml575Pos + 5);
  if (spanStart != -1)
  {
   spanStart += 6;
   int spanEnd = html.indexOf("</span>", spanStart);
   if (spanEnd != -1)
   {
    String tempStr = html.substring(spanStart, spanEnd);
    tempStr.trim();
    stats.temperature = tempStr.toFloat();
   }
  }
 }

 // Extract ml581 (fault status)
 int ml581Pos = html.indexOf("ml581", portPos);
 if (ml581Pos != -1 && ml581Pos < portPos + 1000)
 {
  int spanStart = html.indexOf("<span>", ml581Pos + 5);
  if (spanStart != -1)
  {
   spanStart += 6;
   int spanEnd = html.indexOf("</span>", spanStart);
   if (spanEnd != -1)
   {
    stats.fault = html.substring(spanStart, spanEnd);
    stats.fault.trim();
   }
  }
 }

 return true;
}

/**
 * @brief Get comprehensive statistics for all PoE ports in a single call
 * @param stats Array of PoEPortStats structures (must be size 8)
 * @return true if successful, false on error
 */
bool GS308EP::getAllPoEPortStats(PoEPortStats stats[8])
{
 if (!_authenticated)
 {
  return false;
 }

 // Fetch PoE port status page once
 String url = String("http://") + _ip + POE_STATUS_URL_TEMPLATE;
 String response = httpGet(url);

 if (_lastResponseCode != 200)
 {
  return false;
 }

 // Extract statistics for all ports
 bool allSuccess = true;
 for (uint8_t port = 1; port <= MAX_PORTS; port++)
 {
  if (!extractPortStats(response, port, stats[port - 1]))
  {
   allSuccess = false;
  }
 }

 return allSuccess;
}

/**
 * @brief Validate port number
 */
bool GS308EP::isValidPort(uint8_t port)
{
 return (port >= 1 && port <= MAX_PORTS);
}

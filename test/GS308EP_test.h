/**
 * @file GS308EP_test.h
 * @brief Test-friendly version of GS308EP class with minimal dependencies
 *
 * This header extracts the parsing and validation logic from GS308EP
 * for unit testing without Arduino/ESP32 dependencies.
 */

#ifndef GS308EP_TEST_H
#define GS308EP_TEST_H

#include <string>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <openssl/md5.h>

// Simple String class for testing
class String : public std::string
{
public:
 String() : std::string() {}
 String(const char *str) : std::string(str ? str : "") {}
 String(const std::string &str) : std::string(str) {}
 String(int val) : std::string(std::to_string(val)) {}
 String(size_t n, char c) : std::string(n, c) {} // Constructor for single char strings

 const char *c_str() const { return std::string::c_str(); }
 bool isEmpty() const { return empty(); }
 int length() const { return size(); }

 int indexOf(const char *str) const
 {
  size_t pos = find(str);
  return pos == std::string::npos ? -1 : static_cast<int>(pos);
 }

 int indexOf(const char *str, int start) const
 {
  if (start < 0 || start >= static_cast<int>(length()))
   return -1;
  size_t pos = find(str, start);
  return pos == std::string::npos ? -1 : static_cast<int>(pos);
 }

 int indexOf(const String &str) const
 {
  size_t pos = find(str);
  return pos == std::string::npos ? -1 : static_cast<int>(pos);
 }

 int indexOf(const String &str, int start) const
 {
  if (start < 0 || start >= static_cast<int>(length()))
   return -1;
  size_t pos = find(str, start);
  return pos == std::string::npos ? -1 : static_cast<int>(pos);
 }

 String substring(int start) const
 {
  if (start < 0 || start >= static_cast<int>(length()))
   return String();
  return String(substr(start));
 }

 String substring(int start, int end) const
 {
  if (start < 0 || start >= static_cast<int>(length()))
   return String();
  if (end > static_cast<int>(length()))
   end = length();
  return String(substr(start, end - start));
 }
};

// MD5 hash function
String md5Hash(const String &input)
{
 unsigned char digest[MD5_DIGEST_LENGTH];
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
 MD5((unsigned char *)input.c_str(), input.length(), digest);
#pragma GCC diagnostic pop

 std::ostringstream oss;
 for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
 {
  oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
 }
 return String(oss.str());
}

// PoE Port Statistics structure
struct PoEPortStats
{
 uint8_t port;
 bool enabled;
 String status;
 float voltage;
 float current;
 float power;
 float temperature;
 String fault;
 String powerClass;
};

// GS308EP test class with parsing logic
class GS308EPTest
{
protected:
 String _ip;
 String _password;
 String _cookieSID;
 String _clientHash;
 bool _authenticated;

public:
 GS308EPTest(const char *ip, const char *password)
     : _ip(ip), _password(password), _authenticated(false) {}

 bool begin() { return true; }
 bool isAuthenticated() const { return _authenticated; }
 void setAuthenticated(bool auth) { _authenticated = auth; }
 String getCookieSID() const { return _cookieSID; }
 void setCookieSID(const String &sid) { _cookieSID = sid; }
 String getClientHash() const { return _clientHash; }

 // Extract rand token from login page HTML
 String extractRand(const String &html)
 {
  int pos = html.indexOf("name=\"rand\"");
  if (pos == -1)
   pos = html.indexOf("name='rand'");
  if (pos == -1)
   return "";

  int valuePos = html.indexOf("value", pos);
  if (valuePos == -1)
   return "";

  int quotePos = html.indexOf("\"", valuePos);
  int singleQuotePos = html.indexOf("'", valuePos);

  if (quotePos == -1 && singleQuotePos == -1)
   return "";

  int startPos = (quotePos != -1 && (singleQuotePos == -1 || quotePos < singleQuotePos))
                     ? quotePos
                     : singleQuotePos;
  char quoteChar = (startPos == quotePos) ? '"' : '\'';

  startPos++;
  String quoteStr(1, quoteChar);
  int endPos = html.indexOf(quoteStr, startPos);
  if (endPos == -1)
   return "";

  return html.substring(startPos, endPos);
 }

 // Extract SID cookie from response headers
 bool extractCookie(const String &headers)
 {
  int pos = headers.indexOf("SID=");
  if (pos == -1)
   return false;

  pos += 4;
  int endPos = headers.indexOf(";", pos);
  if (endPos == -1)
   endPos = headers.indexOf("\r", pos);
  if (endPos == -1)
   endPos = headers.indexOf("\n", pos);
  if (endPos == -1)
   endPos = headers.length();

  _cookieSID = headers.substring(pos, endPos);
  return !_cookieSID.isEmpty();
 }

 // Extract client hash from PoE config page
 bool extractClientHash(const String &html)
 {
  int pos = html.indexOf("name=\"hash\"");
  if (pos == -1)
   pos = html.indexOf("name='hash'");
  if (pos == -1)
   return false;

  int valuePos = html.indexOf("value", pos);
  if (valuePos == -1)
   return false;

  int quotePos = html.indexOf("\"", valuePos);
  int singleQuotePos = html.indexOf("'", valuePos);

  if (quotePos == -1 && singleQuotePos == -1)
   return false;

  int startPos = (quotePos != -1 && (singleQuotePos == -1 || quotePos < singleQuotePos))
                     ? quotePos
                     : singleQuotePos;
  char quoteChar = (startPos == quotePos) ? '"' : '\'';

  startPos++;
  String quoteStr(1, quoteChar);
  int endPos = html.indexOf(quoteStr, startPos);
  if (endPos == -1)
   return false;

  _clientHash = html.substring(startPos, endPos);
  return !_clientHash.isEmpty();
 }

 // MD5 hash function
 String md5Hash(const String &input)
 {
  return ::md5Hash(input);
 }

 // Merge password and rand, then hash
 String mergeHash(const String &password, const String &rand)
 {
  return md5Hash(password + rand);
 }

 // Validate port number (1-8)
 bool isValidPort(uint8_t port)
 {
  return (port >= 1 && port <= 8);
 }

 // Extract power consumption for a specific port from HTML
 float extractPortPower(const String &html, uint8_t port)
 {
  String portMarker = "value=\"" + String(port) + "\"";
  int portPos = html.indexOf(portMarker);

  if (portPos == -1)
   return -1.0;

  int ml574Pos = html.indexOf("ml574", portPos);
  if (ml574Pos == -1 || ml574Pos > portPos + 2000)
   return -1.0;

  int spanStart = html.indexOf("<span>", ml574Pos + 5);
  if (spanStart == -1)
   return -1.0;

  spanStart += 6;
  int spanEnd = html.indexOf("</span>", spanStart);
  if (spanEnd == -1)
   return -1.0;

  String powerStr = html.substring(spanStart, spanEnd);
  return std::stof(powerStr);
 }

 // Extract comprehensive statistics for a specific port from HTML
 bool extractPortStats(const String &html, uint8_t port, PoEPortStats &stats)
 {
  stats.port = port;
  stats.enabled = false;
  stats.status = "Unknown";
  stats.voltage = 0.0;
  stats.current = 0.0;
  stats.power = 0.0;
  stats.temperature = 0.0;
  stats.fault = "Unknown";
  stats.powerClass = "Unknown";

  String portMarker = "value=\"" + String(port) + "\"";
  int portPos = html.indexOf(portMarker);
  if (portPos == -1)
   return false;

  // Extract status (search backwards from port marker)
  int searchStart = (portPos > 500) ? portPos - 500 : 0;
  String searchArea = html.substring(searchStart, portPos);

  // Find last occurrence of poe-power-mode in the search area
  int statusModePos = -1;
  int tempPos = 0;
  while ((tempPos = searchArea.indexOf("poe-power-mode", tempPos)) != -1)
  {
   statusModePos = tempPos;
   tempPos++;
  }

  if (statusModePos != -1)
  {
   int spanStart = searchArea.indexOf("<span>", statusModePos);
   if (spanStart != -1)
   {
    spanStart += 6;
    int spanEnd = searchArea.indexOf("</span>", spanStart);
    if (spanEnd != -1)
    {
     stats.status = searchArea.substring(spanStart, spanEnd);
     stats.enabled = (stats.status == "Delivering Power");
    }
   }
  }

  // Extract power class (search backwards from port marker)
  int classPos = -1;
  tempPos = 0;
  while ((tempPos = searchArea.indexOf("powClassShow", tempPos)) != -1)
  {
   classPos = tempPos;
   tempPos++;
  }

  if (classPos != -1)
  {
   int spanStart = searchArea.indexOf(">", classPos);
   if (spanStart != -1)
   {
    spanStart++;
    int spanEnd = searchArea.indexOf("</span>", spanStart);
    if (spanEnd != -1)
    {
     String classText = searchArea.substring(spanStart, spanEnd);
     if (classText.find("ml003@") == 0 && classText.length() > 7)
     {
      int atPos = classText.indexOf("@", 6);
      if (atPos != -1)
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

  // Extract ml570 (voltage)
  int ml570Pos = html.indexOf("ml570", portPos);
  if (ml570Pos != -1 && ml570Pos < portPos + 2000)
  {
   int spanStart = html.indexOf("<span>", ml570Pos + 5);
   if (spanStart != -1)
   {
    spanStart += 6;
    int spanEnd = html.indexOf("</span>", spanStart);
    if (spanEnd != -1)
    {
     String voltageStr = html.substring(spanStart, spanEnd);
     stats.voltage = std::stof(voltageStr);
    }
   }
  }

  // Extract ml572 (current)
  int ml572Pos = html.indexOf("ml572", portPos);
  if (ml572Pos != -1 && ml572Pos < portPos + 2000)
  {
   int spanStart = html.indexOf("<span>", ml572Pos + 5);
   if (spanStart != -1)
   {
    spanStart += 6;
    int spanEnd = html.indexOf("</span>", spanStart);
    if (spanEnd != -1)
    {
     String currentStr = html.substring(spanStart, spanEnd);
     stats.current = std::stof(currentStr);
    }
   }
  }

  // Extract ml574 (power)
  stats.power = extractPortPower(html, port);

  // Extract ml575 (temperature)
  int ml575Pos = html.indexOf("ml575", portPos);
  if (ml575Pos != -1 && ml575Pos < portPos + 2000)
  {
   int spanStart = html.indexOf("<span>", ml575Pos + 5);
   if (spanStart != -1)
   {
    spanStart += 6;
    int spanEnd = html.indexOf("</span>", spanStart);
    if (spanEnd != -1)
    {
     String tempStr = html.substring(spanStart, spanEnd);
     try
     {
      stats.temperature = std::stof(tempStr);
     }
     catch (...)
     {
      stats.temperature = 0.0;
     }
    }
   }
  }

  // Extract ml581 (fault status)
  int ml581Pos = html.indexOf("ml581", portPos);
  if (ml581Pos != -1 && ml581Pos < portPos + 2000)
  {
   int spanStart = html.indexOf("<span>", ml581Pos + 5);
   if (spanStart != -1)
   {
    spanStart += 6;
    int spanEnd = html.indexOf("</span>", spanStart);
    if (spanEnd != -1)
    {
     stats.fault = html.substring(spanStart, spanEnd);
    }
   }
  }

  return true;
 }
};

#endif // GS308EP_TEST_H

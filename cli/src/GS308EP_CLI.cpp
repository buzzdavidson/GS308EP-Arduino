/**
 * @file GS308EP_CLI.cpp
 * @brief Implementation of CLI controller for GS308EP switch
 */

#include "GS308EP_CLI.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <numeric>
#include <curl/curl.h>
#include <openssl/md5.h>
#include <unistd.h>

// Constants
static const char *LOGIN_URL = "/login.cgi";
static const char *POE_CONFIG_URL = "/PoEPortConfig.cgi";
static const char *POE_STATUS_URL = "/getPoePortStatus.cgi";

// CURL write callback
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
 static_cast<std::string *>(userp)->append(static_cast<char *>(contents), size * nmemb);
 return size * nmemb;
}

// CURL header callback
static size_t HeaderCallback(char *buffer, size_t size, size_t nitems, void *userdata)
{
 static_cast<std::string *>(userdata)->append(buffer, size * nitems);
 return size * nitems;
}

GS308EP_CLI::GS308EP_CLI(const std::string &host, const std::string &password, bool verbose)
    : host_(host), password_(password), authenticated_(false), verbose_(verbose), last_response_code_(0)
{
 curl_global_init(CURL_GLOBAL_DEFAULT);
}

GS308EP_CLI::~GS308EP_CLI()
{
 curl_global_cleanup();
}

std::string GS308EP_CLI::httpGet(const std::string &path)
{
 CURL *curl = curl_easy_init();
 std::string response;
 std::string headers;

 if (curl)
 {
  std::string url = "http://" + host_ + path;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

  // Add cookie if authenticated
  if (!cookie_sid_.empty())
  {
   std::string cookie = "SID=" + cookie_sid_;
   curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
  }

  if (verbose_)
  {
   log("GET " + url);
  }

  CURLcode res = curl_easy_perform(curl);

  if (res == CURLE_OK)
  {
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &last_response_code_);

   // Extract cookie from headers if present
   std::string cookie = extractCookie(headers);
   if (!cookie.empty())
   {
    cookie_sid_ = cookie;
   }
  }
  else
  {
   error("HTTP GET failed: " + std::string(curl_easy_strerror(res)));
   last_response_code_ = 0;
  }

  curl_easy_cleanup(curl);
 }

 return response;
}

std::string GS308EP_CLI::httpPost(const std::string &path, const std::string &data)
{
 CURL *curl = curl_easy_init();
 std::string response;
 std::string headers;

 if (curl)
 {
  std::string url = "http://" + host_ + path;

  curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
  curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, HeaderCallback);
  curl_easy_setopt(curl, CURLOPT_HEADERDATA, &headers);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

  // Add cookie if authenticated
  if (!cookie_sid_.empty())
  {
   std::string cookie = "SID=" + cookie_sid_;
   curl_easy_setopt(curl, CURLOPT_COOKIE, cookie.c_str());
  }

  if (verbose_)
  {
   log("POST " + url + " [" + data + "]");
  }

  CURLcode res = curl_easy_perform(curl);

  if (res == CURLE_OK)
  {
   curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &last_response_code_);

   // Extract cookie from headers if present
   std::string cookie = extractCookie(headers);
   if (!cookie.empty())
   {
    cookie_sid_ = cookie;
   }
  }
  else
  {
   error("HTTP POST failed: " + std::string(curl_easy_strerror(res)));
   last_response_code_ = 0;
  }

  curl_easy_cleanup(curl);
 }

 return response;
}

std::string GS308EP_CLI::md5Hash(const std::string &input)
{
 unsigned char digest[MD5_DIGEST_LENGTH];

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
 MD5(reinterpret_cast<const unsigned char *>(input.c_str()), input.length(), digest);
#pragma GCC diagnostic pop

 std::ostringstream oss;
 for (int i = 0; i < MD5_DIGEST_LENGTH; i++)
 {
  oss << std::hex << std::setw(2) << std::setfill('0') << (int)digest[i];
 }

 return oss.str();
}

std::string GS308EP_CLI::mergeHash(const std::string &password, const std::string &rand)
{
 return md5Hash(password + rand);
}

std::string GS308EP_CLI::extractRand(const std::string &html)
{
 size_t pos = html.find("name=\"rand\"");
 if (pos == std::string::npos)
 {
  pos = html.find("name='rand'");
 }
 if (pos == std::string::npos)
 {
  return "";
 }

 size_t valuePos = html.find("value", pos);
 if (valuePos == std::string::npos)
 {
  return "";
 }

 size_t quotePos = html.find("\"", valuePos);
 size_t singleQuotePos = html.find("'", valuePos);

 if (quotePos == std::string::npos && singleQuotePos == std::string::npos)
 {
  return "";
 }

 size_t startPos = (quotePos != std::string::npos && (singleQuotePos == std::string::npos || quotePos < singleQuotePos))
                       ? quotePos
                       : singleQuotePos;
 char quoteChar = (startPos == quotePos) ? '"' : '\'';

 startPos++;
 size_t endPos = html.find(quoteChar, startPos);
 if (endPos == std::string::npos)
 {
  return "";
 }

 return html.substr(startPos, endPos - startPos);
}

std::string GS308EP_CLI::extractCookie(const std::string &headers)
{
 size_t pos = headers.find("SID=");
 if (pos == std::string::npos)
 {
  return "";
 }

 pos += 4;
 size_t endPos = headers.find(";", pos);
 if (endPos == std::string::npos)
 {
  endPos = headers.find("\r", pos);
 }
 if (endPos == std::string::npos)
 {
  endPos = headers.find("\n", pos);
 }
 if (endPos == std::string::npos)
 {
  endPos = headers.length();
 }

 return headers.substr(pos, endPos - pos);
}

bool GS308EP_CLI::extractClientHash(const std::string &html)
{
 size_t pos = html.find("name=\"hash\"");
 if (pos == std::string::npos)
 {
  pos = html.find("name='hash'");
 }
 if (pos == std::string::npos)
 {
  return false;
 }

 size_t valuePos = html.find("value", pos);
 if (valuePos == std::string::npos)
 {
  return false;
 }

 size_t quotePos = html.find("\"", valuePos);
 size_t singleQuotePos = html.find("'", valuePos);

 if (quotePos == std::string::npos && singleQuotePos == std::string::npos)
 {
  return false;
 }

 size_t startPos = (quotePos != std::string::npos && (singleQuotePos == std::string::npos || quotePos < singleQuotePos))
                       ? quotePos
                       : singleQuotePos;
 char quoteChar = (startPos == quotePos) ? '"' : '\'';

 startPos++;
 size_t endPos = html.find(quoteChar, startPos);
 if (endPos == std::string::npos)
 {
  return false;
 }

 client_hash_ = html.substr(startPos, endPos - startPos);
 return !client_hash_.empty();
}

bool GS308EP_CLI::login()
{
 // Step 1: Get login page and extract rand token
 std::string loginPage = httpGet(LOGIN_URL);
 if (last_response_code_ != 200)
 {
  error("Failed to fetch login page");
  return false;
 }

 std::string rand = extractRand(loginPage);
 if (rand.empty())
 {
  error("Failed to extract rand token");
  return false;
 }

 if (verbose_)
 {
  log("Rand token: " + rand);
 }

 // Step 2: Calculate password hash
 std::string hash = mergeHash(password_, rand);

 if (verbose_)
 {
  log("Password hash: " + hash);
 }

 // Step 3: POST login with hashed password
 std::string postData = "password=" + hash;
 httpPost(LOGIN_URL, postData);

 if (last_response_code_ != 200 || cookie_sid_.empty())
 {
  error("Authentication failed");
  return false;
 }

 if (verbose_)
 {
  log("Session ID: " + cookie_sid_);
 }

 authenticated_ = true;
 return true;
}

bool GS308EP_CLI::isValidPort(int port) const
{
 return port >= 1 && port <= 8;
}

bool GS308EP_CLI::setPortState(int port, bool enabled)
{
 if (!authenticated_)
 {
  error("Not authenticated");
  return false;
 }

 if (!isValidPort(port))
 {
  error("Invalid port number");
  return false;
 }

 // Get current config to extract client hash
 std::string configPage = httpGet(POE_CONFIG_URL);
 if (last_response_code_ != 200)
 {
  error("Failed to fetch PoE config");
  return false;
 }

 if (!extractClientHash(configPage))
 {
  error("Failed to extract client hash");
  return false;
 }

 // Build POST data (port is zero-indexed for API)
 std::ostringstream postData;
 postData << "ACTION=Apply";
 postData << "&portID=" << (port - 1);
 postData << "&ADMIN_MODE=" << (enabled ? "1" : "0");
 postData << "&PORT_PRIO=0";
 postData << "&POW_MOD=3";
 postData << "&POW_LIMT_TYP=0";
 postData << "&DETEC_TYP=2";
 postData << "&DISCONNECT_TYP=2";
 postData << "&hash=" << client_hash_;

 httpPost(POE_CONFIG_URL, postData.str());

 return last_response_code_ == 200;
}

bool GS308EP_CLI::getPortStatus(int port)
{
 if (!authenticated_ || !isValidPort(port))
 {
  return false;
 }

 std::string statusPage = httpGet(POE_STATUS_URL);
 if (last_response_code_ != 200)
 {
  return false;
 }

 // Look for port marker and enabled status
 std::string portMarker = "value=\"" + std::to_string(port) + "\"";
 size_t portPos = statusPage.find(portMarker);
 if (portPos == std::string::npos)
 {
  return false;
 }

 // Search within 1000 chars after port marker
 size_t searchEnd = std::min(portPos + 1000, statusPage.length());
 std::string searchArea = statusPage.substr(portPos, searchEnd - portPos);

 // Look for hidPortPwr value (1=on, 0=off)
 size_t pwrPos = searchArea.find("hidPortPwr");
 if (pwrPos != std::string::npos)
 {
  size_t valuePos = searchArea.find("value", pwrPos);
  if (valuePos != std::string::npos)
  {
   size_t quotePos = searchArea.find("\"", valuePos);
   if (quotePos != std::string::npos)
   {
    char statusChar = searchArea[quotePos + 1];
    return statusChar == '1';
   }
  }
 }

 return false;
}

float GS308EP_CLI::extractPortPower(const std::string &html, int port)
{
 std::string portMarker = "value=\"" + std::to_string(port) + "\"";
 size_t portPos = html.find(portMarker);

 if (portPos == std::string::npos)
 {
  return -1.0f;
 }

 size_t ml574Pos = html.find("ml574", portPos);
 if (ml574Pos == std::string::npos || ml574Pos > portPos + 2000)
 {
  return -1.0f;
 }

 size_t spanStart = html.find("<span>", ml574Pos + 5);
 if (spanStart == std::string::npos)
 {
  return -1.0f;
 }

 spanStart += 6;
 size_t spanEnd = html.find("</span>", spanStart);
 if (spanEnd == std::string::npos)
 {
  return -1.0f;
 }

 std::string powerStr = html.substr(spanStart, spanEnd - spanStart);

 try
 {
  return std::stof(powerStr);
 }
 catch (...)
 {
  return -1.0f;
 }
}

bool GS308EP_CLI::extractPortStats(const std::string &html, int port, PoEPortStats &stats)
{
 stats.port = port;
 stats.enabled = false;
 stats.status = "Unknown";
 stats.voltage = 0.0f;
 stats.current = 0.0f;
 stats.power = 0.0f;
 stats.temperature = 0.0f;
 stats.fault = "Unknown";
 stats.powerClass = "Unknown";

 std::string portMarker = "value=\"" + std::to_string(port) + "\"";
 size_t portPos = html.find(portMarker);
 if (portPos == std::string::npos)
 {
  return false;
 }

 // Extract status (search backwards)
 size_t searchStart = (portPos > 500) ? portPos - 500 : 0;
 std::string searchArea = html.substr(searchStart, portPos - searchStart);

 // Find last occurrence of poe-power-mode
 size_t statusModePos = std::string::npos;
 size_t tempPos = 0;
 while ((tempPos = searchArea.find("poe-power-mode", tempPos)) != std::string::npos)
 {
  statusModePos = tempPos;
  tempPos++;
 }

 if (statusModePos != std::string::npos)
 {
  size_t spanStart = searchArea.find("<span>", statusModePos);
  if (spanStart != std::string::npos)
  {
   spanStart += 6;
   size_t spanEnd = searchArea.find("</span>", spanStart);
   if (spanEnd != std::string::npos)
   {
    stats.status = searchArea.substr(spanStart, spanEnd - spanStart);
    stats.enabled = (stats.status == "Delivering Power");
   }
  }
 }

 // Extract power class
 size_t classPos = std::string::npos;
 tempPos = 0;
 while ((tempPos = searchArea.find("powClassShow", tempPos)) != std::string::npos)
 {
  classPos = tempPos;
  tempPos++;
 }

 if (classPos != std::string::npos)
 {
  size_t spanStart = searchArea.find(">", classPos);
  if (spanStart != std::string::npos)
  {
   spanStart++;
   size_t spanEnd = searchArea.find("</span>", spanStart);
   if (spanEnd != std::string::npos)
   {
    std::string classText = searchArea.substr(spanStart, spanEnd - spanStart);
    if (classText.find("ml003@") == 0 && classText.length() > 7)
    {
     size_t atPos = classText.find("@", 6);
     if (atPos != std::string::npos)
     {
      std::string classNum = classText.substr(6, atPos - 6);
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

 // Helper lambda to extract value
 auto extractValue = [&](const std::string &field) -> float
 {
  size_t fieldPos = html.find(field, portPos);
  if (fieldPos != std::string::npos && fieldPos < portPos + 2000)
  {
   size_t spanStart = html.find("<span>", fieldPos + 5);
   if (spanStart != std::string::npos)
   {
    spanStart += 6;
    size_t spanEnd = html.find("</span>", spanStart);
    if (spanEnd != std::string::npos)
    {
     try
     {
      return std::stof(html.substr(spanStart, spanEnd - spanStart));
     }
     catch (...)
     {
     }
    }
   }
  }
  return 0.0f;
 };

 stats.voltage = extractValue("ml570");
 stats.current = extractValue("ml572");
 stats.power = extractPortPower(html, port);
 stats.temperature = extractValue("ml575");

 // Extract fault status
 size_t ml581Pos = html.find("ml581", portPos);
 if (ml581Pos != std::string::npos && ml581Pos < portPos + 2000)
 {
  size_t spanStart = html.find("<span>", ml581Pos + 5);
  if (spanStart != std::string::npos)
  {
   spanStart += 6;
   size_t spanEnd = html.find("</span>", spanStart);
   if (spanEnd != std::string::npos)
   {
    stats.fault = html.substr(spanStart, spanEnd - spanStart);
   }
  }
 }

 return true;
}

// Action implementations
bool GS308EP_CLI::turnOnPort(int port, bool json, bool quiet)
{
 if (setPortState(port, true))
 {
  if (json)
  {
   outputJSON("{\"port\":" + std::to_string(port) + ",\"action\":\"on\",\"success\":true}");
  }
  else if (!quiet)
  {
   std::cout << "Port " << port << " turned ON" << std::endl;
  }
  return true;
 }

 if (json)
 {
  outputJSON("{\"port\":" + std::to_string(port) + ",\"action\":\"on\",\"success\":false}");
 }
 else
 {
  error("Failed to turn on port " + std::to_string(port));
 }
 return false;
}

bool GS308EP_CLI::turnOffPort(int port, bool json, bool quiet)
{
 if (setPortState(port, false))
 {
  if (json)
  {
   outputJSON("{\"port\":" + std::to_string(port) + ",\"action\":\"off\",\"success\":true}");
  }
  else if (!quiet)
  {
   std::cout << "Port " << port << " turned OFF" << std::endl;
  }
  return true;
 }

 if (json)
 {
  outputJSON("{\"port\":" + std::to_string(port) + ",\"action\":\"off\",\"success\":false}");
 }
 else
 {
  error("Failed to turn off port " + std::to_string(port));
 }
 return false;
}

bool GS308EP_CLI::cyclePort(int port, int delayMs, bool json, bool quiet)
{
 if (!setPortState(port, false))
 {
  if (json)
  {
   outputJSON("{\"port\":" + std::to_string(port) + ",\"action\":\"cycle\",\"success\":false}");
  }
  else
  {
   error("Failed to turn off port " + std::to_string(port));
  }
  return false;
 }

 if (!quiet && !json)
 {
  std::cout << "Port " << port << " turned OFF, waiting " << delayMs << "ms..." << std::endl;
 }

 usleep(delayMs * 1000);

 if (!setPortState(port, true))
 {
  if (json)
  {
   outputJSON("{\"port\":" + std::to_string(port) + ",\"action\":\"cycle\",\"success\":false}");
  }
  else
  {
   error("Failed to turn on port " + std::to_string(port));
  }
  return false;
 }

 if (json)
 {
  outputJSON("{\"port\":" + std::to_string(port) + ",\"action\":\"cycle\",\"delay\":" + std::to_string(delayMs) + ",\"success\":true}");
 }
 else if (!quiet)
 {
  std::cout << "Port " << port << " turned ON (cycle complete)" << std::endl;
 }

 return true;
}

bool GS308EP_CLI::showPortStatus(int port, bool json, bool quiet)
{
 bool status = getPortStatus(port);
 outputPortStatus(port, status, json, quiet);
 return true;
}

bool GS308EP_CLI::showPortPower(int port, bool json, bool quiet)
{
 std::string statusPage = httpGet(POE_STATUS_URL);
 if (last_response_code_ != 200)
 {
  error("Failed to fetch PoE status");
  return false;
 }

 float power = extractPortPower(statusPage, port);
 outputPortPower(port, power, json, quiet);
 return power >= 0;
}

bool GS308EP_CLI::showTotalPower(bool json, bool quiet)
{
 std::string statusPage = httpGet(POE_STATUS_URL);
 if (last_response_code_ != 200)
 {
  error("Failed to fetch PoE status");
  return false;
 }

 float total = 0.0f;
 for (int port = 1; port <= 8; port++)
 {
  float power = extractPortPower(statusPage, port);
  if (power >= 0)
  {
   total += power;
  }
 }

 outputTotalPower(total, json, quiet);
 return true;
}

bool GS308EP_CLI::showAllStats(bool json, bool quiet)
{
 std::string statusPage = httpGet(POE_STATUS_URL);
 if (last_response_code_ != 200)
 {
  error("Failed to fetch PoE status");
  return false;
 }

 std::vector<PoEPortStats> stats;
 for (int port = 1; port <= 8; port++)
 {
  PoEPortStats portStats;
  if (extractPortStats(statusPage, port, portStats))
  {
   stats.push_back(portStats);
  }
 }

 outputAllStats(stats, json, quiet);
 return !stats.empty();
}

// Output methods
void GS308EP_CLI::outputJSON(const std::string &json)
{
 std::cout << json << std::endl;
}

void GS308EP_CLI::outputPortStatus(int port, bool status, bool json, bool quiet)
{
 if (json)
 {
  outputJSON("{\"port\":" + std::to_string(port) + ",\"status\":\"" + (status ? "on" : "off") + "\"}");
 }
 else if (!quiet)
 {
  std::cout << "Port " << port << ": " << (status ? "ON" : "OFF") << std::endl;
 }
}

void GS308EP_CLI::outputPortPower(int port, float power, bool json, bool quiet)
{
 if (json)
 {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1);
  oss << "{\"port\":" << port << ",\"power\":" << power << "}";
  outputJSON(oss.str());
 }
 else if (!quiet)
 {
  std::cout << "Port " << port << " power: ";
  if (power >= 0)
  {
   std::cout << std::fixed << std::setprecision(1) << power << " W" << std::endl;
  }
  else
  {
   std::cout << "N/A" << std::endl;
  }
 }
}

void GS308EP_CLI::outputTotalPower(float power, bool json, bool quiet)
{
 if (json)
 {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(1);
  oss << "{\"total_power\":" << power << ",\"max_power\":65.0}";
  outputJSON(oss.str());
 }
 else if (!quiet)
 {
  std::cout << "Total PoE power: " << std::fixed << std::setprecision(1) << power << " W / 65.0 W" << std::endl;
 }
}

void GS308EP_CLI::outputAllStats(const std::vector<PoEPortStats> &stats, bool json, bool quiet)
{
 if (json)
 {
  std::ostringstream oss;
  oss << "{\"ports\":[";
  for (size_t i = 0; i < stats.size(); i++)
  {
   if (i > 0)
    oss << ",";
   oss << "{\"port\":" << (int)stats[i].port
       << ",\"enabled\":" << (stats[i].enabled ? "true" : "false")
       << ",\"status\":\"" << stats[i].status << "\""
       << ",\"class\":\"" << stats[i].powerClass << "\""
       << ",\"voltage\":" << std::fixed << std::setprecision(1) << stats[i].voltage
       << ",\"current\":" << std::fixed << std::setprecision(0) << stats[i].current
       << ",\"power\":" << std::fixed << std::setprecision(1) << stats[i].power
       << ",\"temperature\":" << std::fixed << std::setprecision(0) << stats[i].temperature
       << ",\"fault\":\"" << stats[i].fault << "\"}";
  }

  float total = std::accumulate(stats.begin(), stats.end(), 0.0f,
   [](float sum, const PoEPortStats &s) { return sum + s.power; });
  oss << "],\"total_power\":" << std::fixed << std::setprecision(1) << total << "}";
  outputJSON(oss.str());
 }
 else if (!quiet)
 {
  std::cout << std::endl;
  std::cout << "=== PoE Port Statistics ===" << std::endl;
  std::cout << std::endl;

  for (const auto &s : stats)
  {
   std::cout << "Port " << (int)s.port << ": " << s.status << std::endl;
   std::cout << "  Class: " << s.powerClass
             << "  |  Voltage: " << std::fixed << std::setprecision(1) << s.voltage << " V"
             << "  |  Current: " << std::fixed << std::setprecision(0) << s.current << " mA" << std::endl;
   std::cout << "  Power: " << std::fixed << std::setprecision(1) << s.power << " W"
             << "  |  Temperature: " << std::fixed << std::setprecision(0) << s.temperature << " °C"
             << "  |  Fault: " << s.fault << std::endl;
   std::cout << std::endl;
  }

  float total = std::accumulate(stats.begin(), stats.end(), 0.0f,
   [](float sum, const PoEPortStats &s) { return sum + s.power; });
  std::cout << "Total Power Budget Used: " << std::fixed << std::setprecision(1) << total << " W / 65.0 W" << std::endl;
 }
}

void GS308EP_CLI::log(const std::string &message)
{
 if (verbose_)
 {
  std::cerr << "[INFO] " << message << std::endl;
 }
}

void GS308EP_CLI::error(const std::string &message)
{
 std::cerr << "[ERROR] " << message << std::endl;
}

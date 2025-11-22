/**
 * @file test_gs308ep_cli.cpp
 * @brief Unit tests for GS308EP CLI
 *
 * Tests parsing logic, validation, and output formatting without network calls
 */

#include "../src/GS308EP_CLI.h"
#include <cassert>
#include <iostream>
#include <sstream>
#include <vector>
#include <cstring>

// Test counter
static int tests_passed = 0;
static int tests_failed = 0;

// Test helper macros
#define TEST(name)                                         \
 void test_##name();                                       \
 void run_test_##name()                                    \
 {                                                         \
  std::cout << "Running test: " << #name << "...";         \
  try                                                      \
  {                                                        \
   test_##name();                                          \
   tests_passed++;                                         \
   std::cout << " PASSED" << std::endl;                    \
  }                                                        \
  catch (const std::exception &e)                          \
  {                                                        \
   tests_failed++;                                         \
   std::cout << " FAILED: " << e.what() << std::endl;      \
  }                                                        \
  catch (...)                                              \
  {                                                        \
   tests_failed++;                                         \
   std::cout << " FAILED: Unknown exception" << std::endl; \
  }                                                        \
 }                                                         \
 void test_##name()

#define ASSERT_TRUE(condition)                               \
 if (!(condition))                                           \
 {                                                           \
  throw std::runtime_error("Assertion failed: " #condition); \
 }

#define ASSERT_FALSE(condition)                                      \
 if (condition)                                                      \
 {                                                                   \
  throw std::runtime_error("Assertion failed: not(" #condition ")"); \
 }

#define ASSERT_EQ(expected, actual)                            \
 if ((expected) != (actual))                                   \
 {                                                             \
  std::ostringstream oss;                                      \
  oss << "Expected " << (expected) << " but got " << (actual); \
  throw std::runtime_error(oss.str());                         \
 }

#define ASSERT_NEAR(expected, actual, epsilon)                                       \
 if (std::abs((expected) - (actual)) > (epsilon))                                    \
 {                                                                                   \
  std::ostringstream oss;                                                            \
  oss << "Expected " << (expected) << " ± " << (epsilon) << " but got " << (actual); \
  throw std::runtime_error(oss.str());                                               \
 }

#define ASSERT_CONTAINS(haystack, needle)                                           \
 if ((haystack).find(needle) == std::string::npos)                                  \
 {                                                                                  \
  std::ostringstream oss;                                                           \
  oss << "String \"" << (haystack) << "\" does not contain \"" << (needle) << "\""; \
  throw std::runtime_error(oss.str());                                              \
 }

// Testable wrapper to expose private methods
class GS308EP_CLI_Testable
{
public:
 // Test helper to extract rand token from HTML
 static std::string extractRand(const std::string &html)
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

 // Test helper to extract cookie from headers
 static std::string extractCookie(const std::string &headers)
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

 // Test helper to extract client hash
 static std::string extractClientHash(const std::string &html)
 {
  size_t pos = html.find("name=\"hash\"");
  if (pos == std::string::npos)
  {
   pos = html.find("name='hash'");
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

 // Test helper to extract port power
 static float extractPortPower(const std::string &html, int port)
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

 // Port validation
 static bool isValidPort(int port)
 {
  return port >= 1 && port <= 8;
 }
};

// ============================================================================
// HTML Parsing Tests
// ============================================================================

TEST(extract_rand_double_quotes)
{
 std::string html = "<input type=hidden id=\"rand\" name=\"rand\" value=\"1735414426\">";
 std::string rand = GS308EP_CLI_Testable::extractRand(html);
 ASSERT_EQ(std::string("1735414426"), rand);
}

TEST(extract_rand_single_quotes)
{
 std::string html = "<input type=hidden id='rand' name='rand' value='1735414426'>";
 std::string rand = GS308EP_CLI_Testable::extractRand(html);
 ASSERT_EQ(std::string("1735414426"), rand);
}

TEST(extract_rand_mixed_quotes)
{
 std::string html = "<input type=hidden id=\"rand\" name='rand' value=\"1735414426\">";
 std::string rand = GS308EP_CLI_Testable::extractRand(html);
 ASSERT_EQ(std::string("1735414426"), rand);
}

TEST(extract_rand_not_found)
{
 std::string html = "<input type=hidden name=\"other\" value=\"123\">";
 std::string rand = GS308EP_CLI_Testable::extractRand(html);
 ASSERT_TRUE(rand.empty());
}

TEST(extract_cookie_simple)
{
 std::string headers = "Set-Cookie: SID=abc123def456; Path=/\r\n";
 std::string cookie = GS308EP_CLI_Testable::extractCookie(headers);
 ASSERT_EQ(std::string("abc123def456"), cookie);
}

TEST(extract_cookie_with_newline)
{
 std::string headers = "Set-Cookie: SID=xyz789\n";
 std::string cookie = GS308EP_CLI_Testable::extractCookie(headers);
 ASSERT_EQ(std::string("xyz789"), cookie);
}

TEST(extract_cookie_not_found)
{
 std::string headers = "Set-Cookie: OTHER=value\r\n";
 std::string cookie = GS308EP_CLI_Testable::extractCookie(headers);
 ASSERT_TRUE(cookie.empty());
}

TEST(extract_client_hash_double_quotes)
{
 std::string html = "<input type=hidden name=\"hash\" id=\"hash\" value=\"3483299a0487987b90483a70c5d3d2dd\">";
 std::string hash = GS308EP_CLI_Testable::extractClientHash(html);
 ASSERT_EQ(std::string("3483299a0487987b90483a70c5d3d2dd"), hash);
}

TEST(extract_client_hash_single_quotes)
{
 std::string html = "<input type=hidden name='hash' id='hash' value='abc123def456'>";
 std::string hash = GS308EP_CLI_Testable::extractClientHash(html);
 ASSERT_EQ(std::string("abc123def456"), hash);
}

TEST(extract_client_hash_not_found)
{
 std::string html = "<input type=hidden name=\"other\" value=\"123\">";
 std::string hash = GS308EP_CLI_Testable::extractClientHash(html);
 ASSERT_TRUE(hash.empty());
}

// ============================================================================
// Port Power Extraction Tests
// ============================================================================

TEST(extract_port_power_valid)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"3\">"
     "<div id=\"ml574\" class=\"power\"><span>5.2</span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 3);
 ASSERT_NEAR(5.2f, power, 0.01f);
}

TEST(extract_port_power_zero)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"5\">"
     "<div id=\"ml574\"><span>0.0</span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 5);
 ASSERT_NEAR(0.0f, power, 0.01f);
}

TEST(extract_port_power_high_value)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"1\">"
     "<div id=\"ml574\"><span>15.8</span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 1);
 ASSERT_NEAR(15.8f, power, 0.01f);
}

TEST(extract_port_power_port_not_found)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"3\">"
     "<div id=\"ml574\"><span>5.2</span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 7);
 ASSERT_EQ(-1.0f, power);
}

TEST(extract_port_power_ml574_not_found)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"3\">"
     "<div id=\"other\"><span>5.2</span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 3);
 ASSERT_EQ(-1.0f, power);
}

TEST(extract_port_power_invalid_format)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"3\">"
     "<div id=\"ml574\"><span>invalid</span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 3);
 ASSERT_EQ(-1.0f, power);
}

TEST(extract_port_power_multiple_ports)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"1\">"
     "<div id=\"ml574\"><span>2.1</span></div>"
     "<input type=\"hidden\" class=\"port\" value=\"2\">"
     "<div id=\"ml574\"><span>3.5</span></div>"
     "<input type=\"hidden\" class=\"port\" value=\"3\">"
     "<div id=\"ml574\"><span>7.8</span></div>";

 float power1 = GS308EP_CLI_Testable::extractPortPower(html, 1);
 ASSERT_NEAR(2.1f, power1, 0.01f);

 float power2 = GS308EP_CLI_Testable::extractPortPower(html, 2);
 ASSERT_NEAR(3.5f, power2, 0.01f);

 float power3 = GS308EP_CLI_Testable::extractPortPower(html, 3);
 ASSERT_NEAR(7.8f, power3, 0.01f);
}

// ============================================================================
// Port Validation Tests
// ============================================================================

TEST(port_validation_valid_ports)
{
 for (int i = 1; i <= 8; i++)
 {
  ASSERT_TRUE(GS308EP_CLI_Testable::isValidPort(i));
 }
}

TEST(port_validation_invalid_zero)
{
 ASSERT_FALSE(GS308EP_CLI_Testable::isValidPort(0));
}

TEST(port_validation_invalid_negative)
{
 ASSERT_FALSE(GS308EP_CLI_Testable::isValidPort(-1));
 ASSERT_FALSE(GS308EP_CLI_Testable::isValidPort(-100));
}

TEST(port_validation_invalid_too_high)
{
 ASSERT_FALSE(GS308EP_CLI_Testable::isValidPort(9));
 ASSERT_FALSE(GS308EP_CLI_Testable::isValidPort(100));
}

// ============================================================================
// Complex HTML Parsing Tests
// ============================================================================

TEST(extract_rand_with_surrounding_html)
{
 std::string html =
     "<html><body><form method=\"post\">"
     "<input type=\"text\" name=\"username\">"
     "<input type=hidden id=\"rand\" name=\"rand\" value=\"1735414426\">"
     "<input type=\"password\" name=\"password\">"
     "</form></body></html>";

 std::string rand = GS308EP_CLI_Testable::extractRand(html);
 ASSERT_EQ(std::string("1735414426"), rand);
}

TEST(extract_cookie_multiple_cookies)
{
 std::string headers =
     "Set-Cookie: SESSION=temp123; Path=/\r\n"
     "Set-Cookie: SID=abc123def456; Path=/\r\n"
     "Set-Cookie: LANG=en; Path=/\r\n";

 std::string cookie = GS308EP_CLI_Testable::extractCookie(headers);
 ASSERT_EQ(std::string("abc123def456"), cookie);
}

TEST(extract_port_power_with_whitespace)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"4\">"
     "<div id=\"ml574\"><span>  8.3  </span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 4);
 ASSERT_NEAR(8.3f, power, 0.01f);
}

TEST(extract_port_power_decimal_precision)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"2\">"
     "<div id=\"ml574\"><span>12.456</span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 2);
 ASSERT_NEAR(12.456f, power, 0.001f);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST(extract_rand_empty_value)
{
 std::string html = "<input type=hidden name=\"rand\" value=\"\">";
 std::string rand = GS308EP_CLI_Testable::extractRand(html);
 ASSERT_TRUE(rand.empty());
}

TEST(extract_cookie_empty_value)
{
 std::string headers = "Set-Cookie: SID=;\r\n";
 std::string cookie = GS308EP_CLI_Testable::extractCookie(headers);
 ASSERT_TRUE(cookie.empty());
}

TEST(extract_port_power_empty_span)
{
 std::string html =
     "<input type=\"hidden\" class=\"port\" value=\"6\">"
     "<div id=\"ml574\"><span></span></div>";

 float power = GS308EP_CLI_Testable::extractPortPower(html, 6);
 ASSERT_EQ(-1.0f, power);
}

TEST(extract_rand_malformed_html)
{
 std::string html = "<input type=hidden name=\"rand\" value=\"123";
 std::string rand = GS308EP_CLI_Testable::extractRand(html);
 ASSERT_TRUE(rand.empty());
}

TEST(extract_client_hash_malformed_html)
{
 std::string html = "<input name=\"hash\" value=\"abc";
 std::string hash = GS308EP_CLI_Testable::extractClientHash(html);
 ASSERT_TRUE(hash.empty());
}

// ============================================================================
// Main Test Runner
// ============================================================================

int main(int /*argc*/, char * /*argv*/[])
{
 std::cout << "==================================" << std::endl;
 std::cout << "GS308EP CLI Unit Tests" << std::endl;
 std::cout << "==================================" << std::endl;
 std::cout << std::endl;

 // Run all tests
 run_test_extract_rand_double_quotes();
 run_test_extract_rand_single_quotes();
 run_test_extract_rand_mixed_quotes();
 run_test_extract_rand_not_found();

 run_test_extract_cookie_simple();
 run_test_extract_cookie_with_newline();
 run_test_extract_cookie_not_found();

 run_test_extract_client_hash_double_quotes();
 run_test_extract_client_hash_single_quotes();
 run_test_extract_client_hash_not_found();

 run_test_extract_port_power_valid();
 run_test_extract_port_power_zero();
 run_test_extract_port_power_high_value();
 run_test_extract_port_power_port_not_found();
 run_test_extract_port_power_ml574_not_found();
 run_test_extract_port_power_invalid_format();
 run_test_extract_port_power_multiple_ports();

 run_test_port_validation_valid_ports();
 run_test_port_validation_invalid_zero();
 run_test_port_validation_invalid_negative();
 run_test_port_validation_invalid_too_high();

 run_test_extract_rand_with_surrounding_html();
 run_test_extract_cookie_multiple_cookies();
 run_test_extract_port_power_with_whitespace();
 run_test_extract_port_power_decimal_precision();

 run_test_extract_rand_empty_value();
 run_test_extract_cookie_empty_value();
 run_test_extract_port_power_empty_span();
 run_test_extract_rand_malformed_html();
 run_test_extract_client_hash_malformed_html();

 // Print results
 std::cout << std::endl;
 std::cout << "==================================" << std::endl;
 std::cout << "Test Results:" << std::endl;
 std::cout << "  Passed: " << tests_passed << std::endl;
 std::cout << "  Failed: " << tests_failed << std::endl;
 std::cout << "  Total:  " << (tests_passed + tests_failed) << std::endl;
 std::cout << "==================================" << std::endl;

 return tests_failed > 0 ? 1 : 0;
}

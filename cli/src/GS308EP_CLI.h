/**
 * @file GS308EP_CLI.h
 * @brief CLI controller for GS308EP switch using libcurl
 */

#ifndef GS308EP_CLI_H
#define GS308EP_CLI_H

#include <string>
#include <map>
#include <vector>
#include <memory>

// Forward declaration
struct PoEPortStats
{
 uint8_t port;
 bool enabled;
 std::string status;
 float voltage;
 float current;
 float power;
 float temperature;
 std::string fault;
 std::string powerClass;
};

class GS308EP_CLI
{
public:
 GS308EP_CLI(const std::string &host, const std::string &password, bool verbose = false);
 ~GS308EP_CLI();

 // Authentication
 bool login();
 bool isAuthenticated() const { return authenticated_; }

 // Port control operations
 bool turnOnPort(int port, bool json, bool quiet);
 bool turnOffPort(int port, bool json, bool quiet);
 bool cyclePort(int port, int delayMs, bool json, bool quiet);
 bool showPortStatus(int port, bool json, bool quiet);

 // Power monitoring operations
 bool showPortPower(int port, bool json, bool quiet);
 bool showTotalPower(bool json, bool quiet);
 bool showAllStats(bool json, bool quiet);

private:
 std::string host_;
 std::string password_;
 std::string cookie_sid_;
 std::string client_hash_;
 bool authenticated_;
 bool verbose_;
 int last_response_code_;

 // HTTP operations
 std::string httpGet(const std::string &url);
 std::string httpPost(const std::string &url, const std::string &data);

 // Helper methods
 std::string extractRand(const std::string &html);
 std::string extractCookie(const std::string &headers);
 bool extractClientHash(const std::string &html);
 std::string md5Hash(const std::string &input);
 std::string mergeHash(const std::string &password, const std::string &rand);

 // Parsing methods
 bool getPortStatus(int port);
 float extractPortPower(const std::string &html, int port);
 bool extractPortStats(const std::string &html, int port, PoEPortStats &stats);
 bool setPortState(int port, bool enabled);

 // Output methods
 void outputJSON(const std::string &json);
 void outputPortStatus(int port, bool status, bool json, bool quiet);
 void outputPortPower(int port, float power, bool json, bool quiet);
 void outputTotalPower(float power, bool json, bool quiet);
 void outputAllStats(const std::vector<PoEPortStats> &stats, bool json, bool quiet);

 // Validation
 bool isValidPort(int port) const;

 // Logging
 void log(const std::string &message);
 void error(const std::string &message);
};

#endif // GS308EP_CLI_H

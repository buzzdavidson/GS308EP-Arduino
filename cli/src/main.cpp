/**
 * @file main.cpp
 * @brief CLI tool for controlling Netgear GS308EP PoE switch
 *
 * GNU-style command-line interface for switch management
 *
 * @version 0.5.0
 */

#include <iostream>
#include <string>
#include <vector>
#include <getopt.h>
#include <cstdlib>
#include <iomanip>
#include "GS308EP_CLI.h"

const char *VERSION = "0.5.0";
const char *PROGRAM_NAME = "gs308ep";

void print_version()
{
 std::cout << PROGRAM_NAME << " version " << VERSION << std::endl;
 std::cout << "Netgear GS308EP PoE Switch Control Tool" << std::endl;
}

void print_usage()
{
 std::cout << "Usage: " << PROGRAM_NAME << " [OPTIONS]" << std::endl;
 std::cout << std::endl;
 std::cout << "Control Netgear GS308EP PoE switch ports and monitor power consumption." << std::endl;
 std::cout << std::endl;
 std::cout << "Required options:" << std::endl;
 std::cout << "  -h, --host=HOST        Switch IP address or hostname" << std::endl;
 std::cout << "  -p, --password=PASS    Administrator password" << std::endl;
 std::cout << std::endl;
 std::cout << "Port control:" << std::endl;
 std::cout << "  -P, --port=NUM         Port number (1-8)" << std::endl;
 std::cout << "  -o, --on               Turn port ON" << std::endl;
 std::cout << "  -f, --off              Turn port OFF" << std::endl;
 std::cout << "  -c, --cycle[=DELAY]    Power cycle port (optional delay in ms, default 2000)" << std::endl;
 std::cout << "  -s, --status           Show port status" << std::endl;
 std::cout << std::endl;
 std::cout << "Power monitoring:" << std::endl;
 std::cout << "  -w, --power            Show power consumption for specified port" << std::endl;
 std::cout << "  -W, --total-power      Show total power consumption" << std::endl;
 std::cout << "  -S, --stats            Show comprehensive statistics for all ports" << std::endl;
 std::cout << std::endl;
 std::cout << "Output format:" << std::endl;
 std::cout << "  -j, --json             Output in JSON format" << std::endl;
 std::cout << "  -q, --quiet            Suppress non-essential output" << std::endl;
 std::cout << "  -v, --verbose          Enable verbose output" << std::endl;
 std::cout << std::endl;
 std::cout << "Other options:" << std::endl;
 std::cout << "      --help             Display this help and exit" << std::endl;
 std::cout << "      --version          Output version information and exit" << std::endl;
 std::cout << std::endl;
 std::cout << "Environment variables:" << std::endl;
 std::cout << "  GS308EP_HOST           Switch IP address (overridden by --host)" << std::endl;
 std::cout << "  GS308EP_PASSWORD       Administrator password (overridden by --password)" << std::endl;
 std::cout << std::endl;
 std::cout << "Examples:" << std::endl;
 std::cout << "  " << PROGRAM_NAME << " -h 192.168.1.1 -p admin -P 3 -o" << std::endl;
 std::cout << "    Turn on port 3" << std::endl;
 std::cout << std::endl;
 std::cout << "  " << PROGRAM_NAME << " -h 192.168.1.1 -p admin -P 5 -c 3000" << std::endl;
 std::cout << "    Power cycle port 5 with 3 second delay" << std::endl;
 std::cout << std::endl;
 std::cout << "  " << PROGRAM_NAME << " -h 192.168.1.1 -p admin -S --json" << std::endl;
 std::cout << "    Show all port statistics in JSON format" << std::endl;
 std::cout << std::endl;
 std::cout << "  " << PROGRAM_NAME << " -h 192.168.1.1 -p admin -W" << std::endl;
 std::cout << "    Show total power consumption" << std::endl;
}

int main(int argc, char *argv[])
{
 std::string host;
 std::string password;
 int port = -1;
 bool turn_on = false;
 bool turn_off = false;
 bool cycle = false;
 int cycle_delay = 2000;
 bool show_status = false;
 bool show_power = false;
 bool show_total_power = false;
 bool show_stats = false;
 bool json_output = false;
 bool quiet = false;
 bool verbose = false;

 // Check environment variables
 const char *env_host = std::getenv("GS308EP_HOST");
 const char *env_password = std::getenv("GS308EP_PASSWORD");

 if (env_host)
 {
  host = env_host;
 }
 if (env_password)
 {
  password = env_password;
 }

 static struct option long_options[] = {
     {"host", required_argument, 0, 'h'},
     {"password", required_argument, 0, 'p'},
     {"port", required_argument, 0, 'P'},
     {"on", no_argument, 0, 'o'},
     {"off", no_argument, 0, 'f'},
     {"cycle", optional_argument, 0, 'c'},
     {"status", no_argument, 0, 's'},
     {"power", no_argument, 0, 'w'},
     {"total-power", no_argument, 0, 'W'},
     {"stats", no_argument, 0, 'S'},
     {"json", no_argument, 0, 'j'},
     {"quiet", no_argument, 0, 'q'},
     {"verbose", no_argument, 0, 'v'},
     {"help", no_argument, 0, 0},
     {"version", no_argument, 0, 1},
     {0, 0, 0, 0}};

 int option_index = 0;
 int c;

 while ((c = getopt_long(argc, argv, "h:p:P:ofc::swWSjqv", long_options, &option_index)) != -1)
 {
  switch (c)
  {
  case 0: // --help
   print_usage();
   return 0;
  case 1: // --version
   print_version();
   return 0;
  case 'h':
   host = optarg;
   break;
  case 'p':
   password = optarg;
   break;
  case 'P':
   port = std::atoi(optarg);
   if (port < 1 || port > 8)
   {
    std::cerr << "Error: Port must be between 1 and 8" << std::endl;
    return 1;
   }
   break;
  case 'o':
   turn_on = true;
   break;
  case 'f':
   turn_off = true;
   break;
  case 'c':
   cycle = true;
   if (optarg)
   {
    cycle_delay = std::atoi(optarg);
    if (cycle_delay < 0)
    {
     std::cerr << "Error: Cycle delay must be non-negative" << std::endl;
     return 1;
    }
   }
   break;
  case 's':
   show_status = true;
   break;
  case 'w':
   show_power = true;
   break;
  case 'W':
   show_total_power = true;
   break;
  case 'S':
   show_stats = true;
   break;
  case 'j':
   json_output = true;
   break;
  case 'q':
   quiet = true;
   break;
  case 'v':
   verbose = true;
   break;
  case '?':
   return 1;
  default:
   std::cerr << "Error: Unknown option" << std::endl;
   return 1;
  }
 }

 // Validate required arguments
 if (host.empty())
 {
  std::cerr << "Error: Switch host is required (use --host or GS308EP_HOST)" << std::endl;
  std::cerr << "Try '" << PROGRAM_NAME << " --help' for more information." << std::endl;
  return 1;
 }

 if (password.empty())
 {
  std::cerr << "Error: Switch password is required (use --password or GS308EP_PASSWORD)" << std::endl;
  std::cerr << "Try '" << PROGRAM_NAME << " --help' for more information." << std::endl;
  return 1;
 }

 // Validate action combinations
 int action_count = turn_on + turn_off + cycle + show_status + show_power + show_total_power + show_stats;
 if (action_count == 0)
 {
  std::cerr << "Error: No action specified" << std::endl;
  std::cerr << "Try '" << PROGRAM_NAME << " --help' for more information." << std::endl;
  return 1;
 }

 if (action_count > 1)
 {
  std::cerr << "Error: Only one action can be specified at a time" << std::endl;
  return 1;
 }

 // Port-specific actions require port number
 if ((turn_on || turn_off || cycle || show_status || show_power) && port == -1)
 {
  std::cerr << "Error: Port number required for this action (use --port)" << std::endl;
  return 1;
 }

 // Create CLI controller
 GS308EP_CLI controller(host, password, verbose);

 // Connect and authenticate
 if (!quiet && !json_output)
 {
  std::cout << "Connecting to " << host << "..." << std::endl;
 }

 if (!controller.login())
 {
  std::cerr << "Error: Authentication failed" << std::endl;
  return 1;
 }

 if (!quiet && !json_output)
 {
  std::cout << "Authenticated successfully" << std::endl;
 }

 // Execute action
 bool success = false;

 if (turn_on)
 {
  success = controller.turnOnPort(port, json_output, quiet);
 }
 else if (turn_off)
 {
  success = controller.turnOffPort(port, json_output, quiet);
 }
 else if (cycle)
 {
  success = controller.cyclePort(port, cycle_delay, json_output, quiet);
 }
 else if (show_status)
 {
  success = controller.showPortStatus(port, json_output, quiet);
 }
 else if (show_power)
 {
  success = controller.showPortPower(port, json_output, quiet);
 }
 else if (show_total_power)
 {
  success = controller.showTotalPower(json_output, quiet);
 }
 else if (show_stats)
 {
  success = controller.showAllStats(json_output, quiet);
 }

 return success ? 0 : 1;
}

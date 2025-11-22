/**
 * Basic PoE Control Example
 *
 * This example demonstrates how to use the GS308EP library to control
 * PoE ports on a Netgear GS308EP switch from an ESP32.
 *
 * Hardware Required:
 * - ESP32 development board (with or without Ethernet)
 * - Netgear GS308EP switch on the same network
 *
 * Tested Boards:
 * - Waveshare ESP32-S3 ETH (W5500 SPI Ethernet)
 * - Generic ESP32 with WiFi
 *
 * Setup:
 * 1. Choose connection type (WiFi or Ethernet) by setting USE_ETHERNET
 * 2. Update credentials below
 * 3. Update switch IP address and password
 * 4. Upload to ESP32
 */

#include <GS308EP.h>
#include <SPI.h>

// Connection type selection
#define USE_ETHERNET true // Set to true for Ethernet, false for WiFi

#if USE_ETHERNET
#include <Ethernet.h>

// Waveshare ESP32-S3 ETH Board with W5500 configuration
#define W5500_CS 10 // Chip Select
#define W5500_INT 4 // Interrupt pin
#define W5500_RST 5 // Reset pin

// MAC address - adjust if needed (must be unique on your network)
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};
#else
#include <WiFi.h>
// WiFi credentials
const char *ssid = "YourWiFiSSID";
const char *password = "YourWiFiPassword";
#endif

// Switch credentials
const char *switchIP = "192.168.1.1";    // Change to your switch IP
const char *switchPassword = "password"; // Change to your switch password

// Create switch object
GS308EP poeSwitch(switchIP, switchPassword);

void setup()
{
 Serial.begin(115200);
 delay(1000);

 Serial.println("\nGS308EP PoE Control Example");
 Serial.println("============================\n");

#if USE_ETHERNET
 // Connect via Ethernet (W5500)
 Serial.println("Initializing W5500 Ethernet...");

 // Initialize W5500 reset pin
 pinMode(W5500_RST, OUTPUT);
 digitalWrite(W5500_RST, LOW);
 delay(50);
 digitalWrite(W5500_RST, HIGH);
 delay(200);

 // Initialize SPI for W5500
 SPI.begin();
 Ethernet.init(W5500_CS);

 // Start Ethernet with DHCP
 Serial.print("Connecting to network via DHCP");
 if (Ethernet.begin(mac) == 0)
 {
  Serial.println("\nFailed to configure Ethernet using DHCP");

  // Try to configure with static IP as fallback (adjust to your network)
  // IPAddress ip(192, 168, 1, 177);
  // IPAddress dns(192, 168, 1, 1);
  // IPAddress gateway(192, 168, 1, 1);
  // IPAddress subnet(255, 255, 255, 0);
  // Ethernet.begin(mac, ip, dns, gateway, subnet);
  return;
 }

 // Wait for link up
 int timeout = 20; // 10 seconds
 while (Ethernet.linkStatus() != LinkON && timeout > 0)
 {
  delay(500);
  Serial.print(".");
  timeout--;
 }

 if (Ethernet.linkStatus() != LinkON)
 {
  Serial.println("\nNo Ethernet cable connected!");
  return;
 }

 Serial.println("\nEthernet connected!");
 Serial.print("IP address: ");
 Serial.println(Ethernet.localIP());
 Serial.print("Gateway: ");
 Serial.println(Ethernet.gatewayIP());
 Serial.print("Subnet mask: ");
 Serial.println(Ethernet.subnetMask());
#else
 // Connect via WiFi
 Serial.print("Connecting to WiFi");
 WiFi.begin(ssid, password);

 while (WiFi.status() != WL_CONNECTED)
 {
  delay(500);
  Serial.print(".");
 }

 Serial.println("\nWiFi connected!");
 Serial.print("IP address: ");
 Serial.println(WiFi.localIP());
#endif

 // Initialize the library
 Serial.println("\nInitializing GS308EP library...");
 if (!poeSwitch.begin())
 {
  Serial.println("Failed to initialize library");
  return;
 }

 // Login to the switch
 Serial.print("Logging in to switch at ");
 Serial.print(switchIP);
 Serial.println("...");

 if (!poeSwitch.login())
 {
  Serial.println("Login failed!");
  Serial.print("HTTP Response Code: ");
  Serial.println(poeSwitch.getLastResponseCode());
  return;
 }

 Serial.println("Login successful!\n");

 // Demonstrate comprehensive statistics
 demonstrateAllStatistics();

 // Demonstrate power monitoring
 demonstratePowerMonitoring();

 // Demonstrate port control
 demonstratePortControl();
}

void loop()
{
 // Nothing to do in loop for this example
 delay(1000);
}

void demonstrateAllStatistics()
{
 Serial.println("=== Comprehensive PoE Statistics ===\n");

 // Get all port statistics in a single call
 PoEPortStats allStats[8];
 if (!poeSwitch.getAllPoEPortStats(allStats))
 {
  Serial.println("Error: Failed to retrieve statistics");
  Serial.println();
  return;
 }

 // Display comprehensive statistics for each port
 for (int i = 0; i < 8; i++)
 {
  PoEPortStats &stats = allStats[i];

  Serial.print("Port ");
  Serial.print(stats.port);
  Serial.print(": ");
  Serial.println(stats.status);

  Serial.print("  Class: ");
  Serial.print(stats.powerClass);
  Serial.print("  |  Voltage: ");
  Serial.print(stats.voltage, 1);
  Serial.print(" V  |  Current: ");
  Serial.print(stats.current, 0);
  Serial.println(" mA");

  Serial.print("  Power: ");
  Serial.print(stats.power, 1);
  Serial.print(" W  |  Temperature: ");
  Serial.print(stats.temperature, 0);
  Serial.print(" °C  |  Fault: ");
  Serial.println(stats.fault);

  Serial.println();
 }

 // Calculate and display total power
 float totalPower = 0.0;
 for (int i = 0; i < 8; i++)
 {
  totalPower += allStats[i].power;
 }

 Serial.print("Total Power Budget Used: ");
 Serial.print(totalPower, 1);
 Serial.println(" W / 65.0 W");
 Serial.println();
}

void demonstratePowerMonitoring()
{
 Serial.println("=== Power Monitoring Demonstration ===\n");

 // Check individual port power consumption
 Serial.println("Per-Port Power Consumption:");
 for (uint8_t port = 1; port <= 8; port++)
 {
  float power = poeSwitch.getPoEPortPower(port);
  Serial.print("  Port ");
  Serial.print(port);
  Serial.print(": ");
  if (power >= 0.0)
  {
   Serial.print(power, 1);
   Serial.println(" W");
  }
  else
  {
   Serial.println("Error reading power");
  }
 }

 // Get total power consumption
 Serial.println();
 float totalPower = poeSwitch.getTotalPoEPower();
 Serial.print("Total PoE Power: ");
 if (totalPower >= 0.0)
 {
  Serial.print(totalPower, 1);
  Serial.println(" W");
 }
 else
 {
  Serial.println("Error reading total power");
 }

 Serial.println();
}

void demonstratePortControl()
{
 uint8_t testPort = 1; // Test with port 1

 Serial.println("=== Port Control Demonstration ===\n");

 // Check initial port status
 Serial.print("Checking initial status of port ");
 Serial.print(testPort);
 Serial.println("...");

 bool initialStatus = poeSwitch.getPoEPortStatus(testPort);
 Serial.print("Port ");
 Serial.print(testPort);
 Serial.print(" is currently: ");
 Serial.println(initialStatus ? "ON" : "OFF");
 Serial.println();

 // Turn port OFF
 Serial.print("Turning port ");
 Serial.print(testPort);
 Serial.println(" OFF...");

 if (poeSwitch.turnOffPoEPort(testPort))
 {
  Serial.println("Success!");
 }
 else
 {
  Serial.println("Failed!");
 }

 delay(2000);

 // Verify it's off
 bool statusAfterOff = poeSwitch.getPoEPortStatus(testPort);
 Serial.print("Port status: ");
 Serial.println(statusAfterOff ? "ON" : "OFF");
 Serial.println();

 // Turn port ON
 Serial.print("Turning port ");
 Serial.print(testPort);
 Serial.println(" ON...");

 if (poeSwitch.turnOnPoEPort(testPort))
 {
  Serial.println("Success!");
 }
 else
 {
  Serial.println("Failed!");
 }

 delay(2000);

 // Verify it's on
 bool statusAfterOn = poeSwitch.getPoEPortStatus(testPort);
 Serial.print("Port status: ");
 Serial.println(statusAfterOn ? "ON" : "OFF");
 Serial.println();

 // Demonstrate power cycling
 Serial.println("=== Power Cycle Demonstration ===\n");
 Serial.print("Power cycling port ");
 Serial.print(testPort);
 Serial.println("...");
 Serial.println("(OFF for 2 seconds, then ON)");

 if (poeSwitch.cyclePoEPort(testPort, 2000))
 {
  Serial.println("Power cycle complete!");
 }
 else
 {
  Serial.println("Power cycle failed!");
 }

 Serial.println("\n=== Demonstration Complete ===");
 Serial.println("You can now modify this code to control ports as needed.");
}

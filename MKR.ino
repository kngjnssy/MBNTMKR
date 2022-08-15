#include <SPI.h>
#include <WiFi101.h>
#include <WiFi101OTA.h>
#include <ThingerWifi101.h>
#include <Arduino_WiFiConnectionHandler.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"
#include "MHZ19.h"

MHZ19 myMHZ19;

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int status = WL_IDLE_STATUS;     // the Wifi radio's status

WiFiConnectionHandler net(ssid, pass);
bool OTA_Configured = false;

#define DHTPIN 2     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht(DHTPIN, DHTTYPE);

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

long rssi;
ThingerWifi101 thing(USERNAME, DEVICE_ID, DEVICE_CREDENTIAL);

void setup() {
  setDebugMessageLevel(DBG_INFO);

  net.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  net.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  net.addCallback(NetworkConnectionEvent::ERROR, onNetworkError);

  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }
  Serial.println("You're connected to the network");
  Serial.println("----------------------------------------");
  printWLData();
  Serial.println("----------------------------------------");

  thing.add_wifi(SECRET_SSID, SECRET_PASS);

  dht.begin();

  // MZH-19B setup
  Serial1.begin(9600);  //Initialisierung der seriellen Schnittstelle für den ersten Sensor
  myMHZ19.begin(Serial1);                                // *Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();

  thing["Vitals"] >> [](pson & out) {
    out["temperature"] = dht.readTemperature();
    out["humidity"] = dht.readHumidity();
    out["co2custom"] = getCO2UART();
    out["co2lib"] = myMHZ19.getCO2();
    out["rssi"] = WiFi.RSSI();
  };

}

void onNetworkConnect()
{
  Serial.println(">>>> CONNECTED to network");
  WiFiOTA.begin("MKR", DEVICEPASS, InternalStorage);
  OTA_Configured = true;
}

void onNetworkDisconnect()
{
  Serial.println(">>>> DISCONNECTED from network");
  OTA_Configured = false;
}

void onNetworkError() {
  Serial.println(">>>> ERROR");
}

void printWLData() {
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);
  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
}

void loop() {
  net.check();
  if (OTA_Configured)
  {
    WiFiOTA.poll();
    thing.handle();

    float h = dht.readHumidity();
    float t = dht.readTemperature();
    static int CO2 = 0;
    CO2 = myMHZ19.getCO2();

    display.clearDisplay();
    //    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

    display.setCursor(6, 3);
    display.print("T ");
    display.print(t);
    display.print(" C");
    display.setCursor(6, 12);
    display.print("H ");
    display.print(h);
    display.print(" %");

    display.setCursor(6, 21);
    display.print("RSSI ");
    display.print(rssi);

    getCO2UART();
    display.setCursor(80, 3);
    display.print("custom/lib");
    display.setCursor(80, 21);
    display.print(CO2);
    display.print(" PPM/");
    display.display();
    delay(5000);

  }
}

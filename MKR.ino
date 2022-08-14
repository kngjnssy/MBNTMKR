#include <SPI.h>
#include <WiFi101.h>
#include <WiFi101OTA.h>
//#include <ArduinoOTA.h>
#include <ThingerWifi101.h>
#include <Arduino_WiFiConnectionHandler.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "secrets.h"

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

float hum;  //Stores humidity value
float temp; //Stores temperature value

ThingerWifi101 thing(USERNAME, DEVICENAME, TOKEN);

void setup() {
  setDebugMessageLevel(DBG_INFO);

  net.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  net.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  net.addCallback(NetworkConnectionEvent::ERROR, onNetworkError);


  // start the WiFi OTA library with internal (flash) based storage
  //  WiFiOTA.begin("Arduino", "password", InternalStorage);

  thing.add_wifi(SECRET_SSID, SECRET_PASS);
  thing["Temperature"] >> [](pson & out) {
    out = temperatureInC();
  };
  thing["Humidity"] >> [](pson & out) {
    out = humidity();
  };
  thing["CO2"] >> [](pson & out) {
    out = co2ppm();
  };
  thing["rssi"] >> [](pson & out) {
    out = WIFIrssi();
  };

  // MZH-19B setup
  Serial1.begin(9600);  //Initialisierung der seriellen Schnittstelle für den ersten Sensor

  Serial.begin(9600);
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  // attempt to connect to Wifi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to network: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network:
    status = WiFi.begin(ssid, pass);
  }

  display.setTextSize(1);
  display.setTextColor(WHITE, BLACK);
  display.clearDisplay();

  // you're connected now, so print out the data:
  Serial.println("You're connected to the network");

  Serial.println("----------------------------------------");
  printData();
  printWifiStatus();
  Serial.println("----------------------------------------");
  dht.begin();
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


void printData() {
  Serial.println("Board Information:");
  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  Serial.println();
  Serial.println("Network Information:");
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
}

void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
  display.setCursor(95, 3);
  display.print("RSSI");
  display.setCursor(95, 12);
  display.print(rssi);
  display.setCursor(95, 21);
  display.print("dBm");
  display.display();
}

byte getCheckSum(byte *packet)
{
  byte checksum = 0;
  for ( int i = 1; i < 8; i++)
  {
    checksum += packet[i];
  }
  checksum = 0xFF - checksum;
  checksum += 1;
  return checksum;
}

void readCO2() {
  // Read MH-Z19B
  Serial.println("Read MH-Z19...");
  byte com[] = {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};  //Befehl zum Auslesen der CO2 Konzentration
  byte return1[9];                                                      //hier kommt der zurückgegeben Wert des ersten Sensors rein

  while (Serial1.available() > 0) {    //Hier wird der Buffer der Seriellen Schnittstelle gelehrt
    Serial1.read();
  }

  Serial1.write(com, 9);                       //Befehl zum Auslesen der CO2 Konzentration
  Serial1.readBytes(return1, 9);               //Auslesen der Antwort

  int concentration = 0;                      //CO2-Konzentration des Sensors
  if (getCheckSum(return1) != return1[8]) {
    Serial.println("Checksum error");
  } else {
    concentration = return1[2] * 256 + return1[3]; //Berechnung der Konzentration
    Serial.print("MH-Z19 sample OK: ");
    Serial.print(concentration); ; Serial.println(" ppm");
  }
  display.setCursor(6, 21);
  display.print("CO2 ");
  display.print(concentration);
  display.print(" PPM");
  display.display();
  delay(2000);
}

unsigned long lastMillis = 0;

float temperatureInC() {
  float tempinC = dht.readTemperature();
  return tempinC;
}

float humidity() {
  float hum = dht.readHumidity();
  return hum;
}

float WIFIrssi() {
  long rssi = WiFi.RSSI();
  return rssi;
}

float co2ppm() {
  // Read MH-Z19B
  Serial.println("Read MH-Z19...");
  byte com[] = {0xff, 0x01, 0x86, 0x00, 0x00, 0x00, 0x00, 0x00, 0x79};  //Befehl zum Auslesen der CO2 Konzentration
  byte return1[9];                                                      //hier kommt der zurückgegeben Wert des ersten Sensors rein

  while (Serial1.available() > 0) {    //Hier wird der Buffer der Seriellen Schnittstelle gelehrt
    Serial1.read();
  }

  Serial1.write(com, 9);                       //Befehl zum Auslesen der CO2 Konzentration
  Serial1.readBytes(return1, 9);               //Auslesen der Antwort

  int concentration = 0;                      //CO2-Konzentration des Sensors
  if (getCheckSum(return1) != return1[8]) {
    Serial.println("Checksum error");
  } else {
    concentration = return1[2] * 256 + return1[3]; //Berechnung der Konzentration
    Serial.print("MH-Z19 sample OK: ");
    Serial.print(concentration); ; Serial.println(" ppm");
  }
  return concentration;
}


void loop() {
  net.check();
  if (OTA_Configured)
  {
    WiFiOTA.poll();
    thing.handle();

    if (status == WL_CONNECTED) {
      printWifiStatus();
    }

    float h = dht.readHumidity();
    float t = dht.readTemperature();

    if (isnan(h) || isnan(t)) {
      Serial.println(F("Failed to read from DHT sensor!"));
      return;
    }

    float hic = dht.computeHeatIndex(t, h, false);   // Compute heat index in Celsius (isFahreheit = false)

    Serial.print(F("Humidity: "));
    Serial.print(h);
    Serial.print(F("%  Temperature: "));
    Serial.print(t);
    Serial.print(F("°C "));
    Serial.print(F("Heat index: "));
    Serial.print(hic);
    Serial.println(F("°C "));
    delay(5000);
    display.clearDisplay();
    display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);

    display.setCursor(6, 3);
    display.print("T ");
    display.print(t);
    display.print(" C");
    display.setCursor(6, 12);
    display.print("H ");
    display.print(h);
    display.print(" %");
    display.display();
    readCO2();
  }



}

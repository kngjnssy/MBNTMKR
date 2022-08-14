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

//const int pwmpin = 4;
//const int range = 5000;

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
long rssi;
ThingerWifi101 thing(USERNAME, DEVICENAME, TOKEN);

void setup() {
  setDebugMessageLevel(DBG_INFO);

  net.addCallback(NetworkConnectionEvent::CONNECTED, onNetworkConnect);
  net.addCallback(NetworkConnectionEvent::DISCONNECTED, onNetworkDisconnect);
  net.addCallback(NetworkConnectionEvent::ERROR, onNetworkError);

  //  pinMode(pwmpin, INPUT);

  thing.add_wifi(SECRET_SSID, SECRET_PASS);

  thing["Temperature"] >> [](pson & out) {
    out = temperatureInC();
  };
  thing["Humidity"] >> [](pson & out) {
    out = humidity();
  };
  thing["CO2"] >> [](pson & out) {
    out = getCO2UART();
  };
    thing["CO2lib"] >> [](pson & out) {
      out = CO2lib();
    };

  thing["rssi"] >> [](pson & out) {
    out = WIFIrssi();
  };

  // MZH-19B setup
  Serial1.begin(9600);  //Initialisierung der seriellen Schnittstelle für den ersten Sensor
  myMHZ19.begin(Serial1);                                // *Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

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
  rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.println(rssi);
}


float temperatureInC() {
  float tempinC = dht.readTemperature();
  return tempinC;
}

float humidity() {
  float hum = dht.readHumidity();
  return hum;
}

float WIFIrssi() {
  int rssi = WiFi.RSSI();
  return rssi;
}

float CO2lib() {
  int co2lib = myMHZ19.getCO2();
  return co2lib;
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
    CO2 = myMHZ19.getCO2();  // Request CO2 (as ppm)

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
    display.setCursor(80, 12);
    display.print(CO2);
    display.display();

  }
}

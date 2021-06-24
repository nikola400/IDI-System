#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Wire.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_PCF8574.h>
#include "config.h"

#define SEALEVELPRESSURE_HPA (1013.25)

// LED Lights Pins
const int LED_PIN_GREEN = 22;
const int LED_PIN_RED = 23;
const int LED_PIN_YELLOW = 21;

// WiFi configurations and variables
//const char* ssid = NETWORK_SECRET_SSID;
//const char* password = NETWORK_SECRET_PASS;
const char* ssid = "A1_C5B4";
const char* password = "48575443B036A4A4";
//WiFiServer server(80);

//I2C Pins 
const int PIN_SDA = 7;
//// BME 280 Sensor for temperature and pressure
const int PIN_SCL = 6;
Adafruit_BME280 bme; // I2C

// LCD Liquid Crystal Display
LiquidCrystal_PCF8574 lcd(0x27);

// Web Server
WebServer server(80);

// Soil Moisture Sensor
const int AirValue = 3620;
const int WaterValue = 1680;
const int TOMATO_SOIL_MOISTURE_MIN = 70;
const int LED_ANALOG_SOIL_MOISTURE_PIN = 36;
int soilMoistureValue = 0;
int soilmoisturePercent = 0;

const long sleepTime = 3600000; //1 hour dalay time

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  //Set pins mode
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);

  // power on lights to show esp32 is working
  lightsOn();

  // Establish WiFi Connection
  boolean connectedWiFi = connectToWiFi();
  if (connectedWiFi) {
    Serial.println("Connected to WiFi successfully!");
  } else {
    Serial.println("Cannot connect to WiFi successfully!");
  }

  createWebServer();

  connectBME280();

  lcd.begin(PIN_SCL, PIN_SDA);
  
}

void loop() {
  // Print metrisc from BME 280
  printToLCD();
  printToSerial();
  printSoilMoisturePercentToSerial();

  server.handleClient();

  sendSignalValve();
}

void printSoilMoisturePercentToSerial() {
  Serial.print("Soil Moisture Percent: ");
  Serial.print(getSoilMoisturePercent());
  Serial.println("%");
}

int getSoilMoisturePercent() {
  int soilMoistureValue = analogRead(LED_ANALOG_SOIL_MOISTURE_PIN);
  int soilMoisturePercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);

  if (soilMoisturePercent > 100) {
    return 100;
  }

  if (soilMoisturePercent < 0) {
    return 0;
  }

  return soilMoisturePercent;
}

void lightsOn() {
  digitalWrite(LED_PIN_RED, HIGH);
  delay(500);
  digitalWrite(LED_PIN_RED, LOW);
  digitalWrite(LED_PIN_GREEN, HIGH);
  delay(500);
  digitalWrite(LED_PIN_GREEN, LOW);
  digitalWrite(LED_PIN_RED, HIGH);
  delay(500);
  digitalWrite(LED_PIN_RED, LOW);
  digitalWrite(LED_PIN_GREEN, HIGH);
  delay(500);
  digitalWrite(LED_PIN_GREEN, LOW);
}

void turnOnRed() {
  digitalWrite(LED_PIN_GREEN, LOW);
  digitalWrite(LED_PIN_RED, HIGH);
}

void turnOnGreen() {
  digitalWrite(LED_PIN_RED, LOW);
  digitalWrite(LED_PIN_GREEN, HIGH);
}

void turnLightsOff() {
  digitalWrite(LED_PIN_RED, LOW);
  digitalWrite(LED_PIN_GREEN, LOW);
}

boolean connectToWiFi() {
  //not connected yet
  turnOnRed();
  
  boolean foundNetwork = scanNetworks();
  if (!foundNetwork) {
    Serial.print("Network ");
    Serial.print(ssid);
    Serial.print(" not found");
    return false;
  }
    
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println(WiFi.status());
  }

  // connected
  turnOnGreen();
  
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  return true;
}

// Scanning WiFi Networks
boolean scanNetworks() {
  Serial.println("Scanning available networks...");
  int n = WiFi.scanNetworks();
  Serial.println("Scan done.");
  
  boolean foundNetwork = false;
  if (n == 0) {
     Serial.println("no networks found");
     return false;
  }
  
  Serial.print(n);
  Serial.println(" networks found");
  for (int i = 0; i < n; ++i) {
    // Check for the network we are looking
    if (WiFi.SSID(i).equals(ssid)) {
      foundNetwork = true;
    }
    
    // Print SSID and RSSI for each network found
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.print(WiFi.SSID(i));
    Serial.print(" (");
    Serial.print(WiFi.RSSI(i));
    Serial.print("): ");
    Serial.print(WiFi.BSSIDstr(i));
    Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN)?" ":"*");
    delay(10);          
  }
  return foundNetwork;
}

void connectBME280() {
  unsigned status;
    
  Wire.begin(PIN_SCL, PIN_SDA);
  
  // default settings
  status = bme.begin(0x76);  
  // You can also pass in a Wire library object like &Wire2
  // status = bme.begin(0x76, &Wire2)
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
      Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
      Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
      Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
      Serial.print("        ID of 0x60 represents a BME 280.\n");
      Serial.print("        ID of 0x61 represents a BME 680.\n");
      while (1) delay(10);
  } else {
    Serial.println("BME 280 found Successfully");
  }
}

void printToLCD() {
  Serial.println("Printing Temprerature and Pressure from BME280 to LCD Liquid Crystal display!");
  
  lcd.setBacklight(50);
  lcd.home();
  lcd.clear();
  //first line
  lcd.setCursor(0,0);
  lcd.print("Temp: ");
  lcd.print(bme.readTemperature());
  lcd.print("C");
  //second line
  lcd.setCursor(0,1);
  lcd.print("Pres: ");
  lcd.println(bme.readPressure() / 100.0F);
  lcd.print("hPa");
}

void printToSerial() {
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");

  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}

void createWebServer() {
  if (MDNS.begin("esp32")) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/test", []() {
    server.send(200, "text/plain", "Web Server works well");
  });

  server.on("/temp", returnTemp);

  server.on("/hum", returnHum);

  server.on("/soilMoisture", returnSoilMoisture);

  server.on("/addPlant", addPlant);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "This is IDI System Web Server!\n";
  response += "You can call these endpoints: \n";
  response += "/temp  -get air temperature \n";
  response += "/hum  -get air humidity \n";
  response += "/soilMoisture  -get soilMoisture \n";
  response += "/addPlant  -add new plant for monitoring \n\n";

  
  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnTemp() {
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Air Temperature: ";
  response += bme.readTemperature();
  response += "C";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnHum() {
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Air Humidity: ";
  response += bme.readHumidity();
  response += "%";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnSoilMoisture() {
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Soil Moisture: ";
  response += getSoilMoisturePercent();
  response += "%";
  
  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

// TODO
void addPlant() {
  digitalWrite(LED_PIN_YELLOW, HIGH);
  
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void handleNotFound() {
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Bad request\n\n";
  response += "To view available ednpoints call /";
  
  server.send(400, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

boolean checkForWatering() {
  //TODO: check time (do not water in the middle of the day)

  int currentSoilMoisture = getSoilMoisturePercent();
  if (currentSoilMoisture < TOMATO_SOIL_MOISTURE_MIN) {
    return true;
  }

  return false;
}

void sendSignalValve() {
  //TODO send signal to the valve if watering is needed
  if (checkForWatering()) {
    // send signal
  }
}

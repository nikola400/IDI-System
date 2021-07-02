#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <LiquidCrystal_PCF8574.h>
#include "config.h"

// SPI Comunication Pins
#define BME_SCK 14
#define BME_MISO 12
#define BME_MOSI 13
#define BME_CS 15

// LED Lights Pins
#define LED_PIN_GREEN 22
#define LED_PIN_RED 23
#define LED_PIN_YELLOW 21

// Water Valve Pin
#define VALVE_PIN 35

// Soil Moisture Pin
#define SOIL_MOISTURE_ANALOG_PIN 36

#define SEALEVELPRESSURE_HPA (1013.25)

// Constants
const int MORNING_WATERING_START = 6;
const int MORNING_WATERING_END = 8;
const int EVENING_WATERING_START = 19;
const int EVENING_WATERING_END = 22;

// REST APIs URLs
const char* weatherApi= ""; // still searching
const char* timeApi = "http://worldtimeapi.org/api/timezone/Europe/Sofia";

// WiFi configurations and variables
//const char* ssid = NETWORK_SECRET_SSID;
//const char* password = NETWORK_SECRET_PASS;
const char* ssid = "A1_C5B4";
const char* password = "48575443B036A4A4";

// Soil Moisture Sensor
const int AIR_VALUE = 3620;
const int WATER_VALUE = 1680;
const int TOMATO_SOIL_MOISTURE_MIN = 70;
const int TOMATO_SOIL_MOISTURE_MAX = 90;

// Global variables
int soilMoistureValue = 0;
int soilmoisturePercent = 0;

// Plant config
String plantName = "tomato";
int plantSoilMoistureMin = 70;
int plantSoilMoistureMax = 90;

// Web Server
WebServer server(80);

// HTTP Clien
HTTPClient http;

// JSON Parser
StaticJsonBuffer<200> jsonBuffer;

// BME Connected via SPI Protocol
Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);

  //Set pins mode
  pinMode(LED_PIN_RED, OUTPUT);
  pinMode(LED_PIN_GREEN, OUTPUT);
  pinMode(LED_PIN_YELLOW, OUTPUT);

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
  connectBME();
}

void loop() {
  // Print metrisc from BME 280

  // uncoment for debug
//  Serial.println("Esp in loop func");
//  delay(1000); 
//  printToSerial();
//  printSoilMoisturePercentToSerial();

  
  server.handleClient();
  checkForWatering();
}

//void printSoilMoisturePercentToSerial() {
//  Serial.print("Soil Moisture Percent: ");
//  Serial.print(getSoilMoisturePercent());
//  Serial.println("%");
//}

int getSoilMoisturePercent() {
  return 80;
  int soilMoistureValue = analogRead(SOIL_MOISTURE_ANALOG_PIN);
  int soilMoisturePercent = map(soilMoistureValue, AIR_VALUE, WATER_VALUE, 0, 100);

  if (soilMoisturePercent > 100) {
    return 100;
  }

  if (soilMoisturePercent < 0) {
    return 0;
  }

  return soilMoisturePercent;
}

void lightsOn() {
  digitalWrite(LED_PIN_YELLOW, LOW);
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

void connectBME() {
  bool status;
  status = bme.begin();  
  if (!status) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
  } else {
    Serial.println("Connect to BME280 sensor successfully.");
  }
  // some time for BME280 to boot up
  delay(1000);
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
    Serial.println("[Web Server]: handle GET /test request");
    Serial.println();
  
    digitalWrite(LED_PIN_YELLOW, HIGH);
    server.send(200, "text/plain", "Web Server works well");
    digitalWrite(LED_PIN_YELLOW, LOW);
  });

  server.on("/temp", returnTemp);

  server.on("/hum", returnHum);

  server.on("/alt", returnAlt);

  server.on("/soilMoisture", returnSoilMoisture);

  server.on("/metrics", returnMetrics);

  server.on("/plant", returnPlant);

  server.on("/setPlant", setPlant);

  server.onNotFound(handleNotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  Serial.println("[Web Server]: handle GET / request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "This is IDI System Web Server!\n";
  response += "From here you can check all metrics of the system.";
  response += "You can also add new plants to the system.";
  response += "You can call these endpoints: \n";
  response += "GET /temp  - get air temperature \n";
  response += "GET /hum  - get air humidity \n";
  response += "GET /alt  - get approx. altitude \n";
  response += "GET /soilMoisture  - get soilMoisture \n";
  response += "GET /metrics  - get metrics from all sensors \n";
  response += "GET /plant  - get plant configuration of the system \n";
  response += "POST /setPlant - upsert plant configuration of the system \n";
//  response += "/addPlant  - add new plant for monitoring \n\n";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnTemp() {
  Serial.println("[Web Server]: handle GET Air Temperature request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Air Temperature: ";
  response += bme.readTemperature();
  response += " *C";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnHum() {
  Serial.println("[Web Server]: handle GET Air Humidity request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Air Humidity: ";
  response += bme.readHumidity();
  response += " %";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnAlt() {
  Serial.println("[Web Server]: handle GET Approx. Altitude request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Approx. Altitude = ";
  response += bme.readAltitude(SEALEVELPRESSURE_HPA);
  response += " m";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnSoilMoisture() {
  Serial.println("[Web Server]: handle GET Soil Moisture request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Soil Moisture: ";
//  response += getSoilMoisturePercent();
  response += " %";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnMetrics() {
  Serial.println("[Web Server]: handle GET /metrics request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Air Temperature: ";
  response += bme.readTemperature();
  response += " *C\n";

  response += "Air Humidity: ";
  response += bme.readHumidity();
  response += " %\n";

  response += "Approx. Altitude = ";
  response += bme.readAltitude(SEALEVELPRESSURE_HPA);
  response += " m\n";

  response += "Pressure = ";
  response += bme.readPressure() / 100.0F;
  response += " hPa\n";

  response += "Soil Moisture: ";
//  response += getSoilMoisturePercent();
  response += " %\n\n";

  server.send(200, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void returnPlant() {
  Serial.println("[Web Server]: handle GET /plant request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = jsonPlant();

  server.send(200, "text/json", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

// TODO: plant should be updated every time
void setPlant() {
  Serial.println("[Web Server]: handle POST /plant request");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String bodyRequest = server.arg(0);
  JsonObject& parsed = jsonBuffer.parseObject(bodyRequest); //Parse message
  String newPlantName = parsed["plantName"];
  int newMin = parsed["minPlantSoilMoisture"];
  int newMax = parsed["maxPlantSoilMoisture"];
  jsonBuffer.clear(); // clear buffer so it can be used again later
  
  plantSoilMoistureMin = newMin;
  plantSoilMoistureMax = newMax;
  
  if (newPlantName.equals(plantName)) {
    Serial.print("[Web Server]: Plant ");
    Serial.print(plantName);
    Serial.println(" was updated successfully!");

    server.send(200, "text/json", jsonPlant());
  } else {
    String oldName = plantName;
    plantName = newPlantName;
    Serial.print("[Web Server]: Plant ");
    Serial.print(oldName);
    Serial.print(" was changed successfully to ");
    Serial.println(plantName);

    server.send(200, "text/json", jsonPlant());
  }
  digitalWrite(LED_PIN_YELLOW, LOW);
}

// TODO
void addPlant() {
  digitalWrite(LED_PIN_YELLOW, HIGH);
  
  digitalWrite(LED_PIN_YELLOW, LOW);
}

void handleNotFound() {
  Serial.println("[Web Server]: handle not found..");
  Serial.println();
  
  digitalWrite(LED_PIN_YELLOW, HIGH);
  String response = "Bad request\n\n";
  response += "To view available ednpoints call /";
  
  server.send(400, "text/plain", response);
  digitalWrite(LED_PIN_YELLOW, LOW);
}

String doGetRequest(const char* apiUrl) {
  http.begin(apiUrl);
  
  Serial.print("[HTTP]: Get Request to: ");
  Serial.println(apiUrl);

  int httpResponseCode = http.GET();
  if (httpResponseCode == 200) {
    return http.getString();
  } else {
    Serial.print("Error sending the Get Request to ");
    Serial.println(apiUrl);
    Serial.println(httpResponseCode);
    Serial.println();

    return "";
  }
  http.end();
}

String jsonPlant() { 
  String json = "{";
  json += "\"plantName\": \"" + plantName + "\",";
  json += "\"minPlantSoilMoisture\": " + String(plantSoilMoistureMin) + ",";
  json += "\"maxPlantSoilMoisture\": " + String(plantSoilMoistureMax);
  json += "}";

  return json;
}

void checkForWatering() {
  int currentHour = getCurrentHour();
  Serial.print("Current hour is: ");
  Serial.println(currentHour);
  
  // if in irrigation hours - irrigate
  Serial.println("Check time..."); 
  boolean isMorningWateringTime = currentHour > MORNING_WATERING_START && currentHour < MORNING_WATERING_END;
  boolean isEveningWateringTime = currentHour > EVENING_WATERING_START && currentHour < EVENING_WATERING_END;
  if (isMorningWateringTime || isEveningWateringTime) {
    Serial.println("Check Soil Moisture...");
    irrigate();
  } else {
    Serial.println("Now is not an irrigation time");
  }
}

int getCurrentHour() {
  String apiResponse = doGetRequest(timeApi);
  if (apiResponse.equals("")) {
    // if rest request fails check for watering
    Serial.println("Error with get request to the weather api");
    return MORNING_WATERING_START + 1;
  }
  
  String datetimeField = apiResponse.substring(apiResponse.indexOf("\"datetime\""));
  int hour = datetimeField.substring(23, 25).toInt();
  return hour;
}

void irrigate() {
  int currentSoilMoisture = getSoilMoisturePercent();
  if (currentSoilMoisture < plantSoilMoistureMin) {
    Serial.println("Start irrigation proces");
    ValveOn();
    
    while(currentSoilMoisture < plantSoilMoistureMax) {
      currentSoilMoisture = getSoilMoisturePercent();
    }
    
    ValveOff();
  }  else {
    Serial.println("Irrigation not needed");
  }
}

void ValveOn() {
  digitalWrite(VALVE_PIN, HIGH);
  Serial.println("Valve ON !!!");
}

void ValveOff() {
  digitalWrite(VALVE_PIN, LOW);
  Serial.println("Valve OFF !!!");
}

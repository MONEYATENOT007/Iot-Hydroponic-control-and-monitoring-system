#define BLYNK_TEMPLATE_ID "TMPL3P39Pw6-9"
#define BLYNK_TEMPLATE_NAME "iothydroponic"
#define BLYNK_AUTH_TOKEN "UG801njEx9TTfkAIVVcG58TMwk7TVuOO"

#define BLYNK_PRINT Serial

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// Replace with your WiFi credentials
char ssid[] = "esp8266hydro";
char pass[] = "12345678";

// Define the pins
#define DHTPIN D4               // DHT11 data pin
#define SOIL_MOISTURE_PIN A0    // Soil moisture sensor analog pin
#define RELAY1_PIN D5           // Relay 1 control pin
#define RELAY2_PIN D6           // Relay 2 control pin
#define RELAY3_PIN D0           // New Relay control pin for soil moisture condition
#define TRIG_PIN D3             // Ultrasonic sensor TRIG pin
#define ECHO_PIN D7             // Ultrasonic sensor ECHO pin

// DHT settings
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// LCD settings
LiquidCrystal_I2C lcd(0x27, 16, 2); // Adjust the address if necessary

BlynkTimer timer;

// Relay control states
bool relay1Override = false;
bool relay2Override = false;
bool relay3Override = false;
bool relay1State = LOW;
bool relay2State = LOW;
bool relay3State = LOW;

#define TANK_HEIGHT 1300  // Example height of the tank in cm

long measureWaterLevel() {
  // Send a pulse to TRIG_PIN
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // Measure the response time from ECHO_PIN
  long duration = pulseIn(ECHO_PIN, HIGH);

  // Calculate the distance in cm
  long distance = duration * 4/2 ;

  // Calculate the water level
  long waterLevel1 = (TANK_HEIGHT - distance)/10;
  Serial.print(", distance: ");
  Serial.println(distance);

  // Ensure the water level is within a valid range
  if (waterLevel1 < 20) {
    waterLevel1 = 0;
  } else if (waterLevel1 > 90) {
    waterLevel1 = 100;
  }

  return waterLevel1;
}


// Function to send sensor data to Blynk app and display on LCD
void sendSensorData() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int soilMoistureValue = analogRead(SOIL_MOISTURE_PIN);
  soilMoistureValue = soilMoistureValue/10;
  long waterLevel = measureWaterLevel(); // Measure water level

  if (isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }

  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.print("%, Temperature: ");
  Serial.print(temperature);
  Serial.print("°C, Soil Moisture: ");
  Serial.print(soilMoistureValue);
  Serial.print(", Water Level: ");
  Serial.println(waterLevel);
  

  Blynk.virtualWrite(V1, humidity);           // Sending humidity to Blynk virtual pin V5
  Blynk.virtualWrite(V0, temperature);        // Sending temperature to Blynk virtual pin V6
  Blynk.virtualWrite(V3, soilMoistureValue);  // Sending soil moisture to Blynk virtual pin V7
  Blynk.virtualWrite(V2, waterLevel);         // Sending water level to Blynk virtual pin V8

  lcd.clear(); // Clear the display before writing new values
  lcd.setCursor(0, 0);
  lcd.print("T: ");
  lcd.print(temperature);
  lcd.print(" C");

  lcd.setCursor(8, 0);
  lcd.print("H: ");
  lcd.print(humidity);
  lcd.print(" %");

  lcd.setCursor(0, 1);
  lcd.print("W: ");
  lcd.print(waterLevel);
  lcd.print(" %");

  // Always active automatic control for RELAY1 based on temperature
  if (temperature < 25) {
    digitalWrite(RELAY1_PIN, LOW);
    relay1State = LOW;
  } else if (temperature > 40) {
    digitalWrite(RELAY1_PIN, HIGH);
    relay1State = HIGH;
  }

  // Always active automatic control for RELAY2 based on humidity
  if (humidity > 80) {
    digitalWrite(RELAY2_PIN, HIGH);
    relay2State = HIGH;
  } else if (humidity < 70) {
    digitalWrite(RELAY2_PIN, LOW);
    relay2State = LOW;
  }

  // Always active automatic control for RELAY3 based on soil moisture and water level
  if (waterLevel > 0) { // Adjust threshold as needed
    if (soilMoistureValue < 10 ) { // Adjust soil moisture threshold as needed
      digitalWrite(RELAY3_PIN, LOW);
      relay3State = LOW;
    } else if(soilMoistureValue > 50){
      digitalWrite(RELAY3_PIN, HIGH);
      relay3State = HIGH;
    }
  }else if(waterLevel == 0)
  {
    digitalWrite(RELAY3_PIN, HIGH);
    relay3State = HIGH;
  } 
}

// Function to control relay 1 via Blynk switch
BLYNK_WRITE(V4) {
  relay1State = param.asInt();
  relay1Override = true;  // Indicate manual override
  digitalWrite(RELAY1_PIN, relay1State);
}

// Function to control relay 2 via Blynk switch
BLYNK_WRITE(V5) {
  relay2State = param.asInt();
  relay2Override = true;  // Indicate manual override
  digitalWrite(RELAY2_PIN, relay2State);
}

// Function to control relay 3 via Blynk switch
BLYNK_WRITE(V6) {
  relay3State = param.asInt();
  relay3Override = true;  // Indicate manual override
  digitalWrite(RELAY3_PIN, relay3State);
}

void setup() {
  // Debug console
  Serial.begin(9600);

  // Blynk setup
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

  // DHT sensor setup
  dht.begin();

  // I2C setup
  Wire.begin(D2, D1); // SDA, SCL
  lcd.begin(); // Initialize the LCD
  lcd.backlight();

  // Relay and ultrasonic sensor pins setup
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(RELAY3_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(RELAY1_PIN, relay1State);  // Initial state of relay 1
  digitalWrite(RELAY2_PIN, relay2State);  // Initial state of relay 2
  digitalWrite(RELAY3_PIN, relay3State);  // Initial state of relay 3

  // Timer setup to send sensor data every 2 seconds
  timer.setInterval(2000L, sendSensorData);
}

void loop() {
  Blynk.run();
  timer.run();
}

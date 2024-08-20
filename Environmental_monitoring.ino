#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "FS.h"
#include "SPIFFS.h"
#include <HTTPClient.h>  // Import the HTTPClient library

const char* ssid = "李";
const char* password = "12345678";
const char* serverUrl = "http://192.168.110.21:8080/data/data";  // Replace <your-server-ip> with your server address

#define DHTPIN 17
#define DHTTYPE DHT11
#define MAX_SAMPLES 10

DHT dht(DHTPIN, DHTTYPE);
AsyncWebServer server(80);

float temperatureSamples[MAX_SAMPLES];
float humiditySamples[MAX_SAMPLES];
int sampleIndex = 0;
bool samplesFilled = false;

const int ledPin = 13;    // Onboard LED pins
const int soundPin = A4;  // The sound sensor is connected to A4
const int avoidPin = 15;  // The infrared obstacle sensor is connected to pin 15

void saveData(String data) {
  File file = SPIFFS.open("/data.txt", FILE_APPEND);
  if(!file){
    Serial.println("Failed to open file for appending");
    return;
  }
  file.println(data);
  file.close();
}

void readData() {
  File file = SPIFFS.open("/data.txt", FILE_READ);
  if(!file){
    Serial.println("Failed to open file for reading");
    return;
  }
  while(file.available()){
    Serial.println(file.readStringUntil('\n'));
  }
  file.close();
}

void sendDataToServer(String temperature, String humidity, String avgTemperature, String avgHumidity, String soundLevel, String obstacleDetection) {
  HTTPClient http;
  http.begin(serverUrl); // Change the URL to your server address
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  String postData = "temperature=" + temperature + "&humidity=" + humidity +
                    "&avgTemperature=" + avgTemperature + "&avgHumidity=" + avgHumidity +
                    "&soundLevel=" + soundLevel + "&obstacleDetection=" + obstacleDetection;

  int httpResponseCode = http.POST(postData);

  if (httpResponseCode > 0) {
    String response = http.getString(); // Returns the response from the server
    Serial.println(httpResponseCode);   // Print out HTTP response codes
    Serial.println(response);           // Output the data returned by the server
  } else {
    Serial.print("Error on sending POST: ");
    Serial.println(httpResponseCode);
  }

  http.end(); // Close HTTP Connection
}

String readDHTTemperature() {
  float t = dht.readTemperature();
  if (isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  } else {
    temperatureSamples[sampleIndex] = t;
    Serial.println(t);
    saveData("Temperature: " + String(t));
    return String(t);
  }
}

String readDHTHumidity() {
  float h = dht.readHumidity();
  if (isnan(h)) {
    Serial.println("Failed to read from DHT sensor!");
    return "--";
  } else {
    humiditySamples[sampleIndex] = h;
    Serial.println(h);
    saveData("Humidity: " + String(h));
    return String(h);
  }
}

String readSoundSensor() {
  int value = analogRead(soundPin); // Read the sound sensor values
  Serial.println(value);            // Print it out
  saveData("Sound Level: " + String(value));
  if(value > 600)                   // If the sound sensor value is greater than 600
  {
    digitalWrite(ledPin,HIGH);     // Turn on the led lights
  }
  else                             
  {
    digitalWrite(ledPin,LOW);      // Turn off the led lights
  }
  return String(value);
}

String readAvoidSensor() {
  boolean avoidVal = digitalRead(avoidPin);  // Read the value of avoidPin
  if (avoidVal == LOW) // If it is LOW level
  {
    digitalWrite(ledPin, HIGH);  // Turn on the led lights
    saveData("Obstacle detected");
    return "Obstacle detected";
  }
  else
  {
    digitalWrite(ledPin, LOW);  // Turn off the led lights
    saveData("No obstacle");
    return "No obstacle";
  }
}

float calculateAverage(float samples[], int count) {
  float sum = 0.0;
  for (int i = 0; i < count; i++) {
    sum += samples[i];
  }
  return sum / count;
}

String getAverageTemperature() {
  if (!samplesFilled && sampleIndex < MAX_SAMPLES) return "--";
  return String(calculateAverage(temperatureSamples, MAX_SAMPLES));
}

String getAverageHumidity() {
  if (!samplesFilled && sampleIndex < MAX_SAMPLES) return "--";
  return String(calculateAverage(humiditySamples, MAX_SAMPLES));
}

String processor(const String& var){
  if (var == "TEMPERATURE") {
    return readDHTTemperature();
  }
  else if (var == "HUMIDITY") {
    return readDHTHumidity();
  }
  else if (var == "AVGTEMPERATURE") {
    return getAverageTemperature();
  }
  else if (var == "AVGHUMIDITY") {
    return getAverageHumidity();
  }
  else if (var == "SOUND") {
    return readSoundSensor();
  }
  else if (var == "AVOID") {
    return readAvoidSensor();
  }
  return String("N/A");
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html { font-family: Arial; display: inline-block; margin: 0px auto; text-align: center; }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels { font-size: 1.5rem; vertical-align: middle; padding-bottom: 15px; }
  </style>
</head>
<body>
  <h2>ESP32 Sensor Server</h2>
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
  <p>
    <i class="fas fa-thermometer-three-quarters" style="color:#f0ad4e;"></i> 
    <span class="dht-labels">Average Temperature</span> 
    <span id="avgTemperature">%AVGTEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#5bc0de;"></i> 
    <span class="dht-labels">Average Humidity</span>
    <span id="avgHumidity">%AVGHUMIDITY%</span>
    <sup class="units">&percnt;</sup>
  </p>
  <p>
    <i class="fas fa-microphone" style="color:#e74c3c;"></i> 
    <span class="dht-labels">Sound Level</span>
    <span id="sound">%SOUND%</span>
    <sup class="units">units</sup>
  </p>
  <p>
    <i class="fas fa-exclamation-triangle" style="color:#ff0000;"></i> 
    <span class="dht-labels">Obstacle Detection</span>
    <span id="avoid">%AVOID%</span>
  </p>
</body>
<script>
setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 5000); // Updates every 5 seconds

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 5000); //It is updated every 5 seconds

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("avgTemperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/avgTemperature", true);
  xhttp.send();
}, 5000); // It is updated every 5 seconds

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("avgHumidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/avgHumidity", true);
  xhttp.send();
}, 5000); //It is updated every 5 seconds

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("sound").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/sound", true);
  xhttp.send();
}, 5000); //It is updated every 5 seconds

setInterval(function() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("avoid").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/avoid", true);
  xhttp.send();
}, 5000); // It is updated every 5 seconds
</script>
</html>)rawliteral";

void setup() {
  Serial.begin(115200);
  dht.begin();
  
  // 初始化SPIFFS
  if(!SPIFFS.begin(true)){
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  Serial.println("SPIFFS mounted successfully");

  WiFi.begin(ssid, password);

  pinMode(ledPin, OUTPUT);  // Set ledPin to OUTPUT
  pinMode(avoidPin, INPUT); // Set avoidPin to INPUT
  analogSetWidth(10);       // Set the sampling resolution to 10bit and the range can be between 9-12
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/temperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTTemperature().c_str());
  });
  server.on("/humidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readDHTHumidity().c_str());
  });
  server.on("/avgTemperature", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", getAverageTemperature().c_str());
  });
  server.on("/avgHumidity", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", getAverageHumidity().c_str());
  });
  server.on("/sound", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readSoundSensor().c_str());
  });
  server.on("/avoid", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", readAvoidSensor().c_str());
  });

  server.begin();
}

void loop() {
  // Update the sample index for averaging
  sampleIndex = (sampleIndex + 1) % MAX_SAMPLES;
  if (sampleIndex == 0 && !samplesFilled) {
    samplesFilled = true; // Now we have enough samples to compute averages
  }

  //Data is sent to the server after each data read
  String temperature = readDHTTemperature();
  String humidity = readDHTHumidity();
  String avgTemperature = getAverageTemperature();
  String avgHumidity = getAverageHumidity();
  String soundLevel = readSoundSensor();
  String obstacleDetection = readAvoidSensor();

  // Periodically yield to reset the watchdog timer
  yield();

  sendDataToServer(temperature, humidity, avgTemperature, avgHumidity, soundLevel, obstacleDetection);

  // Check WiFi connection status
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi lost connection. Reconnecting...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
      delay(1000);
      Serial.println("Reconnecting to WiFi...");
    }
    Serial.println("Reconnected to WiFi.");
  }

  // Yield again before the delay
  yield();

  delay(10000); // A 10-second delay prevents sending data too often
}

#include <DHT.h>
#include <WiFi.h>
#include <PubSubClient.h>

#define WIFI_AP "Wokwi-GUEST"
#define WIFI_PASS ""

#define TB_SERVER "thingsboard.cloud"
#define TB_PORT 1883
#define TB_TOKEN "SCHqIZ8eFaYT8KXubWkX"

#define DHTPIN 4
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void connectToWifi() {
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_AP, WIFI_PASS);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi!");
  }
}

void connectToThingsboard() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to ThingsBoard... ");
    if (mqttClient.connect("ESP32_Client", TB_TOKEN, "")) {
      Serial.println("Connected to ThingsBoard!");
    } else {
      Serial.print("Failed, rc=");
      Serial.print(mqttClient.state());
      Serial.println(" retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("Starting DHT22 Sensor...");
  dht.begin();

  connectToWifi();
  
  mqttClient.setServer(TB_SERVER, TB_PORT);
  connectToThingsboard(); // Connect to ThingsBoard after WiFi
}

void sendTelemetry(float temperature, float humidity) {
  if (!mqttClient.connected()) {
    connectToThingsboard();
  }
  
  mqttClient.loop();

  // Prepare JSON payload
  String payload = "{\"temperature\":";
  payload += temperature;
  payload += ",\"humidity\":";
  payload += humidity;
  payload += "}";

  // Publish telemetry to ThingsBoard
  if (mqttClient.publish("v1/devices/me/telemetry", payload.c_str())) {
    Serial.println("Telemetry sent: " + payload);
  } else {
    Serial.println("Failed to send telemetry");
  }
}

void loop() {
  // Reconnect to WiFi if necessary
  if (WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }

  // Read sensor data
  float temperature = dht.readTemperature();
  float humidity = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("Failed to read from DHT sensor!");
  } else {
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println("°C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println("%");

    // Send data to ThingsBoard
    sendTelemetry(temperature, humidity);
  }

  delay(3000);
}

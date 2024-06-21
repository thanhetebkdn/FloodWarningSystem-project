#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <HardwareSerial.h>

const char* ssid = "Bao1";
const char* password = "123456789";
const char* serverUrl = "https://rtos-server.onrender.com/api/sensors";

HardwareSerial MySerial(2);
String accumulatedData; // Global variable to accumulate sensor data

QueueHandle_t sensorDataQueue; // Queue to hold sensor data

void sendDataTask(void * parameter) {
  for (;;) {
    String dataToSend;

    if (xQueueReceive(sensorDataQueue, &dataToSend, portMAX_DELAY)) {
      Serial.println("Received complete data: " + dataToSend);

      // Send HTTP POST request to server
      HTTPClient http;
      http.begin(serverUrl);
      http.addHeader("Content-Type", "application/json");

      // Prepare JSON payload
      String jsonString = "{\"data\": \"" + dataToSend + "\"}";

      int httpResponseCode = http.POST(jsonString);

      if (httpResponseCode > 0) {
        Serial.printf("HTTP POST request to server successful, response code: %d\n", httpResponseCode);
        String payload = http.getString();
        Serial.println("Response payload: " + payload);
      } else {
        Serial.printf("HTTP POST request failed, error code: %d\n", httpResponseCode);
      }

      http.end();
      
    }
  }
}

void setup() {
  Serial.begin(115200);
  MySerial.begin(115200, SERIAL_8N1, 16, 17);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize the queue for sensor data
  sensorDataQueue = xQueueCreate(10, sizeof(String));

  // Create FreeRTOS task to send data
  xTaskCreate(
    sendDataTask,         // Task function
    "SendDataTask",       // Task name
    8192,                 // Stack size (bytes)
    NULL,                 // Task input parameters
    1,                    // Priority (1 is lower priority, 3 is higher)
    NULL                  // Task handle
  );
}

void loop() {
  static String distanceValue = "", rainValue = "", tempValue = "", humiValue = "";

  while (MySerial.available()) {
    String receivedData = MySerial.readStringUntil('\n');
    Serial.println("Received data: " + receivedData);

    int colonIndex = receivedData.indexOf(':');
    if (colonIndex != -1) {
      String sensorType = receivedData.substring(0, colonIndex);
      String sensorValue = receivedData.substring(colonIndex + 1);

      if (sensorType.equals("Distance")) {
        distanceValue = sensorValue;
      } else if (sensorType.equals("Rain")) {
        rainValue = sensorValue;
      } else if (sensorType.equals("Temp")) {
        tempValue = sensorValue;
      } else if (sensorType.equals("Humi")) {
        humiValue = sensorValue;
      }
    }
  }

  // Check if all values are available
  if (!distanceValue.isEmpty() && !rainValue.isEmpty() &&
      !tempValue.isEmpty() && !humiValue.isEmpty()) {
    // Construct the data string
    accumulatedData = distanceValue + "-" + rainValue + "-" + tempValue + "-" + humiValue;
    
    // Send accumulated data to the queue
    xQueueSend(sensorDataQueue, &accumulatedData, portMAX_DELAY);

    // Clear values for next batch
    distanceValue = "";
    rainValue = "";
    tempValue = "";
    humiValue = "";
  }

  delay(100); // Add a small delay to avoid excessive CPU usage
}

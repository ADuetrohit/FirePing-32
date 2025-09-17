#include <WiFi.h>
#include <HTTPClient.h>

// WiFi credentials
const char* ssid = "Galaxy";
const char* password = "123456789";

// Telegram Bot
String botToken = "8011062444:AAG9kEs-lrM7A6zBNbP37exvWLpA7TTVCsw";
String chatID = "1677381765"; // Your Telegram chat ID

WiFiServer server(80);

#define FIRE_SENSOR_PIN 34
#define BUZZER_PIN 13

bool fireNotified = false; // Prevent repeated Telegram messages

void setup() {
  Serial.begin(115200);

  // Pins
  pinMode(FIRE_SENSOR_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  // Connect WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\n✅ Connected!");
  Serial.print("ESP32 IP: "); Serial.println(WiFi.localIP());

  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  // Read fire sensor
  int fireState = digitalRead(FIRE_SENSOR_PIN); // LOW = fire detected
  digitalWrite(BUZZER_PIN, (fireState == LOW) ? HIGH : LOW);

  // Telegram notification (once per fire event)
  if (fireState == LOW && !fireNotified) {
    sendTelegramMessage("🔥 FIRE DETECTED! Take immediate action!");
    fireNotified = true;
  } else if (fireState == HIGH) {
    fireNotified = false;
  }

  // Web interface
  if (client) {
    String currentLine = "";
    bool requestEnded = false;
    while (client.connected() && !requestEnded) {
      if (client.available()) {
        char c = client.read();
        currentLine += c;
        if (currentLine.endsWith("\r\n\r\n")) requestEnded = true;
      }
    }

    // Build professional HTML page
    String html = "<!DOCTYPE html><html lang='en'>"
                  "<head>"
                  "<meta charset='UTF-8'>"
                  "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
                  "<meta http-equiv='refresh' content='2'>"
                  "<title>ESP32 Fire Alarm</title>"
                  "<style>"
                  "body { margin:0; font-family: Arial; background: #1a1a1a; color:#fff; text-align:center; }"
                  ".container { display:flex; flex-direction:column; justify-content:center; align-items:center; min-height:100vh; }"
                  "h1 { font-size:2.5em; color:#ff6b00; margin-bottom:20px; }"
                  ".status { font-size:1.8em; margin:20px; padding:20px; border-radius:15px; width:80%; max-width:400px; }"
                  ".fire { background:red; color:white; box-shadow:0 0 20px red; animation: pulse 1s infinite; }"
                  ".safe { background:green; color:white; box-shadow:0 0 20px green; }"
                  "@keyframes pulse { 0% { transform:scale(1); } 50% { transform:scale(1.05); } 100% { transform:scale(1); } }"
                  ".buzzer { font-size:1.2em; margin-top:10px; }"
                  "</style>"
                  "</head>"
                  "<body>"
                  "<div class='container'>"
                  "<h1>🔥 ESP32 Fire Alarm 🔥</h1>"
                  "<div class='status " + String((fireState == LOW) ? "fire" : "safe") + "'>"
                  + String((fireState == LOW) ? "🔥 FIRE DETECTED!" : "✅ Safe - No Fire") +
                  "</div>"
                  "<div class='buzzer'>Buzzer: " + String((fireState == LOW) ? "ON" : "OFF") + "</div>"
                  "</div>"
                  "</body></html>";

    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");
    client.print("Content-Length: "); client.println(html.length());
    client.println();
    client.print(html);

    client.stop();
  }
}

// Function to send Telegram message
void sendTelegramMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatID + "&text=" + message;
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
      Serial.println("Telegram message sent!");
    } else {
      Serial.println("Error sending Telegram message");
    }
    http.end();
  }
}

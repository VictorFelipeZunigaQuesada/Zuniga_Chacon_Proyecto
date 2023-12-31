#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_camera.h"
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>
#include <Wire.h>
//#include "SparkFunBME280.h"
 
const char* ServerName = "esp32cam"; // Address to the server with http://esp32cam.local/
 
String local_hwaddr; // WiFi local hardware Address
String local_swaddr; // WiFi local software Address
// Replace with your network credentials
const char* ssid = "NETLIFE-CHACON";
const char* password = "Coco1972**";
// Initialize Telegram BOT
// My_ESP_camBot
String chatId = "1948780014"; // User ID
String BOTtoken = "6151782844:AAE02nVoN8xDgJ45cZ9ImPnh2ey4iNUWFcg";
 
bool sendPhoto = false;
 
WiFiClientSecure clientTCP;
 
UniversalTelegramBot bot(BOTtoken, clientTCP);
 
//CAMERA_MODEL_AI_THINKER
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
 
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
 


// Motion Sensor AM312 PIR sensor or XYC-WB-DC radar sensor
#define MOTION_SENSOR 13 // MOTION_SENSOR PIN: GPIO 13
 
//bool FlashState = false;
bool MotionDetected = false;
bool MotionState = false;

 
int botRequestDelay = 1000; // mean time between scan messages
long lastTimeBotRan; // last time messages’ scan has been done
 
void handleNewMessages(int numNewMessages);
String sendPhotoTelegram();


 
void setup() {
WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
Serial.begin(115200);
delay(500);
 
pinMode(MOTION_SENSOR, INPUT_PULLUP); // MOTION_SENSOR
 
delay(500);
Serial.println("\nESP Cam using Telegram Bot");
//String readings = getReadings();
//Serial.print(readings);
 
WiFi.mode(WIFI_STA);
Serial.println();
Serial.print("Connecting to ");
Serial.println(ssid);
WiFi.begin(ssid, password);
 
// ADDED This Update
clientTCP.setCACert(TELEGRAM_CERTIFICATE_ROOT); // Add root certificate for api.telegram.org
 
while (WiFi.status() != WL_CONNECTED) {
Serial.print(".");
delay(500);
}
Serial.println(" >> CONNECTED");
 
Serial.print("ESP32-CAM IP Address: ");
Serial.println(WiFi.localIP());
 
// Print the Signal Strength:
long rssi = WiFi.RSSI() + 100;
Serial.print("Signal Strength = " + String(rssi));
if (rssi > 50) {
Serial.println(F(" (>50 – Good)"));
} else {
Serial.println(F(" (Could be Better)"));
}

camera_config_t config;
config.ledc_channel = LEDC_CHANNEL_0;
config.ledc_timer = LEDC_TIMER_0;
config.pin_d0 = Y2_GPIO_NUM;
config.pin_d1 = Y3_GPIO_NUM;
config.pin_d2 = Y4_GPIO_NUM;
config.pin_d3 = Y5_GPIO_NUM;
config.pin_d4 = Y6_GPIO_NUM;
config.pin_d5 = Y7_GPIO_NUM;
config.pin_d6 = Y8_GPIO_NUM;
config.pin_d7 = Y9_GPIO_NUM;
config.pin_xclk = XCLK_GPIO_NUM;
config.pin_pclk = PCLK_GPIO_NUM;
config.pin_vsync = VSYNC_GPIO_NUM;
config.pin_href = HREF_GPIO_NUM;
config.pin_sscb_sda = SIOD_GPIO_NUM;
config.pin_sscb_scl = SIOC_GPIO_NUM;
config.pin_pwdn = PWDN_GPIO_NUM;
config.pin_reset = RESET_GPIO_NUM;
config.xclk_freq_hz = 20000000;
config.pixel_format = PIXFORMAT_JPEG;
 
//init with high specs to pre-allocate larger buffers
if (psramFound()) {
config.frame_size = FRAMESIZE_UXGA;
config.jpeg_quality = 10; //0-63 lower number means higher quality
config.fb_count = 2;
} else {
config.frame_size = FRAMESIZE_SVGA;
config.jpeg_quality = 12; //0-63 lower number means higher quality
config.fb_count = 1;
}
 
// camera init
esp_err_t err = esp_camera_init(&config);
if (err != ESP_OK) {
Serial.printf("Camera init failed with error 0x%x", err);
delay(1000);
ESP.restart();
}
Serial.printf("Camera Initialized >> OK \r\n");
 
// Drop down frame size for higher initial frame rate
sensor_t * s = esp_camera_sensor_get();
s->set_framesize(s, FRAMESIZE_CIF); // UXGA|SXGA|XGA|SVGA|VGA|CIF|QVGA|HQVGA|QQVGA
 
Serial.printf("Digite /Start para iniciar o Bot no Telegram\r\n");
 
}

// Bucle 

void loop() {
if (sendPhoto) {
 
Serial.println("Preparing photo");
sendPhotoTelegram();
sendPhoto = false;
delay(1000);
}
  
if (MotionState) {

//pinMode(MOTION_SENSOR, INPUT_PULLUP); // MOTION_SENSOR
int MovimientoDetectado= digitalRead(MOTION_SENSOR);
Serial.println(MovimientoDetectado);
if(MovimientoDetectado==1){
  bot.sendMessage(chatId, "Movimento detectado!", "");
  Serial.println("Movimento detectado!");
  sendPhotoTelegram();
  delay(2000);
}

}
 
if (millis() > lastTimeBotRan + botRequestDelay) {
int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
while (numNewMessages) {
Serial.print("Message received : ");
handleNewMessages(numNewMessages);
numNewMessages = bot.getUpdates(bot.last_message_received + 1);
}
lastTimeBotRan = millis();
}
}
 
String sendPhotoTelegram() {
const char* myDomain = "api.telegram.org";
String getAll = "";
String getBody = "";
 
camera_fb_t * fb = NULL;
 
//if (FlashState == true) digitalWrite(FLASH_LED_PIN, HIGH); // FLASH ON
 
delay(10);
 
fb = esp_camera_fb_get();
 
//digitalWrite(FLASH_LED_PIN, LOW); // FLASH OFF
 
if (!fb) {
Serial.println("Camera capture failed");
delay(1000);
ESP.restart();
return "Camera capture failed";
}
 
Serial.println("Connect to " + String(myDomain));
 
if (clientTCP.connect(myDomain, 443)) {
Serial.println("Connection successful");
 
String head = "--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"chat_id\"; \r\n\r\n" + chatId + "\r\n--RandomNerdTutorials\r\nContent-Disposition: form-data; name=\"photo\"; filename=\"esp32-cam.jpg\"\r\nContent-Type: image/jpeg\r\n\r\n";
String tail = "\r\n--RandomNerdTutorials--\r\n";
 
uint16_t imageLen = fb->len;
uint16_t extraLen = head.length() + tail.length();
uint16_t totalLen = imageLen + extraLen;
 
clientTCP.println("POST /bot" + BOTtoken + "/sendPhoto HTTP/1.1");
clientTCP.println("Host: " + String(myDomain));
clientTCP.println("Content-Length: " + String(totalLen));
clientTCP.println("Content-Type: multipart/form-data; boundary=RandomNerdTutorials");
clientTCP.println();
clientTCP.print(head);
 
uint8_t *fbBuf = fb->buf;
size_t fbLen = fb->len;
for (size_t n = 0; n < fbLen; n = n + 1024) {
if (n + 1024 < fbLen) {
clientTCP.write(fbBuf, 1024);
fbBuf += 1024;
}
else if
(fbLen % 1024 > 0) {
size_t remainder = fbLen % 1024;
clientTCP.write(fbBuf, remainder);
}
}
 
clientTCP.print(tail);
 
esp_camera_fb_return(fb);
 
int waitTime = 10000; // timeout 10 seconds
long startTimer = millis();
boolean state = false;
 
while ((startTimer + waitTime) > millis()) {
Serial.print(".");
delay(100);
while (clientTCP.available()) {
char c = clientTCP.read();
if (c == '\n') {
if (getAll.length() == 0) state = true;
getAll = "";
}
else if (c != '\r') {
getAll += String(c);
}
if (state == true) {
getBody += String(c);
}
startTimer = millis();
}
if (getBody.length() > 0) break;
}
clientTCP.stop();
 
// Print Information
//Serial.println(getBody);
Serial.println("Photo Sent");
}
else {
getBody = "Connected to api.telegram.org failed.";
Serial.println("Connected to api.telegram.org failed.");
}
return getBody;
}
 
void handleNewMessages(int numNewMessages) {
 
for (int i = 0; i < numNewMessages; i++) {
// Chat id of the requester
String chat_id = String(bot.messages[i].chat_id);
if (chat_id != chatId) {
bot.sendMessage(chat_id, "Unauthorized user", "");
continue;
}
 
// Print message received
String fromName = bot.messages[i].from_name;
String text = bot.messages[i].text;
 
Serial.println(numNewMessages + " From " + fromName + " >" + text + " request");


if (text == "/Motion") {
MotionState = !MotionState;
String welcome = "Status sensor.\n";
Serial.print("MotionState = ");
if (MotionState == true) Serial.println("ON"); else Serial.println("OFF");
//welcome += "Motion : toggle Motion Sensor\n";
if (MotionState == true) welcome += "Motion : ON now\n"; else welcome += "Motion : OFF now\n";
bot.sendMessage(chatId, welcome, "Markdown");
// digitalWrite(FLASH_LED_PIN, MotionState);
}
 
if (text == "/Photo") {
sendPhoto = true;
}

if (text == "/Start") {
String welcome = "Bienvenidos al Sistema de reconocimiento facial con cerradura y sensor de movimiento.\n";
long rssi = WiFi.RSSI() + 100;
welcome += "Sistemas Embebidos P#104 Chacon-Zuñiga\n";
//welcome += "Sinal Wi-Fi: -" + String(rssi) + "\n";
welcome += "/Photo : Tomar foto\n";
//welcome += "/Flash : encencer flash\n";
welcome += "/Motion : Modo sensor de movimiento\n";
welcome += "Motion : Activara el modo sensor de momvimiento\n";
if (MotionState == true) welcome += "Motion : ON now\n"; else welcome += "Motion : OFF now\n";
welcome += "Se recibira un mensaje al detectar movimiento!.\n";
bot.sendMessage(chatId, welcome, "Markdown");
}
}
}
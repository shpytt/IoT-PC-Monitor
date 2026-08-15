#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ArduinoJson.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#include <LittleFS.h>

#define TFT_CS 15
#define TFT_RST 2
#define TFT_DC 0

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
ESP8266WebServer server(80);

const char* ssid = "iPhone Остап";
const char* password = "12121212";

String pendingCmd = "";
String selectedCity = "Lviv";
String wCity = "Loading...";
String wDesc = "";
int wTemp = 0;
String clockTxt = "00:00";
int cpu=0, gpu=0, ram=0;

unsigned long lastDataTime = 0;
bool isOnline = true; 
int logId = 1;

void writeInternalLog(String msg) {
  File logFile = LittleFS.open("/log.txt", "a");
  if (logFile) {
    logFile.println(msg);
    logFile.close();
  }
}

uint16_t getLoadColor(int percent) {
  if (percent > 85) return ST7735_RED;
  if (percent > 60) return ST7735_YELLOW;
  return ST7735_GREEN;
}

String getHtmlColor(int percent) {
  if (percent > 85) return "#ff4d4d"; 
  if (percent > 60) return "#ffcc00"; 
  return "#00ffcc"; 
}

void drawBar(int x, int y, int percent, uint16_t color) {
  int w = 110; int h = 8;
  int fill = (w * percent) / 100;
  tft.fillRect(x, y, fill, h, color);
  tft.fillRect(x + fill, y, w - fill, h, ST7735_BLACK);
  tft.drawRect(x-1, y-1, w+2, h+2, ST7735_WHITE);
}

void showOfflineScreen() {
  tft.fillScreen(ST7735_BLACK);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  
  String l1 = "Waiting for the";
  String l2 = "program launching";
  String l3 = "on PC.";
  String l4 = "IP: " + WiFi.localIP().toString(); 
  
  int16_t x1, y1; uint16_t w, h;
  tft.getTextBounds(l1, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((128 - w) / 2, 35); tft.print(l1);
  
  tft.getTextBounds(l2, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((128 - w) / 2, 50); tft.print(l2);
  
  tft.getTextBounds(l3, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((128 - w) / 2, 65); tft.print(l3);

  tft.setTextColor(ST7735_GREEN);
  tft.getTextBounds(l4, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((128 - w) / 2, 90); tft.print(l4);
}

void handleData() {
  lastDataTime = millis();
  isOnline = true;

  JsonDocument doc;
  deserializeJson(doc, server.arg("plain"));
  
  cpu = doc["cpu"]; gpu = doc["gpu"]; ram = doc["ram"];
  clockTxt = doc["clock"].as<String>();
  wCity = doc["weather_city"].as<String>();
  wTemp = doc["weather_temp"];
  wDesc = doc["weather_desc"].as<String>();

  tft.fillScreen(ST7735_BLACK);
  
  int16_t x1, y1; uint16_t w, h;
  
  tft.setTextSize(2); tft.setTextColor(ST7735_WHITE);
  tft.getTextBounds(clockTxt, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((128 - w) / 2, 5); tft.print(clockTxt);

  tft.setTextSize(1); tft.setTextColor(ST7735_CYAN);
  String line1 = wCity + ": " + String(wTemp) + (char)247 + "C"; 
  tft.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((128 - w) / 2, 26); tft.print(line1);

  tft.setTextColor(ST7735_YELLOW);
  tft.getTextBounds(wDesc, 0, 0, &x1, &y1, &w, &h);
  tft.setCursor((128 - w) / 2, 38); tft.print(wDesc);

  tft.setTextColor(ST7735_WHITE);
  tft.setCursor(10, 52); tft.printf("CPU: %d%%", cpu);
  drawBar(10, 62, cpu, getLoadColor(cpu));
  
  tft.setCursor(10, 78); tft.printf("GPU: %d%%", gpu);
  drawBar(10, 88, gpu, getLoadColor(gpu));

  tft.setCursor(10, 104); tft.printf("RAM: %d%%", ram);
  drawBar(10, 114, ram, getLoadColor(ram));

  String weatherStr = wCity + "(" + String(wTemp) + "C)";
  String logEntry = String(logId++) + "\t" + clockTxt + "\t" + weatherStr + "\t" + String(cpu) + "\t" + String(gpu) + "\t" + String(ram);
  writeInternalLog(logEntry);
  Serial.println(logEntry);

  JsonDocument out;
  out["cmd"] = pendingCmd;
  out["city"] = selectedCity;
  String response;
  serializeJson(out, response);
  server.send(200, "application/json", response);
  pendingCmd = ""; 
}

void handleRoot() {
  String s = "<!DOCTYPE html><html><head><meta charset='UTF-8' name='viewport' content='width=device-width, initial-scale=1'>";
  s += "<meta http-equiv='refresh' content='2'>"; 
  s += "<style>body{background-color:#0d0d0d;color:#fff;font-family:'Segoe UI',sans-serif;text-align:center;padding:20px;}";
  s += ".card{background:#1a1a1a;border:1px solid #333;border-radius:12px;padding:20px;margin:15px auto;max-width:350px;box-shadow:0 4px 15px rgba(0,0,0,0.5);}";
  s += "h1{color:#fff;font-weight:300;letter-spacing:2px;text-transform:uppercase;}";
  s += ".stat{font-size:18px;font-weight:600;margin:10px 0;display:flex;justify-content:space-between;border-bottom:1px solid #333;padding-bottom:10px;}";
  s += "select{background:#1a1a1a;color:#fff;border:1px solid #444;padding:12px;border-radius:8px;font-size:16px;margin-bottom:20px;width:100%;outline:none;}";
  s += ".btn{background:linear-gradient(145deg,#222,#111);color:#fff;border:1px solid #333;padding:15px;margin:8px 0;border-radius:8px;font-size:15px;cursor:pointer;width:100%;text-transform:uppercase;letter-spacing:1px;transition:0.3s;}";
  s += ".btn:hover{border-color:#555;background:linear-gradient(145deg,#2a2a2a,#1a1a1a);}</style></head><body>";
  
  s += "<h1>PC Dashboard</h1>";
  
  s += "<div class='card'>";
  s += "<div class='stat'><span>CPU:</span><span style='color:" + getHtmlColor(cpu) + "'>" + String(cpu) + "%</span></div>";
  s += "<div class='stat'><span>GPU:</span><span style='color:" + getHtmlColor(gpu) + "'>" + String(gpu) + "%</span></div>";
  s += "<div class='stat'><span>RAM:</span><span style='color:" + getHtmlColor(ram) + "'>" + String(ram) + "%</span></div>";
  s += "</div>";

  s += "<div class='card'>";
  s += "<select onchange='location=\"/city?v=\"+this.value'>";
  s += "<option value='Lviv' " + String(selectedCity=="Lviv"?"selected":"") + ">Львів</option>";
  s += "<option value='Kyiv' " + String(selectedCity=="Kyiv"?"selected":"") + ">Київ</option>";
  s += "<option value='Odesa' " + String(selectedCity=="Odesa"?"selected":"") + ">Одеса</option>";
  s += "<option value='Kharkiv' " + String(selectedCity=="Kharkiv"?"selected":"") + ">Харків</option>";
  s += "<option value='Dnipro' " + String(selectedCity=="Dnipro"?"selected":"") + ">Дніпро</option>";
  s += "<option value='Frankivsk' " + String(selectedCity=="Frankivsk"?"selected":"") + ">Івано-Франківськ</option>";
  s += "<option value='Borshchovychi' " + String(selectedCity=="Borshchovychi"?"selected":"") + ">Борщовичі</option>";
  s += "<option value='Murovane' " + String(selectedCity=="Murovane"?"selected":"") + ">Муроване</option>";
  s += "</select>";
  
  s += "<button class='btn' onclick='location=\"/btn?c=clean\"'>🧹 Чистка Temp</button>";
  s += "<button class='btn' onclick='location=\"/btn?c=lock\"'>🔒 Блокувати ПК</button>";
  s += "<button class='btn' onclick='location=\"/log\"'>📄 Відкрити системні логи</button>";
  s += "</div></body></html>";
  
  server.send(200, "text/html", s);
}

void setup() {
  Serial.begin(115200);
  
  if (LittleFS.begin()) {
    Serial.println("LittleFS started successfully.");
    writeInternalLog("ID\tTIME\tWEATHER\tCPU\tGPU\tRAM");
  } else {
    Serial.println("LittleFS Error!");
  }

  tft.initR(INITR_BLACKTAB); 
  tft.fillScreen(ST7735_BLACK);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) delay(500);
  
  showOfflineScreen(); 
  
  server.on("/", handleRoot);
  server.on("/data", HTTP_POST, handleData);
  server.on("/btn", []() { pendingCmd = server.arg("c"); server.sendHeader("Location", "/"); server.send(303); });
  server.on("/city", []() { selectedCity = server.arg("v"); server.sendHeader("Location", "/"); server.send(303); });
  
  server.on("/log", []() {
    if (!LittleFS.exists("/log.txt")) {
      server.send(200, "text/plain", "No logs yet.");
      return;
    }
    File f = LittleFS.open("/log.txt", "r");
    server.streamFile(f, "text/plain");
    f.close();
  });

  server.begin();
}

void loop() { 
  server.handleClient(); 

  if (isOnline && (millis() - lastDataTime > 4000)) {
    isOnline = false;
    showOfflineScreen(); 
    cpu = 0; gpu = 0; ram = 0;
  }
}
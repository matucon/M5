// TTM8 RAM Writer for M5ATOM Matrix
// アクセスURL：http://ttm8.local かhttp://{IPアドレス}
// 先に静的リソースをSPIFFS へアップロードしておく。

#include <M5Atom.h>
#include <ESPmDNS.h>
#include <CSV_Parser.h>
#include <map>

#include "ESPAsyncWebServer.h"
#include "SPIFFS.h"

const char* ssid = "xxxx";
const char* pass = "xxxx";

void draw(const int* image);
uint16_t toDecimal(String hexValue);
void shiftBus(uint16_t value);
void setBus(uint16_t dValue);
void latch();
void ramwe() ;
void exrst();

void handleWriteCode(AsyncWebServerRequest *request);
void handleData(AsyncWebServerRequest *request);
void handleRamClear(AsyncWebServerRequest *request);
void handleRclk(AsyncWebServerRequest *request);
void handleRamWe(AsyncWebServerRequest *request);
void handleReset(AsyncWebServerRequest *request);
void handleGpio(AsyncWebServerRequest *request);
void handleGpioStatus(AsyncWebServerRequest *request);
void handleIp(AsyncWebServerRequest *request);
void handleNotFound(AsyncWebServerRequest *request);

AsyncWebServer server(80);
std::vector<int> gpios{19, 23, 33, 21, 25};
const uint16_t interval = 1;
const int WIFI[]    = {0,0xffffff,0xffffff,0xffffff,0,0xffffff,0,0,0,0xffffff,0,0,0x00ffff,0,0,0,0x00ffff,0,0x00ffff,0,0,0,0x0000ff,0,0};
const int WRITING[] = {0,0x777777,0x777777,0x777777,0,0x777777,0,0xff0000,0,0x777777,0x777777,0,0xff0000,0xff0000,0x777777,0x777777,0,0,0,0x777777,0,0x777777,0x777777,0x777777,0};
uint8_t ledNoStatus = 0;
uint8_t brightness = 15;
bool drawStatus = false;

void drawProgress(CRGB color) {
  M5.dis.drawpix(ledNoStatus++, color);
  M5.dis.setBrightness(brightness);
}

void setupWifi() {
  drawProgress(0x408f00);
  WiFi.disconnect(false,true);
  drawProgress(0x408f00);
  WiFi.begin(ssid, pass);
  drawProgress(0x408f00);
  while( WiFi.status() != WL_CONNECTED) {
      delay(500); 
      Serial.print("."); 
  }

  drawProgress(0x408f00);
  if(MDNS.begin("ttm8")){
    Serial.println("aMDNS responder started: http://ttm8.local");
  }
  drawProgress(0x408f00);
}

void setupGpio() {
  drawProgress(0x008f00);
  pinMode(GPIO_NUM_19, OUTPUT); // シフトレジスタ SER
  pinMode(GPIO_NUM_23, OUTPUT); // シフトレジスタ SRCLK
  pinMode(GPIO_NUM_33, OUTPUT); // シフトレジスタ RCLK
  pinMode(GPIO_NUM_21, OUTPUT); // TTMP8 RAM_WE
  pinMode(GPIO_NUM_25, OUTPUT); // TTMP8 EXRST
  
  // 各バスをクリア
  drawProgress(0x008f00);
  setBus(0);
  latch();
}

void setupSpiffs() {
  drawProgress(0x008f00);
  if(!SPIFFS.begin(true)){
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
}

void setupServer() {
  drawProgress(0x008f00);
  server.serveStatic("/", SPIFFS, "/www/").setDefaultFile("index.html");
  server.on("/ttm8.js", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/ttm8.js", "text/javascript");
  });
  server.on("/write"       , HTTP_POST, handleWriteCode);
  server.on("/regista/data", HTTP_GET , handleData);
  server.on("/ramclear"    , HTTP_GET , handleRamClear);
  server.on("/rclk"        , HTTP_GET , handleRclk);
  server.on("/ramwe"       , HTTP_GET , handleRamWe);
  server.on("/reset"       , HTTP_GET , handleReset);
  server.on("/gpio/status" , HTTP_GET , handleGpioStatus);
  server.on("/gpio"        , HTTP_GET , handleGpio);
  server.on("/ip"          , HTTP_GET , handleIp);
  server.onNotFound(handleNotFound);

  server.begin();
}

void setup() {
  M5.begin(true, false, true);
  Serial.begin(115200);
  delay(100);

  setupGpio();
  setupSpiffs();
  setupWifi();
  setupServer();

  draw(WIFI);
}

void loop() {
}

void viewStatus() {
  M5.dis.clear();
  M5.dis.drawpix(14, digitalRead(GPIO_NUM_19) == HIGH ? 0x00FF00 : 0xFF0000);
  M5.dis.drawpix(19, digitalRead(GPIO_NUM_23) == HIGH ? 0x00FF00 : 0xFF0000);
  M5.dis.drawpix(24, digitalRead(GPIO_NUM_33) == HIGH ? 0x00FF00 : 0xFF0000);
  M5.dis.drawpix( 5, digitalRead(GPIO_NUM_21) == HIGH ? 0x00FF00 : 0xFF0000);
  M5.dis.drawpix(10, digitalRead(GPIO_NUM_25) == HIGH ? 0x00FF00 : 0xFF0000);
}

/**
 * アイコンを表示します。
 */
void draw(const int* image) {
  if (drawStatus) {
    viewStatus();
  } else {
    for (int i = 0; i < 25; i++) {
      M5.dis.drawpix(i, image[i]);
    }
  }
  M5.dis.setBrightness(brightness);
}

void processingOn() {
  if (drawStatus) {
    viewStatus();
  } else {
    M5.dis.drawpix(24, 0x008f00);
  }
  M5.dis.setBrightness(brightness);
}

void processingOff() {
  delay(10);
  if (drawStatus) {
    viewStatus();
  } else {
    M5.dis.drawpix(24, 0x000000);
  }
  M5.dis.setBrightness(brightness);
}

/**
 * 入力された16進数文字列を数値化します。
 */
uint16_t toDecimal(String hexValue) {
  String editValue = "0x" + hexValue;
  return strtol(editValue.c_str(), NULL, 0);
}

/**
 * シフトレジスタに対して、1ビット分のデータをシフトします。
 */
void shiftBus(uint16_t value) {
  digitalWrite(GPIO_NUM_19, value == 0 ? LOW : HIGH);
  digitalWrite(GPIO_NUM_23, HIGH);
  delay(interval);
  digitalWrite(GPIO_NUM_23, LOW);
  digitalWrite(GPIO_NUM_19, LOW);
}

/**
 * 指定されたバスに指定されたデータを設定します。
 */
void setBus(uint16_t dValue) {
  for (int i = 15; i >= 0; i--) {
    shiftBus(bitRead(dValue, i));
  }
}

/**
 * シフトレジスタをラッチします。
 */
void latch() {
  digitalWrite(GPIO_NUM_33, HIGH);
  delay(interval);
  digitalWrite(GPIO_NUM_33, LOW);
}

/**
 * TTM8 の書き込み信号を発信します。
 */
void ramwe() {
  digitalWrite(GPIO_NUM_21, HIGH);
  delay(interval);
  digitalWrite(GPIO_NUM_21, LOW);
}

/**
 * TTM8 の外部リセット信号を発信します。
 */
void exrst() {
  digitalWrite(GPIO_NUM_21, HIGH);
  digitalWrite(GPIO_NUM_25, HIGH);
  delay(1200);
  digitalWrite(GPIO_NUM_25, LOW);
  digitalWrite(GPIO_NUM_21, LOW);
}

void handleWriteCode(AsyncWebServerRequest *request) {
  draw(WRITING);
  String runcode = request->arg("runcode");
  const char* c = runcode.c_str();
  CSV_Parser cp(c, "xx", false); // 第3引数は、ヘッダレス指定
  uint32_t *csvAddr = (uint32_t*)cp[0];
  uint32_t *csvData = (uint32_t*)cp[1];

  // コード書き込み
  std::vector<uint32_t> usingAddr;
  for (int i = 0; i < cp.getRowsCount(); i++) {
    setBus((csvAddr[i] << 8) + csvData[i]);
    latch();
    ramwe();
    usingAddr.push_back(csvAddr[i]);
  }

  // 未使用領域削除
  String cleansing = request->arg("cleansing");
  if (cleansing == "true") {
    for (int i = 0x00; i <= 0xbf; i++) {
      bool found = std::find(usingAddr.begin(), usingAddr.end(), i) != usingAddr.end();
      if (!found) {
        setBus(i << 8);
        latch();
        ramwe();
      }
    }
  }

  delay(50);
  exrst();
  setBus(0);
  latch();
  delay(50);
  draw(WIFI);
  request->send(200, "application/json", "{}");
}

void handleData(AsyncWebServerRequest *request) {
  processingOn();
  if (request->hasParam("shift")) {
    shiftBus(request->getParam("shift")->value().toInt());
  } else if (request->hasParam("value")) {
    setBus(toDecimal(request->getParam("value")->value()));
  }
  latch();
  processingOff();
  request->send(200, "application/json", "{}");
}

void handleRamClear(AsyncWebServerRequest *request) {
  draw(WRITING);
  uint8_t addrStart = 0x00;
  uint8_t addrEnd   = 0xbf;
  if (request->hasParam("from") && request->hasParam("to")) {
    addrStart = toDecimal(request->getParam("from")->value());
    addrEnd   = toDecimal(request->getParam("to")->value());
  }

  for (int i = addrStart; i <= addrEnd; i++) {
    setBus(i << 8);
    latch();
    ramwe();
  }
  setBus(0);
  latch();
  draw(WIFI);
  request->send(200, "application/json", "{}");
}

void handleRclk(AsyncWebServerRequest *request) {
  processingOn();
  latch();
  processingOff();
  request->send(200, "application/json", "{}");
}

void handleRamWe(AsyncWebServerRequest *request) {
  processingOn();
  ramwe();
  processingOff();
  request->send(200, "application/json", "{}");
  
}

void handleReset(AsyncWebServerRequest *request) {
  draw(WRITING);
  exrst();
  draw(WIFI);
  request->send(200, "application/json", "{}");
}

void handleGpio(AsyncWebServerRequest *request) {
  processingOn();
  if (!request->hasParam("no") || !request->hasParam("flag")) {
    request->send(400, "application/json", "{}");
  }

  uint8_t no = request->getParam("no")->value().toInt();
  String flag = request->getParam("flag")->value();

  if (no == 0) {
    for(auto x : gpios) {
      if (flag == "click") {
        digitalWrite(no, LOW);
        delay(interval);
        digitalWrite(x, HIGH);
        delay(interval);
        digitalWrite(x, LOW);
      } else {
        digitalWrite(x, flag == "true" ? HIGH : LOW);
      }
    }
  } else {
    if (flag == "click") {
      digitalWrite(no, LOW);
      delay(interval);
      digitalWrite(no, HIGH);
      delay(interval);
      digitalWrite(no, LOW);
    } else {
      digitalWrite(no, flag == "true" ? HIGH : LOW);
    }
  }
  
  processingOff();
  request->send(200, "application/json", "{}");
}

void handleGpioStatus(AsyncWebServerRequest *request) {
  if (!request->hasParam("status")) {
    request->send(400, "application/json", "{}");
  }
  drawStatus = (request->getParam("status")->value() == "true");
  draw(WIFI);
  request->send(200, "application/json", "{}");
}

void handleIp(AsyncWebServerRequest *request) {
  String ip = "{\"ip\":\"" + WiFi.localIP().toString() + "\"}";
  request->send(200, "application/json", ip);
}

void handleNotFound(AsyncWebServerRequest *request){
  request->send(404);
}

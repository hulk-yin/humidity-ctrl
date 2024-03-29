// #include <Arduino.h>
#include "DHTesp.h" // Click here to get the library: http://librarymanager/All#DHTesp
#include <DNSServer.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include "ESPAsyncWebServer.h"
#include "EEPROM.h"

DNSServer dnsServer;
AsyncWebServer server(80);

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
   // 定义成员变量
  std::string status = "OK"; // 状态
  float humidity = 0.0; // 湿度
  float temperatureC = 0.0; // 温度（摄氏度）
  float temperatureF = 0.0; // 温度（华氏度）
  float heatIndexC = 0.0; // 热指数（摄氏度）
  float heatIndexF = 0.0; // 热指数（华氏度）
  // 湿度控制阈值
  int humidityThreshold = 85;
  // FAN和UVLamp状态
  int FANState = LOW;
  int UVLampState = LOW;
  int FANTime=0;
  int UVLampTime=0;
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest *request){
    //request->addInterestingHeader("ANY");
    return true;
  }
  void handleRequest(AsyncWebServerRequest *request) {
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->print("<!DOCTYPE html><html><head><title>Captive Portal</title>");
    response->print("<meta name='viewport' content='width=device-width, initial-scale=1'>");
    response->print("<script>");
    response->print("function toggleUVLamp() { fetch('/toggleUVLamp'); }");
    response->print("function toggleFan() { fetch('/toggleFan'); }");
    response->print("</script>");
    response->print("</head>");
    response->print("<body>");
    response->printf("<table>");
    response->printf("<tr><td>Time</td><td>Status</td><td>Humidity (%)</td><td>Temperature (C)</td><td>(F)</td><td>HeatIndex (C)</td><td>(F)</td></tr>");
    response->printf("<tr><td id='time'>%d</td><td id='status'>%s</td><td id='humidity'>%f</td><td id='temperatureC'>%f</td><td id='temperatureF'>%f</td><td id='heatIndexC'>%f</td><td id='heatIndexF'>%f</td></tr>", millis()/1000, status.c_str(), humidity, temperatureC, temperatureF, heatIndexC, heatIndexF);
    // Fan状态
    response->printf("<tr><td>FAN</td><td id='fanStatus'>%s</td></tr>", FANState == HIGH ? "ON" : "OFF");
    // UVLamp状态
    response->printf("<tr><td>UVLamp</td><td id='uvLampStatus'>%s</td></tr>", UVLampState == HIGH ? "ON" : "OFF");
    response->print("<button onclick='toggleUVLamp()'>Toggle UV Lamp</button>");
    response->print("<button onclick='toggleFan()'>Toggle Fan</button>");
    // 当前湿度阈值
    response->printf("<div>Current Humidity Threshold: <span id='chtspan'>%d</span>\%</div>", humidityThreshold);
    // 设置湿度阈值数字输入字段，范围50-100
    response->printf("<input type='number' id='humidityThreshold' min='50' max='100' value='85'>");
    response->print("<button onclick='setHumidityThreshold()'>Set Humidity Threshold</button>");
    response->print("<div id='res_txt'></div>");
    response->print("<div id='error'></div>");
    response->print("<script>");
    response->print("function updateStatus() {");
    response->print("  fetch('/status')");
    response->print("  .then(response => {");
    response->print("    if (!response.ok) {");
    response->print("      throw new Error('Network response was not ok');");
    response->print("    }");
    // response->print("     response.text().then(txt=>document.getElementById('res_txt').textContent = txt);");
    response->print("    const res = response.json();");
    // 输出返回的数据
    response->print("    console.log(res);");
    response->print("    return res;");

    response->print("  })");
    response->print("  .then(data => {");
    response->print("    document.getElementById('res_txt').textContent = JSON.stringify(data);");
    response->print("    document.getElementById('time').textContent = data.time;");
    response->print("    document.getElementById('status').textContent = data.status;");
    response->print("    document.getElementById('humidity').textContent = data.humidity;");
    response->print("    document.getElementById('temperatureC').textContent = data.temperatureC;");
    response->print("    document.getElementById('temperatureF').textContent = data.temperatureF;");
    response->print("    document.getElementById('heatIndexC').textContent = data.heatIndexC;");
    response->print("    document.getElementById('heatIndexF').textContent = data.heatIndexF;");
    response->print("    document.getElementById('fanStatus').textContent = data.fanStatus == 1 ? 'ON' : 'OFF';");
    response->print("    document.getElementById('uvLampStatus').textContent = data.uvLampStatus == 1 ? 'ON' : 'OFF';");
    response->print("    document.getElementById('chtspan').textContent = data.humidityThreshold;");
    response->print("  })");
    response->print("  .catch(error => {");
    response->print("    document.getElementById('error').textContent = 'There has been a problem with your fetch operation: ' + error;");
    response->print("  });");
    response->print("}");
    response->print("setInterval(updateStatus, 1000);"); // 每秒更新一次状态
    // 添加setHumidityThreshold方法
    response->print("function setHumidityThreshold() {");
    response->print("  let threshold = document.getElementById('humidityThreshold').value;");
    response->print("  fetch('/setHumidityThreshold?threshold=' + threshold)");
    response->print("  .then(response => {");
    response->print("    if (!response.ok) {");
    response->print("       document.getElementById('error').textContent = response.statusText;");
    response->print("        throw new Error('Network response was not ok');");
    response->print("    }");
    // 更新状态
    response->print("    updateStatus();");
    response->print("    return response.json();");
    response->print("  })");
    response->print("}");
    response->print("</script>");
    
    // 插入vconsole.js
    // response->print("<script src='https://cdn.bootcdn.net/ajax/libs/vConsole/3.3.4/vconsole.min.js'></script>");
    // response->print("<script>new VConsole();</script>");
    response->print("</body></html>");
    request->send(response);
  }
  String getStatus() {
    char buffer[512];
    snprintf(buffer, sizeof(buffer), "{\"time\":%d,\"status\":\"%s\",\"humidity\":\"%f\",\"temperatureC\":\"%f\",\"temperatureF\":\"%f\",\"heatIndexC\":\"%f\",\"heatIndexF\":\"%f\",\"fanStatus\":%d,\"uvLampStatus\":%d,\"humidityThreshold\":\"%d\"}", 
    millis()/1000, status.c_str(), humidity , temperatureC , temperatureF  , heatIndexC, heatIndexF , FANState, UVLampState,humidityThreshold);
    return String(buffer);
  }
};

// put function declarations here:
DHTesp dht;
int FANPin = 3;
int DHTPin = 2;
int UVLampPin =1;
CaptiveRequestHandler* captiveRequestHandler;
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();
  String thisBoard= ARDUINO_BOARD;
  Serial.println(thisBoard);


  EEPROM.begin(512);
  // WIFi设置为中继模式
  // 设置设备Wifi名称

  // 设置为客户端模式

  // WiFi.begin("Xiaomi_73A0", "03031902");
  // // 设置设备名称
  // WiFi.setHostname("qihao-s-ctl");
  // // 链接网络
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(1000);
  //   Serial.println("Connecting to WiFi..");
  // }
  
  // IPAddress apIP(192, 168, 4, 1);
  // IPAddress netMsk(255, 255, 255, 0);
  // WiFi.softAPConfig(apIP, apIP, netMsk);

  WiFi.softAP("qihao-fan-01");
  // 设置0pin 为输出，用于控制led
  pinMode(FANPin,OUTPUT);
  pinMode(UVLampPin, OUTPUT);
  // 下拉                                                    ·
  digitalWrite(FANPin, LOW);
  digitalWrite(UVLampPin, HIGH);
 
  // Autodetect is not working reliable, don't use the following line
  // dht.setup(17);
  // use this instead: 
  // 设置 0 pin 为DHT11
  dht.setup(DHTPin, DHTesp::DHT11); // Connect DHT sensor to GPIO 17



  // dnsServer.start(53, "*", WiFi.softAPIP());
  captiveRequestHandler = new CaptiveRequestHandler();
  server.on("/toggleUVLamp", HTTP_GET, [](AsyncWebServerRequest *request){
    // 在这里添加切换UV灯的代码
    (*captiveRequestHandler).UVLampState = !(*captiveRequestHandler).UVLampState ;
    (*captiveRequestHandler).UVLampTime = millis()/1000;
    digitalWrite(UVLampPin, (*captiveRequestHandler).UVLampState);
    request->send(200);
  });

  // 切换风扇的状态
  server.on("/toggleFan", HTTP_GET, [](AsyncWebServerRequest *request){
      // 在这里添加切换风扇的代码
      (*captiveRequestHandler).FANState = !digitalRead(FANPin);
      (*captiveRequestHandler).FANTime = millis()/1000;
      digitalWrite(FANPin, (*captiveRequestHandler).FANState);
      request->send(200);
  });
  server.on("/status", HTTP_GET, [](AsyncWebServerRequest *request){

    request->send(200, "application/json", (*captiveRequestHandler).getStatus());
  });
  // 设置湿度阈值
  server.on("/setHumidityThreshold", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("threshold")) {

      (*captiveRequestHandler).humidityThreshold = request->getParam("threshold")->value().toInt();
      // 写入EEPROM中
      // int 转 uint8_t
      uint8_t threshold = (*captiveRequestHandler).humidityThreshold;
      Serial.printf("Setting humidity threshold to %d\n", threshold);
      EEPROM.write(0, threshold);
      EEPROM.commit();  
      request->send(200, "application/json", (*captiveRequestHandler).getStatus());
    } else {
      Serial.println("Missing threshold parameter");
      request->send(400, "application/json", "{\"error\":\"Missing threshold parameter\"}");
    }
  });
  (*captiveRequestHandler).humidityThreshold = EEPROM.read(0) ;
  server.addHandler(captiveRequestHandler).setFilter(ON_AP_FILTER);//only when requested from AP
  //more handlers...
  server.begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.softAPIP());
  Serial.println("Time\tStatus\tHumidity (%)\tTemperature (C)\t(F)\tHeatIndex (C)\t(F)");

}
int state = LOW ;
void loop() {
  // put your main code here, to run repeatedly:
  delay(dht.getMinimumSamplingPeriod());

  float humidity = dht.getHumidity();
  float temperature = dht.getTemperature();
  int now = millis()/1000;

  // Dereference the pointer to captiveRequestHandler
  (*captiveRequestHandler).status = dht.getStatusString();
  (*captiveRequestHandler).humidity = humidity;
  (*captiveRequestHandler).temperatureC = temperature;
  (*captiveRequestHandler).temperatureF = dht.toFahrenheit(temperature);
  (*captiveRequestHandler).heatIndexC = dht.computeHeatIndex(temperature, humidity, false);
  (*captiveRequestHandler).heatIndexF = dht.computeHeatIndex(dht.toFahrenheit(temperature), humidity, true);

  Serial.printf("%d\t%s\t%f\t%f\t%f\t%f\t%f\n", now, (*captiveRequestHandler).status.c_str(), humidity, temperature, dht.toFahrenheit(temperature), (*captiveRequestHandler).heatIndexC, (*captiveRequestHandler).heatIndexF);
 
  //  按钮控制开关5秒后恢复原始状态
  if(now-(*captiveRequestHandler).FANTime >5){
     // 湿度大于60 点亮led (0 pin,拉高)
    if (humidity >=  (*captiveRequestHandler).humidityThreshold) { 
      (*captiveRequestHandler).FANState = HIGH;
      
    } else { 
      (*captiveRequestHandler).FANState = LOW;
    } 
    digitalWrite(FANPin, (*captiveRequestHandler).FANState);
  }
  // 按钮控制开关5秒后恢复原始状态
  if(now-(*captiveRequestHandler).UVLampTime >5){ 
  //  每8小时开启一次UV灯
    int hour = now  / 60 / 60;
    if (hour % 8 < 2) {
      (*captiveRequestHandler).UVLampState = HIGH;
    } else {
      (*captiveRequestHandler).UVLampState = LOW;
    }
      digitalWrite(UVLampPin, (*captiveRequestHandler).UVLampState);

  }
  ;
}
 
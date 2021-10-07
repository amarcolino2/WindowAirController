/*
  Serial Event example

  When new serial data arrives, this sketch adds it to a String.
  When a newline is received, the loop prints the string and clears it.

  A good test for this is to try it with a GPS receiver that sends out
  NMEA 0183 sentences.

  NOTE: The serialEvent() feature is not available on the Leonardo, Micro, or
  other ATmega32U4 based boards.

  created 9 May 2011
  by Tom Igoe

  This example code is in the public domain.

  http://www.arduino.cc/en/Tutorial/SerialEvent
*/
#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <WiFiClient.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h>
#include <ESP8266FtpServer.h>
#include <ArduinoJson.hpp>
#include <ArduinoJson.h>
#define TX 5
#define RX 4
#define L1 13   //saida para acionamento
#define DEBUG

SoftwareSerial NodeMCU;// RX, TX

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;  // whether the string is complete

//webServer Socket
IPAddress local_IP(192,168,4,22);
IPAddress gateway(192,168,4,9);
IPAddress subnet(255,255,255,0);
//IPAddress server_Remoto(192,168,4,22);
 
ESP8266WiFiMulti wifiMulti;       // Create an instance of the ESP8266WiFiMulti class, called 'wifiMulti'
ESP8266WebServer server(80);       // create a web server on port 80
WebSocketsServer webSocket(81);    // create a websocket server on port 81
uint8_t webSocketNum; 

File fsUploadFile;                                    // a File variable to temporarily store the received file
FtpServer ftpSrv;
 
//informações da rede WIFI
//const char* ssid = "VIVOFIBRA-2891";                 //SSID da rede WIFI
//const char* password =  "Fy8om6vGKn";    //senha da rede wifi

//const char* ssid = "IOT";                 //SSID da rede WIFI
//const char* password =  "engie2018";    //senha da rede wifi

const char* ssid = "Claro-50CD";                 //SSID da rede WIFI
const char* password =  "Engie8266";    //senha da rede wifi


const char *passwordAP = "engie8266";   // The password required to connect to it, leave blank for an open network
const char* mdnsName = "arcontroler"; // Domain name for the mDNS responder


/*//informações do broker MQTT - Verifique as informações geradas pelo CloudMQTT
const char* mqttServer = "161.35.125.51";   //server
const char* mqttUser = "sammy";              //user
const char* mqttPassword = "sammy";      //password
const int mqttPort = 1883;                     //port
const char* mqttTopicSub ="janela/device01/swich/on";            //tópico que sera assinado
*/

const char* mqttServer = "35.153.214.253";   //server
const char* mqttUser = "vsd";          //user
const char* mqttPassword = "vsd";            //password
const int mqttPort = 1883;                   //port

String  type = "dev";
char* mqttTopicSub;
char* mqttTopicSubSw;

WiFiClient espClient;
PubSubClient client(espClient);

void setup() {
  
  pinMode(L1,OUTPUT);
  digitalWrite(L1, HIGH);
  // initialize serial:
  Serial.begin(115200);
  NodeMCU.begin(115200, SWSERIAL_8N1, 4, 5, false, 256);
  while (!Serial);
  
  type+=ESP.getChipId();
  const char* ssidAP = type.c_str();

  char* area = "sala0000/";
  char* data = "/data";
  char* sw   = "/switch";
  mqttTopicSub = (char *) malloc(1 + strlen(ssidAP)+ strlen(data)+ strlen(area )); //tópico que sera assinado
  mqttTopicSubSw = (char *) malloc(1 + strlen(ssidAP)+ strlen(sw)+ strlen(area));

  strcpy(mqttTopicSub, (const char*)area);
  strcat(mqttTopicSub, (const char*)ssidAP);
  strcat(mqttTopicSub, data);
  Serial.println(mqttTopicSub);

  strcpy(mqttTopicSubSw, (const char*)area);
  strcat(mqttTopicSubSw, ssidAP);
  strcat(mqttTopicSubSw, sw);
  Serial.println(mqttTopicSubSw);
  
  startWiFi(ssidAP,passwordAP);// Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  startSPIFFS();               // Start the SPIFFS and list all contents
  startWebSocket();            // Start a WebSocket server
  startMDNS();                 // Start the mDNS responder
  startServer();               // Start a HTTP server with a file read handler and an upload handler

  
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    #ifdef DEBUG
    Serial.println("Conectando ao WiFi..");
    //NodeMCU.println("Conectando ao WiFi..");
    #endif
  }
  #ifdef DEBUG
  Serial.println("Conectado na rede WiFi");
  #endif
  
  randomSeed(micros());
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
 
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.println("Conectando ao Broker MQTT...");
    //NodeMCU.println("Conectando ao Broker MQTT...: ");
    #endif
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqttUser, mqttPassword )) {
      #ifdef DEBUG
      Serial.println("Conectado"); 
      //NodeMCU.println("Conectado");
      #endif
 
    } else {
      #ifdef DEBUG 
      Serial.print("falha estado  ");
      Serial.print(client.state());
      #endif
      delay(2000);
 
    }
  }
 
  //subscreve no tópico
  //client.subscribe(mqttTopicSub);
  client.subscribe(mqttTopicSubSw);
  
  // reserve 200 bytes for the inputString:
  inputString.reserve(200);
}

void startWiFi(const char* ssid, const char* pass) { // Start a Wi-Fi access point, and try to connect to some given access points. Then wait for either an AP or STA connection
  WiFi.persistent (false);
  WiFi.disconnect (true);
  Serial.print("Setting soft-AP configuration ... ");
  Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
  Serial.print("Setting soft-AP ... ");
  Serial.println(WiFi.softAP(ssid, pass) ? "Ready" : "Failed!");
  //WiFi.softAP(ssid, password);             // Start the access point
  Serial.print("Access Point \"");
  Serial.print(ssid);
  Serial.print("IP address:\t");
  Serial.println(WiFi.softAPIP()); 
  Serial.println("\" started\r\n");

  wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");   // add Wi-Fi networks you want to connect to
  wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

  Serial.println("Connecting");
  /*while (wifiMulti.run() != WL_CONNECTED && WiFi.softAPgetStationNum() < 1) {  // Wait for the Wi-Fi to connect
    delay(500);
    Serial.print('.');
  }*/
  Serial.println("\r\n");
  if (WiFi.softAPgetStationNum() == 0) {     // If the ESP is connected to an AP
    Serial.print("Connected to ");
    Serial.println(WiFi.SSID());             // Tell us what network we're connected to
    Serial.print("IP address:\t");
    Serial.print(WiFi.localIP());            // Send the IP address of the ESP8266 to the computer
  } else {                                   // If a station is connected to the ESP SoftAP
    Serial.print("Station connected to ESP8266 AP");
  }
  Serial.println("\r\n");
}

String formatBytes(size_t bytes) { // convert sizes in bytes to KB and MB
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  }
}

void startSPIFFS() { // Start the SPIFFS and list all contents
  SPIFFS.begin();                             // Start the SPI Flash File System (SPIFFS)
  Serial.println("SPIFFS started. Contents:");
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {                      // List the file system contents
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("\tFS File: %s, size: %s\r\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
}

boolean numConnect = false;
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { // When a WebSocket message is received
  switch (type) {
    case WStype_DISCONNECTED:             // if the websocket is disconnected
      Serial.printf("[%u] Disconnected!\n", num);
      numConnect = false;
      //TempCode[0].tempVal = 0;
      //TempCode[1].tempVal = 0;
      //FanCode[0].fanVal   = 0;
      //FanCode[1].fanVal   = 0;
      
      //Flags.numConn = 0;
      break;
    case WStype_CONNECTED: {              // if a new websocket connection is established
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        numConnect = true;
      }
        break;
    case WStype_TEXT: {                    // if new text data is received
        Serial.printf("[%u] get Text: %s\n", num, payload); 
        String text = ((const char *)&payload[0]);
        int str_len;

        webSocket.sendTXT(num, "Power:On");
        
        if(text.indexOf("brand")!=-1){
           text.replace("\\", "");
           int pos = text.indexOf('{');
           String json = text.substring(pos,text.length()-1);
           
           break;
        }else if(text.indexOf("setProtocol")!=-1){
          String protocol = subStr(text, ':', 1);
          Serial.println("SET PROTOCOL:"+String(protocol));
          
        }else if(text.indexOf("airflowoff")!=-1){
          
          Serial.println("Calibration airflowoff");
        }else if(text.indexOf("flowmin")!=-1){
          
          Serial.println("Calibration airflowmin");
        }else if(text.indexOf("airflowmax")!=-1){
          
          Serial.println("Calibration airflowmax");
        }else if (text.indexOf("FlgRead")!=-1){
          
          break;
        }else if (text.indexOf("FlgRec")!=-1){
          
          break;
        }
        else if(text.indexOf("ON")!=-1){
          
          Serial.println("ON");
          
          break;
        }
        else if(text.indexOf("OFF")!=-1){
          
          Serial.println("OFF");
          
          break;
        }
        else if(text.substring(0,3)=="TMP"){
         
          break;
       }
       else if(text.substring(0,3)=="FAN"){
          
          break;
        }
        
        break;     
    }
  }
}

void startWebSocket() { // Start a WebSocket server
  webSocket.begin();                          // start the websocket server
  webSocket.onEvent(webSocketEvent);          // if there's an incomming websocket message, go to function 'webSocketEvent'
  Serial.println("WebSocket server started.");
}

void startMDNS() { // Start the mDNS responder
  MDNS.begin(mdnsName);                        // start the multicast domain name server
  Serial.print("mDNS responder started: http://");
  Serial.print(mdnsName);
  Serial.println(".local");
}

void handleFileUpload() { // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  String path;
  if (upload.status == UPLOAD_FILE_START) {
    path = upload.filename;
    if (!path.startsWith("/")) path = "/" + path;
    if (!path.endsWith(".gz")) {                         // The file server always prefers a compressed version of a file
      String pathWithGz = path + ".gz";                  // So if an uploaded file is not compressed, the existing compressed
      if (SPIFFS.exists(pathWithGz))                     // version of that file must be deleted (if it exists)
        SPIFFS.remove(pathWithGz);
    }
    Serial.print("handleFileUpload Name: "); Serial.println(path);
    fsUploadFile = SPIFFS.open(path, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    path = String();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if (upload.status == UPLOAD_FILE_END) {
    if (fsUploadFile) {                                   // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location", "/success.html");     // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}

String getContentType(String filename) { // determine the filetype of a given filename, based on the extension
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  //else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleNotFound() { // if the requested file or page doesn't exist, return a 404 not found error
  if (!handleFileRead(server.uri())) {        // check if the file exists in the flash memory (SPIFFS), if so, send it
    server.send(404, "text/plain", "404: File Not Found");
  }
}

void startServer() { // Start a HTTP server with a file read handler and an upload handler
  server.on("/edit.html",  HTTP_POST, []() {  // If a POST request is sent to the /edit.html address,
    server.send(200, "text/plain", "");
  }, handleFileUpload);                       // go to 'handleFileUpload'
  server.onNotFound(handleNotFound);          // if someone requests any other file or page, go to function 'handleNotFound'
  // and check if the file exists
  server.begin();                             // start the HTTP server
  Serial.println("HTTP server started.");
}

void callback(char* topic, byte* payload, unsigned int length) {
  //armazena msg recebida em uma sring
  payload[length] = '\0';
  String strMSG = String((char*)payload);
 
  #ifdef DEBUG
  Serial.print("Mensagem chegou do tópico: ");
  Serial.println(topic);
  Serial.print("Mensagem:");
  Serial.print(strMSG);
  Serial.println();
  Serial.println("-----------------------");
  #endif
 
  //aciona saída conforme msg recebida 
  if (strMSG == "OFF"){         //se msg "1"
     digitalWrite(L1, HIGH);  //coloca saída em LOW para ligar a Lampada - > o módulo RELE usado tem acionamento invertido. Se necessário ajuste para o seu modulo
  }else if (strMSG == "ON"){   //se msg "0"
     digitalWrite(L1, LOW);   //coloca saída em HIGH para desligar a Lampada - > o módulo RELE usado tem acionamento invertido. Se necessário ajuste para o seu modulo
  }
 
}
 
//função pra reconectar ao servido MQTT
void reconect() {
  //Enquanto estiver desconectado
  while (!client.connected()) {
    #ifdef DEBUG
    Serial.print("Tentando conectar ao servidor MQTT");
    #endif
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    bool conectado = strlen(mqttUser) > 0 ?
                     client.connect(clientId.c_str(), mqttUser, mqttPassword) :
                     client.connect(clientId.c_str());
 
    if(conectado) {
      #ifdef DEBUG
      Serial.println("Conectado!");
      #endif
      //subscreve no tópico
      client.subscribe(mqttTopicSubSw, 1); //nivel de qualidade: QoS 1
    } else {
      #ifdef DEBUG
      Serial.println("Falha durante a conexão.Code: ");
      Serial.println( String(client.state()).c_str());
      Serial.println("Tentando novamente em 10 s");
      #endif
      //Aguarda 10 segundos 
      delay(10000);
    }
  }
}

/*
  SerialEvent occurs whenever a new data comes in the hardware serial RX. This
  routine is run between each time loop() runs, so using delay inside loop can
  delay response. Multiple bytes of data may be available.
*/
/*
  Recebe dados serial do pino 2 do SV3
*/
void serialEvent() { 
  while (Serial.available()) {
    //get the new byte:
    char inChar = (char)Serial.read();
    //inputString = Serial.readString();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag so the main loop can
    // do something about it:
    if (inChar == '}') {
      stringComplete = true;
    }
  }
}

String subStr(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void jsonDeserialize(String json){
  StaticJsonDocument<1024> doc;

  //DeserializationError error = deserializeJson(doc, json);
  auto error = deserializeJson(doc, json);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }


  JsonObject object = doc.as<JsonObject>();
  
  /*const char* payload = doc["tensao"];
  //String brand = doc["brand"];
  Serial.println(payload);
  
  if (client.connected()){
    Serial.print("Sending payload: ");
    Serial.println(payload);
    
    if (client.publish(mqttTopicSub, payload)) {
      Serial.println("Publish ok");
    }
    else {
      Serial.println("Publish failed");
    }
  }*/
  
  /*
  //int lastIndex = brand.length() - 24;
  //brand.remove(lastIndex);
  
  BrandCode.mod=2;
  acState.mod=(byte)0;
  strcpy(BrandCode.brand, (const char*)doc["brand"]);
  //BrandCode.protocol=(int) doc["protocol"];
  //BrandCode.bits=(int)doc["bits"];
  String strCode="";
  
  strCode = (const char*)doc["ON_TEMP_VAL"];
  Serial.println(strCode);
  BrandCode.temp_def=(byte) strCode.toInt();
  Serial.println("Temp Val:"+ String(BrandCode.temp_def));
  
  strCode = (const char*)doc["ON_FAN_VAL"];
  Serial.println(strCode); 
  BrandCode.fan_def=(byte) strCode.toInt();
  Serial.println("Fan Val:"+ String(BrandCode.fan_def));
  
  strCode = (const char*)doc["ON_AUTO"];
  Serial.println(strCode);
  if(strCode.indexOf("Set")!=-1){
    Serial.println("Set: Mod Auto");
    BrandCode.on_auto=1;
  }else {
    BrandCode.on_auto=0;
  }

  yield();
  
  strCode = (const char*)doc["ON_COOL"];
  Serial.println(strCode);
  if(strCode.indexOf("Set")!=-1){
    Serial.println("Set: Mod Cool");
    BrandCode.on_cool=1;
  }else{
    BrandCode.on_cool=0;
  }

  strCode = (const char*)doc["ON_DRY"];
  Serial.println(strCode);
  if(strCode.indexOf("Set")!=-1){
    Serial.println("Set: Mod Dry");
    BrandCode.on_dry=1;
  }else{
    BrandCode.on_dry=0;
  }

  strCode = (const char*)doc["ON_HEAT"];
  Serial.println(strCode);
  if(strCode.indexOf("Set")!=-1){
    Serial.println("Set: Mod Heat");
    BrandCode.on_heat=1;
  }
  else{
    BrandCode.on_heat=0;
  }

  yield();
      
  EEPROM.begin(sizeof(BrandCode));
  EEPROM.put(0, BrandCode);
  EEPROM.commit();
  EEPROM.end(); 
  delay(100);

  EEPROM.begin(sizeof(BrandCode)+sizeof(TempCode));
  EEPROM.put(sizeof(BrandCode), TempCode);
  EEPROM.commit();
  EEPROM.end(); 
  delay(100);

  EEPROM.begin(sizeof(BrandCode)+sizeof(TempCode)+sizeof(acState));
  EEPROM.put(sizeof(BrandCode)+sizeof(TempCode), acState);
  EEPROM.commit();
  EEPROM.end();
  delay(100);
  
  /*EEPROM.begin(sizeof(FanCode));
  EEPROM.put(sizeof(BrandCode)+sizeof(TempCode), FanCode);
  delay(200);
  EEPROM.commit();
  EEPROM.end(); 
  delay(100);*/
/*
  BrandCode.size = (unsigned int)0;
  BrandCode.mod=0;
  acState.mod=(byte)1;
  
  EEPROM.begin(sizeof(BrandCode));
  EEPROM.get(0, BrandCode);
  EEPROM.end();
  delay(100);
  
  EEPROM.begin(sizeof(BrandCode)+sizeof(TempCode));
  EEPROM.get(sizeof(BrandCode), TempCode);
  EEPROM.end();
  delay(100);

  EEPROM.begin(sizeof(BrandCode)+sizeof(TempCode)+sizeof(acState));
  EEPROM.get(sizeof(BrandCode)+sizeof(TempCode), acState);
  EEPROM.end();
  
  /*EEPROM.begin(sizeof(FanCode));
  EEPROM.get(sizeof(BrandCode)+sizeof(TempCode), FanCode);
  EEPROM.end();*/
 /*
  Serial.println(sizeof(BrandCode)+sizeof(TempCode)+sizeof(acState));
  Serial.println(BrandCode.mod);
  Serial.println(acState.mod);
  Serial.println((int)BrandCode.size);
  Serial.print("On: ");
  for(int i=0;i<BrandCode.size;i++){
    Serial.print((char)BrandCode.on_code[i]);
  }
  
  Serial.println("");
  Serial.print("Off: ");
  for(int i=0;i<BrandCode.size;i++){
    Serial.print((char)BrandCode.off_code[i]);
  }*/
}

unsigned long currentMillis    = 0;
unsigned long previousMillis   = 0;
unsigned int interval          = 5000; 
byte message[] = {0x55, 0xaa, 0x03, 0x27, 0x00, 0x00, 0x2a};

void loop() { 
  static int counter = 0;
  server.handleClient();
  yield();
  ESP.wdtFeed();
  webSocket.loop();
  
  if (!client.connected()) {
    reconect();
  }
  
  ++counter;
  client.loop();
  
  //inputString = "{\"V\":\"1.32\",\"I\":\"0.00\",\"PAT\":\"0.00\",\"PAP\":\"0.00\",\"FP\":\"0.00\",\"Tin\":\"24\",\"Tout1\":\"22.63\",\"SC\":\"0\"}";
  
  /*if (stringComplete) {
    const char* payload = inputString.c_str();
    Serial.println(mqttTopicSub);
    Serial.println();
    Serial.println(payload);
  
    if (client.connected()){
      Serial.print("Sending payload: ");
      Serial.println(inputString);
      if (client.publish(mqttTopicSub, payload)) {
        Serial.println("Publish ok");
      }
      else {
        Serial.println("Publish failed");
      }
    }
    inputString = "";
    stringComplete = false;
  }*/

    String content,pkt;
    char character;
    while (NodeMCU.available() > 0) {
      character = NodeMCU.read();
      content.concat(character);
    }
    if((content.indexOf('{')!=-1)&&(content.indexOf('}')!=-1))  // filtra início e fim da string válida
    {
        pkt = content.substring(content.indexOf('{'),content.indexOf('}')+1); // coleta pacote string válida
        Serial.println(pkt);
        Serial.println();
    }
    //if (character == '}') {
    //const char* payload = pkt.c_str();
    
    if (content != ""){
      if(numConnect == false){
      currentMillis = millis();
      if (abs(currentMillis - previousMillis) >= interval-4000) {
        previousMillis = currentMillis;
      Serial.println(content);
      if (client.connected()){
        Serial.print("Sending payload: ");
        Serial.println(mqttTopicSub);
        Serial.println(inputString);
        NodeMCU.println("");
        //mqttTopicSub = "sala0000/dev3879557/data";
        if (client.publish(mqttTopicSub, pkt.c_str())) {//
          Serial.println("Publish ok");
        }
        else {
          Serial.println("Publish failed");
        }
      }
    }
    }
    else{
      currentMillis = millis();
      if (abs(currentMillis - previousMillis) >= interval) {
        previousMillis = currentMillis;
        if (content != "") {
        Serial.println(content);
          if (client.connected()){
            Serial.print("Sending payload: ");
            Serial.println(mqttTopicSub);
            Serial.println(inputString);
            NodeMCU.println("");
            if (client.publish(mqttTopicSub, pkt.c_str())) {
              Serial.println("Publish ok");
              //NodeMCU.println("Publish ok");
            }
            else {
              Serial.println("Publish failed");
              //NodeMCU.println("Publish failed");
            }
          }
        }
      }
    }
   }
  //}
}

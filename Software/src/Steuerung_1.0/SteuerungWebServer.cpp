// brausteuerung@AndreBetz.de
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include "SteuerungWebServer.h"
#include "DbgConsole.h"

static AsyncWebServer mServer(80);
///////////////////////////////////////////////////////////////////////////
// upload html
///////////////////////////////////////////////////////////////////////////
const char upload_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><title>File Upload</title></head>
<body><h1>File Upload</h1>
<form method="POST" action="/uploadDo" enctype="multipart/form-data" target="iframe">
<input type="file" name="upload"><input type="submit" value="Upload"></form>
<iframe style="visibility: hidden;" src="http://" )+local_IPstr+"/Usm" name="iframe"></iframe>
<a href="/">Back</a>
</body></html>
)rawliteral";
///////////////////////////////////////////////////////////////////////////
// update html
///////////////////////////////////////////////////////////////////////////
const char update_html[] PROGMEM = R"rawliteral(
  <h1>Choose .ino or .bin file for update</h1>
  <form id='form' method='POST' action='/updateProcess' enctype='multipart/form-data'>
  <input type='file' name='file' id='file'>
  <br><input type='submit' class='btn' value='Update It'></form>
)rawliteral";
///////////////////////////////////////////////////////////////////////////
// httpProcessUpdate
///////////////////////////////////////////////////////////////////////////
void httpProcessUpdate(AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final) {
  uint32_t free_space = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
  if (!index)   {
    CONSOLELN("process update");
    Update.runAsync(true);
    if (!Update.begin(free_space))     {
      Update.printError(Serial);
    }
    CONSOLE("UploadStart: ");
    CONSOLELN(filename.c_str());
  }

  if (Update.write(data, len) != len)   {
    Update.printError(Serial);
  }

  if (final)   {
    if (!Update.end(true))     {
      Update.printError(Serial);
    } else {
      SteuerungWebServer::mSettings->setRestartEsp(true);
      CONSOLELN("Update complete");
    }
  }
}
///////////////////////////////////////////////////////////////////////////
// getContentType
///////////////////////////////////////////////////////////////////////////
String processorSetup(const String& var){
  if(var == "OFFINPUT"){
    return String(SteuerungWebServer::mSettings->getKalM());
  } 
  else if(var == "ONINPUT"){
    return String(SteuerungWebServer::mSettings->getKalT());
  }
  else if(var == "SWITCHON"){
    return String(SteuerungWebServer::mSettings->getSwitchOn());
  }
  else if(var == "SWITCHOFF"){
    return String(SteuerungWebServer::mSettings->getSwitchOff());
  }
  else if(var == "SWITCHPROTOCOL"){
    return String(SteuerungWebServer::mSettings->getSwitchProtocol());
  }
  else if(var == "SWITCHPULSELENGTH"){
    return String(SteuerungWebServer::mSettings->getSwitchPulseLength());
  }
  else if(var == "SWITCHBITS"){
    return String(SteuerungWebServer::mSettings->getSwitchBits());
  }
  else if(var == "SWITCHREPEATS"){
    return String(SteuerungWebServer::mSettings->getSwitchRepeat());
  }
  return String();
}

void processorSetupGet(AsyncWebServerRequest *request) {
  String inputMessage;
  if (request->hasParam("OnInput")) {
      inputMessage = request->getParam("OnInput")->value();
      CONSOLELN(inputMessage);
      SteuerungWebServer::mSettings->setKalT(inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("OffInput")) {
      inputMessage = request->getParam("OffInput")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setKalM(inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("SwitchOn")) {
      inputMessage = request->getParam("SwitchOn")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setSwitchOn(inputMessage.toInt());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("SwitchOff")) {
      inputMessage = request->getParam("SwitchOff")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setSwitchOff(inputMessage.toInt());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("SwitchProtocol")) {
      inputMessage = request->getParam("SwitchProtocol")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setSwitchProtocol(inputMessage.toInt());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("SwitchPulseLength")) {
      inputMessage = request->getParam("SwitchPulseLength")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setSwitchPulseLength(inputMessage.toInt());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("SwitchBits")) {
      inputMessage = request->getParam("SwitchBits")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setSwitchBits(inputMessage.toInt());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("SwitchRepeats")) {
      inputMessage = request->getParam("SwitchRepeats")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setSwitchRepeat(inputMessage.toInt());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("PwInput")) {
      inputMessage = request->getParam("PwInput")->value();
  }
  request->send(200, "text/html", "<a href=\"/\">Return to Home Page</a>");
}
///////////////////////////////////////////////////////////////////////////
// getContentType
///////////////////////////////////////////////////////////////////////////
String getContentType(String filename) { 
  if (filename.endsWith(".html"))       return "text/html";
  else if (filename.endsWith(".htm"))   return "text/html";
  else if (filename.endsWith(".css"))   return "text/css";
  else if (filename.endsWith(".js"))    return "application/javascript";
  else if (filename.endsWith(".ico"))   return "image/x-icon";
  else if (filename.endsWith(".png"))   return "image/png";
  else if (filename.endsWith(".gif"))   return "image/gif";
  else if (filename.endsWith(".jpg"))   return "image/jpeg";
  else if (filename.endsWith(".xml"))   return "text/xml";
  else if (filename.endsWith(".pdf"))   return "application/pdf";
  else if (filename.endsWith(".zip"))   return "application/zip";  
  else if (filename.endsWith(".gz"))    return "application/x-gzip";
  else if (filename.endsWith(".json"))  return "text/plain";            
  return "text/plain";
}
///////////////////////////////////////////////////////////////////////////
// directory
/////////////////////////////////////////////////////////////////////////// 
void FSzeigen() {
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  float fileTotalKB = fs_info.totalBytes / 1024.0;
  float fileUsedKB = fs_info.usedBytes / 1024.0;
  Serial.print("\nFilesystem Total KB "); Serial.print(fileTotalKB);
  Serial.print(" benutzt KB "); Serial.println(fileUsedKB);

  Dir dir = SPIFFS.openDir("/");  // Dir ausgeben
  while (dir.next()) {
    Serial.print(dir.fileName()); Serial.print("\t");
    File f = dir.openFile("r");
    Serial.println(f.size());
  }
}
///////////////////////////////////////////////////////////////////////////
// Datei lesen
/////////////////////////////////////////////////////////////////////////// 
void readFile(fs::FS &fs, String filename){
  String logmessage = "Read:" + filename;
  File file = fs.open("/" + filename, "r");
  if(!file || file.isDirectory()){
    CONSOLELN(F("error read"));
  }
  CONSOLELN(F("read file:"));
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  file.close();
  CONSOLELN(fileContent);
}
///////////////////////////////////////////////////////////////////////////
// Datenzeigen
/////////////////////////////////////////////////////////////////////////// 
/*
void SteuerungWebServer::Datenzeigen() {
  String message = "received\n";
  message += "URI: ";
  message += mServer.url();
  message += "\nMethod: ";
  message += (mServer.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += mServer.args();
  message += "\n";
  for (uint8_t i = 0; i < mServer.args(); i++) {
    message += " " + mServer.argName(i) + ": " + mServer.arg(i) + "\n";
  }
  CONSOLELN(message);
}
*/
///////////////////////////////////////////////////////////////////////////
// handle File Upload
///////////////////////////////////////////////////////////////////////////
// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  CONSOLELN(logmessage);

  if (!index) {
    logmessage = "Upload Start: " + String(filename);
    request->_tempFile = SPIFFS.open("/" + filename, "w");
    CONSOLELN(logmessage);
  }
  if (len) {
    request->_tempFile.write(data, len);
    logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
    CONSOLELN(logmessage);
  }
  if (final) {
    logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
    request->_tempFile.close();
    CONSOLELN(logmessage);
    FSzeigen();
    readFile(SPIFFS,filename.c_str());
    request->redirect("/");
  }
}
///////////////////////////////////////////////////////////////////////////
// Class
///////////////////////////////////////////////////////////////////////////
Settings* SteuerungWebServer::mSettings;

SteuerungWebServer::SteuerungWebServer(Settings* set) {
  SteuerungWebServer::mSettings = set;
}

void SteuerungWebServer::begin() {
  SPIFFS.begin();
  mServer.begin();

///////////////////////////////////////////////////////////////////////////
// Root
///////////////////////////////////////////////////////////////////////////
  mServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    CONSOLELN(F("root"));
    request->send(SPIFFS, "/start.html", String(), false);
  });
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/uploadDo", HTTP_POST, [](AsyncWebServerRequest *request) {
    CONSOLELN(F("uploadDo"));
    request->send(200); }, handleUpload);       
  mServer.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request){
    CONSOLELN(F("upload"));  
    request->send(200, "text/html", upload_html);
  });
  mServer.onFileUpload(handleUpload);
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    CONSOLELN(F("download"));    
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/settings.json", String(), true);
    response->addHeader(F("Server"), F("ESP Async Web Server"));
    request->send(response);
  });
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
    CONSOLELN(F("update"));    
    request->send(200,"text/html",update_html);
  });
  mServer.on("/updateProcess", HTTP_POST, [](AsyncWebServerRequest *request){
      CONSOLELN(F("updateProcess"));
      request->send(200);
    }, httpProcessUpdate);      
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/setup", HTTP_GET, [](AsyncWebServerRequest *request) {
    CONSOLELN(F("setup"));
    request->send(SPIFFS, "/setup.html", String(), false,processorSetup);
  });
  mServer.on("/setupProcess", HTTP_GET, processorSetupGet);
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/format", HTTP_GET, [](AsyncWebServerRequest *request) {
    CONSOLELN(F("format"));
    SPIFFS.end();
    SPIFFS.begin();
    CONSOLELN(SPIFFS.format());
    SPIFFS.begin();
    request->send(200, "text/html", "format done");
  });
  ///////////////////////////////////////////////////////////////////////////
  mServer.onNotFound([](AsyncWebServerRequest *request){
    CONSOLELN(F("not found"));
    int fnsstart = request->url().lastIndexOf('/');
    String fn = request->url().substring(fnsstart);
    CONSOLELN(fn);
    request->send(SPIFFS, fn, String(), false);
  });  
}
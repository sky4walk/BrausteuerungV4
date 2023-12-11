// brausteuerung@AndreBetz.de
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
//#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebSrv.h>
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
<form action="/"><button onclick="window.location.href='/'">Back</button></form>
<iframe style="visibility: hidden;" src="http://" )+local_IPstr+"/Usm" name="iframe"></iframe>
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
  if(var == "ZIELTEMP"){
    return String(SteuerungWebServer::mSettings->getTemp(0));
  } else if(var == "RASTZEIT"){
    return String(SteuerungWebServer::mSettings->getTime(0));
  } else if(var == "PIDKP"){
    return String(SteuerungWebServer::mSettings->getPidKp());
  } else if(var == "PIDKI"){
    return String(SteuerungWebServer::mSettings->getPidKi());
  } else if(var == "PIDKD"){
    return String(SteuerungWebServer::mSettings->getPidKd());  
  } else if(var == "PIDCYCLE"){
    return String(SteuerungWebServer::mSettings->getPidWindowSize()/1000);  
  } else if(var == "KALIBM"){
    return String(SteuerungWebServer::mSettings->getKalM());
  } else if(var == "KALIBT"){
    return String(SteuerungWebServer::mSettings->getKalT());
  } else if(var == "SWITCHON"){
    return String(SteuerungWebServer::mSettings->getSwitchOn());
  } else if(var == "SWITCHOFF"){
    return String(SteuerungWebServer::mSettings->getSwitchOff());
  } else if(var == "SWITCHPROTOCOL"){
    return String(SteuerungWebServer::mSettings->getSwitchProtocol());
  } else if(var == "SWITCHPULSELENGTH"){
    return String(SteuerungWebServer::mSettings->getSwitchPulseLength());
  } else if(var == "SWITCHBITS"){
    return String(SteuerungWebServer::mSettings->getSwitchBits());
  } else if(var == "SWITCHREPEATS"){
    return String(SteuerungWebServer::mSettings->getSwitchRepeat());
  } else if(var == "APMODEBOX"){ 
    if (SteuerungWebServer::mSettings->getUseAP()) {
      return String("checked");
    } else {
      return String("");
    }
  }
  return String();
}
void showParams(AsyncWebServerRequest *request) {
  int params = request->params();
  String outMessage = String("Params:") + String(params) + String("\n");
  CONSOLELN(outMessage);
  for (int i = 0; i < params; i++) {
    AsyncWebParameter *p = request->getParam(i);
    if (p->isFile()) {
      outMessage = String("FILE:") + String(p->name().c_str()) + String(":") + String(p->value().c_str()) + String(":") + String(p->size()) + String("\n");
    } else if (p->isPost()) {
      outMessage = String("Post:") + String(p->name().c_str()) + String(":") + String(p->value().c_str()) + String("\n");
    } else {
      outMessage = String("Get:") + String(p->name().c_str()) + String(":") + String(p->value().c_str()) + String("\n");
    }
    CONSOLELN(outMessage);
  }
}

void processorSetupGet(AsyncWebServerRequest *request) {
  CONSOLELN(F("processorSetupGet"));
  CONSOLELN(request->url());
  showParams(request);
  String inputMessage;
  if (request->hasParam("ZielTemp")) {
      inputMessage = request->getParam("ZielTemp")->value();
      CONSOLELN(inputMessage);
      SteuerungWebServer::mSettings->setTemp(0,inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("RastZeit")) {
      inputMessage = request->getParam("RastZeit")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setTime(0,inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("Kp")) {
      inputMessage = request->getParam("Kp")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setPidKp(inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("Ki")) {
      inputMessage = request->getParam("Ki")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setPidKi(inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("Kd")) {
      inputMessage = request->getParam("Kd")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setPidKd(inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("pidcycle")) {
      inputMessage = request->getParam("pidcycle")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setPidWindowSize(inputMessage.toInt()*1000);
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("KalM")) {
      inputMessage = request->getParam("KalM")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setKalM(inputMessage.toFloat());
      SteuerungWebServer::mSettings->setShouldSave(true);
  }
  if (request->hasParam("KalT")) {
      inputMessage = request->getParam("KalT")->value();
      CONSOLELN(inputMessage);     
      SteuerungWebServer::mSettings->setKalT(inputMessage.toFloat());
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
  SteuerungWebServer::mSettings->setUseAP(false);
  if (request->hasParam("ApMode")) {
      inputMessage = request->getParam("ApMode")->value();
      CONSOLELN("ApMode");
      CONSOLELN(inputMessage);
      SteuerungWebServer::mSettings->setUseAP(true);
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
String processorTemp(const String& var){
  //Serial.println(var);
  
  if(var == "TEMPERATURE"){
    return String(SteuerungWebServer::mSettings->getActTemp());
  }
  if(var == "ANAUS"){
    if ( SteuerungWebServer::mSettings->getHeatState() ) {
      return String("On");
    } else {
      return String("Off");      
    }
  }
    
  return String();
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
  CONSOLE("\nFilesystem Total KB "); 
  CONSOLE(fileTotalKB);
  CONSOLE(" benutzt KB "); 
  CONSOLELN(fileUsedKB);

  Dir dir = SPIFFS.openDir("/");  // Dir ausgeben
  while (dir.next()) {
    CONSOLE(dir.fileName()); 
    CONSOLE("\t");
    File f = dir.openFile("r");
    CONSOLELN(f.size());
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
// handle File Upload
///////////////////////////////////////////////////////////////////////////
// handles uploads to the filserver
void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
  CONSOLELN(logmessage);

  if (!index) {
    SPIFFS.begin();
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
    //request->redirect("/");
    //request->send(200, "/start.html");
    /*
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "Ok");
    response->addHeader("Test-Header", "My header value");
    request->send(response);
    */
    //request->send(303,"/start.html");
    request->send(200, "text/html", "<a href=\"/\">Return to Home Page</a>");
  }
}
///////////////////////////////////////////////////////////////////////////
// runHandle
///////////////////////////////////////////////////////////////////////////
void runHandle(AsyncWebServerRequest *request) {
  if ( request->hasParam("output") ) {
    String inputMessage = request->getParam("output")->value();
    CONSOLELN(inputMessage);
    
    if ( inputMessage.equals("getdata")) {
      DynamicJsonDocument doc(1024);
      int actRast = SteuerungWebServer::mSettings->getActRast();
      doc["temperature"]     = SteuerungWebServer::mSettings->getActTemp();
      doc["solltemperature"] = SteuerungWebServer::mSettings->getTemp(actRast);
      if ( SteuerungWebServer::mSettings->getTempReached() ) {
        unsigned long sT = SteuerungWebServer::mSettings->getDuration() / 1000;
        if ( sT > 119 )
          sT = sT / 60;
        doc["rastzeit"]        = sT;
       } else {
        doc["rastzeit"]        = SteuerungWebServer::mSettings->getTime(actRast);
      }
      
      if ( SteuerungWebServer::mSettings->getHeatState() ) {
        doc["anaus"]         = String("On");
      } else {
        doc["anaus"]         = String("Off");    
      }

      if ( SteuerungWebServer::mSettings->getStarted() ) {
        doc["started"]       = String("1");
      } else {
        doc["started"]       = String("0");
      }

      if ( SteuerungWebServer::mSettings->getPlaySound() ) {
        doc["playsound"]       = String("1");
      } else {
        doc["playsound"]       = String("0");
      }

      String jsonString;
      serializeJson(doc, jsonString);
      CONSOLELN(jsonString);
      request->send(200, "text/html", jsonString);      
    }    
  }   
}
///////////////////////////////////////////////////////////////////////////
// runGetSwitches
///////////////////////////////////////////////////////////////////////////
void runGetSwitches(AsyncWebServerRequest *request){
  CONSOLELN(F("runState"));
  String inputMessage;
  if (request->hasParam("state"))
  {
      inputMessage = request->getParam("state")->value();
      if ( inputMessage.equals("1")) {
        SteuerungWebServer::mSettings->setShouldStart(true);
      } else {
        SteuerungWebServer::mSettings->setShouldResetState(true);
      }
  } if (request->hasParam("sound")) {
      inputMessage = request->getParam("sound")->value();
      if ( inputMessage.equals("1")) {
        SteuerungWebServer::mSettings->setPlaySound(true);
      } else {
        SteuerungWebServer::mSettings->setPlaySound(false);
      }
  } else {
      inputMessage = "No message sent";
  }
  
  CONSOLELN(inputMessage);
  request->send(200, "text/plain", "OK");
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
    CONSOLELN(F("upload done"));
    request->send(200);    }, handleUpload);
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
  mServer.on("/reset", HTTP_GET, [](AsyncWebServerRequest *request) {
    CONSOLELN(F("reset"));
    SteuerungWebServer::mSettings->reset();
    SteuerungWebServer::mSettings->setShouldSave(true);
    request->send(200, "text/html", "reset done");
  });///////////////////////////////////////////////////////////////////////////
  mServer.on("/run", HTTP_GET,[](AsyncWebServerRequest *request) {
    CONSOLELN(F("run"));
//    request->send(SPIFFS, "/run.html", String(), false, processorTemp);
    request->send(SPIFFS, "/run.html", String(), false);
  });
  mServer.on("/runReadData", HTTP_GET, runHandle);
  mServer.on("/runState", HTTP_GET, runGetSwitches);
  ///////////////////////////////////////////////////////////////////////////
  mServer.onNotFound([](AsyncWebServerRequest *request){
    int fnsstart = request->url().lastIndexOf('/');
    String fn = request->url().substring(fnsstart);
    CONSOLELN(fn);
    if ( SPIFFS.exists(fn) ) {
      CONSOLELN(F("File found"));
      request->send(SPIFFS, fn, String(), false);
    } else {
      CONSOLELN(F("File not found"));
      request->send(200, "text/html", upload_html);
    }
  });  
}
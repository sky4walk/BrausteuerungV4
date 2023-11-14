// brausteuerung@AndreBetz.de
#ifndef __TEMPWEBSERVER__
#define __TEMPWEBSERVER__

#include <ESP8266WebServer.h>
#include <WebSocketsServer_Generic.h>
#include <FS.h>
#include "settings.h"

const char upload_html[] PROGMEM = R"rawliteral(
  <form method="post" enctype="multipart/form-data">
    <input type="file" name="name">
    <input class="button" type="submit" value="Upload">
  </form>
)rawliteral";
 
class TempWebServer {
  public:
    TempWebServer(
      ESP8266WebServer& server,
      Settings& set) :
      mServer(server),
      mWebSocket(WebSocketsServer(81)),
      mSettings(set) {
      mWebSocket.onEvent(std::bind(&TempWebServer::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    }
    String getContentType(String filename) { 
      if (filename.endsWith(".html"))       return "text/html";
      else if (filename.endsWith(".css"))   return "text/css";
      else if (filename.endsWith(".js"))    return "application/javascript";
      else if (filename.endsWith(".ico"))   return "image/x-icon";
      else if (filename.endsWith(".gz"))    return "application/x-gzip";
      else if (filename.endsWith(".json"))  return "text/plain";            
      return "text/plain";
    }
    void begin(){
      SPIFFS.begin();
      mWebSocket.begin();

      // https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html
      mServer.on("/", std::bind(&TempWebServer::handleRoot, this));
      mServer.on("/Setup", std::bind(&TempWebServer::handleSetup, this));
      
      mServer.on("/upload", HTTP_GET, [this]() {
        mServer.send(200, "text/html", upload_html);
      });
      
      mServer.on("/upload", HTTP_POST,[this](){ this->mServer.send(200); }, std::bind(&TempWebServer::handleFileUpload, this));
      mServer.onNotFound(std::bind(&TempWebServer::handleNotFound, this));
    }

    void handleNotFound(){
      if (!handleFileRead(mServer.uri()))                  
        mServer.send(404, "text/plain", F("404: Not found2")); 
    }

    void handleRoot() {
      mServer.send(200, "text/plain", F("Hello world!"));   
    }
    void loop() {
      mServer.handleClient();
      mWebSocket.loop();
    }
    void handleSetup() {
//      mServer.send(200, "text/html", setup_html);   
    }
 
    bool handleFileRead(String path) {
      CONSOLELN("handleFileRead: " + path);
      if (path.endsWith("/")) path += "index.html";          
      String contentType = getContentType(path);             
      String pathWithGz = path + ".gz";
      if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { 
        if (SPIFFS.exists(pathWithGz))                         
          path += ".gz";                                         
        File file = SPIFFS.open(path, "r");                    
        size_t sent = mServer.streamFile(file, contentType);    
        file.close();                                          
        CONSOLELN(String("\tSent file: ") + path);
        return true;
      }
      CONSOLELN(String("\tFile Not Found: ") + path);
      return false;
    }

    void handleFileUpload(){ 
      HTTPUpload& upload = mServer.upload();
      if(upload.status == UPLOAD_FILE_START){
        String filename = upload.filename;
        if(!filename.startsWith("/")) filename = "/"+filename;
        CONSOLE(F("loadName: ")); 
        CONSOLELN(filename);
        mFsUploadFile = SPIFFS.open(filename, "w");            
        filename = String();
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(mFsUploadFile)
          mFsUploadFile.write(upload.buf, upload.currentSize); 
      } else if(upload.status == UPLOAD_FILE_END){
        if(mFsUploadFile) {                                    
          mFsUploadFile.close();                               
          CONSOLE(F("loadSize:")); 
          CONSOLELN(upload.totalSize);
          mServer.sendHeader("Location","/success.html");      
          mServer.send(303);
        } else {
          mServer.send(500, "text/plain", F("500: couldn't create file"));
        }
      }
    }
  
    void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) { 
      switch (type) {
        case WStype_DISCONNECTED:             
          CONSOLE(F("Discon:"));
          CONSOLELN(String(num));
          break;
        case WStype_CONNECTED: {              
            IPAddress ip = mWebSocket.remoteIP(num);
            CONSOLE(F("Con:"));
            CONSOLE(String(num));
            Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
          }
          break;
        case WStype_TEXT:
          CONSOLE(F("Text:"));
          CONSOLE(num);
          Serial.printf("[%u] url: %s\n", num, payload);
          break;
      }
    }
    
  private:
    ESP8266WebServer& mServer;
    WebSocketsServer mWebSocket;
    Settings& mSettings;
    File mFsUploadFile;
};

#endif

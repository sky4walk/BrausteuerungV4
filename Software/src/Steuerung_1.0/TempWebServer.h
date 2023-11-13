// brausteuerung@AndreBetz.de
#ifndef __TEMPWEBSERVER__
#define __TEMPWEBSERVER__

#include <ESP8266WebServer.h>
#include <FS.h>
#include "settings.h"

    const char setup_html[] PROGMEM = R"rawliteral(
      <form action="login" method="POST">
        <input type="text" name="username" placeholder="Username"></br>
        <input type="password" name="password" placeholder="Password"></br>
        <input type="submit" value="Login">
      </form>
      <p>Try 'John Doe' and 'password123' ...</p>
    )rawliteral";      


 

class TempWebServer {
  public:
    TempWebServer(
      ESP8266WebServer& server,
      Settings& set) :
      mServer(server),
      mSettings(set) {
    }
    void begin(){
      // https://tttapa.github.io/ESP8266/Chap12%20-%20Uploading%20to%20Server.html
      mServer.on("/", std::bind(&TempWebServer::handleRoot, this));
      mServer.on("/Setup", std::bind(&TempWebServer::handleSetup, this));
      
      mServer.on("/upload", HTTP_GET, [this]() {
//        if (!std::bind(&TempWebServer::handleFileRead("/upload.html"), this)) // if (!handleFileRead("/upload.html"))                
          this->mServer.send(404, "text/plain", F("404: Not Found"));
      });
      
      mServer.on("/upload", HTTP_POST,[this](){ this->mServer.send(200); }, std::bind(&TempWebServer::handleFileUpload, this));
      mServer.onNotFound(std::bind(&TempWebServer::handleNotFound, this));
    }
    void handleNotFound(){
      if (!handleFileRead(mServer.uri()))                  
        mServer.send(404, "text/plain", F("404: Not found")); 
    }

    void handleRoot() {
      mServer.send(200, "text/plain", F("Hello world!"));   
    }

    void handleSetup() {
      mServer.send(200, "text/html", setup_html);   
    }
 
    String getContentType(String filename) { 
      if (filename.endsWith(".html")) return "text/html";
      else if (filename.endsWith(".css")) return "text/css";
      else if (filename.endsWith(".js")) return "application/javascript";
      else if (filename.endsWith(".ico")) return "image/x-icon";
      else if (filename.endsWith(".gz")) return "application/x-gzip";
      return "text/plain";
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
      CONSOLE("handleFileUpload Name: "); 
      CONSOLELN(filename);
      mFsUploadFile = SPIFFS.open(filename, "w");            
      filename = String();
    } else if(upload.status == UPLOAD_FILE_WRITE){
      if(mFsUploadFile)
        mFsUploadFile.write(upload.buf, upload.currentSize); 
    } else if(upload.status == UPLOAD_FILE_END){
      if(mFsUploadFile) {                                    
        mFsUploadFile.close();                               
        CONSOLE("handleFileUpload Size: "); 
        CONSOLELN(upload.totalSize);
        mServer.sendHeader("Location","/success.html");      
        mServer.send(303);
      } else {
        mServer.send(500, "text/plain", F("500: couldn't create file"));
      }
    }
  }  
  private:
    ESP8266WebServer& mServer;
    Settings& mSettings;
    File mFsUploadFile;
};

#endif

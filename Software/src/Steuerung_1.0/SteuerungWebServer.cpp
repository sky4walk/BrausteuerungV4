// brausteuerung@AndreBetz.de
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include "SteuerungWebServer.h"
#include "DbgConsole.h"

static AsyncWebServer mServer(80);

const char upload_html[] PROGMEM = R"rawliteral(
 
<!DOCTYPE html>
<html>
  <head>
    <title>File Upload</title>
  </head>
  <body>
    <h1>File Upload</h1>
    <form method="POST" action="/uploadDo" enctype="multipart/form-data" target="iframe">
    <input type="file" name="upload"><input type="submit" value="Upload"></form>
    <iframe style="visibility: hidden;" src="http://" )+local_IPstr+"/Usm" name="iframe"></iframe>
  </body>
</html>
)rawliteral";
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
  message += mServer.uri();
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
SteuerungWebServer::SteuerungWebServer(Settings& set) :
  mSettings(set) {
}

void SteuerungWebServer::begin() {
  SPIFFS.begin();
  mServer.begin();

  ///////////////////////////////////////////////////////////////////////////
  // Root
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(SPIFFS, "/start.html", String(), false);
  });
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/uploadDo", HTTP_POST, [](AsyncWebServerRequest *request) {
    request->send(200); }, handleUpload);       
  mServer.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", upload_html);
  });
  mServer.onFileUpload(handleUpload);
  ///////////////////////////////////////////////////////////////////////////
  mServer.on("/download", HTTP_GET, [](AsyncWebServerRequest *request) {
    //request->send(SPIFFS, "/settings.json", String(), true);
    AsyncWebServerResponse *response = request->beginResponse(SPIFFS, "/settings.json", String(), true);
    response->addHeader("Server", "ESP Async Web Server");
    request->send(response);
  });       
  ///////////////////////////////////////////////////////////////////////////
  mServer.onNotFound([](AsyncWebServerRequest *request){
    int fnsstart = request->url().lastIndexOf('/');
    String fn = request->url().substring(fnsstart);
    CONSOLELN(fn);
    request->send(SPIFFS, fn, String(), true);
  });  
}
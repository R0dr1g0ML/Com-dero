#include "webLogic.h"
#include "vars.h"
#include <WiFi.h>
#include "webpages.h"
#include "time.h"
#include <Preferences.h>
#include <SPIFFS.h>
#include <AccelStepper.h>
#include <ArduinoJson.h>

AccelStepper stepper(AccelStepper::HALF4WIRE, IN1, IN3, IN2, IN4);

WebServer server(80);
String currentTZ = "UTC0";


String renderConfigPage(String statusMsg) {
  String page = CONFIG_page;

  String ssid = prefs.getString("ssid", "");
  String password = prefs.getString("password", "");
  String mode = prefs.getString("mode", "AP");
  String ip = prefs.getString("ip", "");
  String gateway = prefs.getString("gateway", "");
  String tz = prefs.getString("timezone", "0");

  page.replace("%STATUS%", statusMsg);

  page.replace("%SSID%", ssid);
  page.replace("%PASSWORD%", password);
  page.replace("%IP%", ip);
  page.replace("%GATEWAY%", gateway);

  page.replace("%AP_CHECKED%", mode == "AP" ? "checked" : "");
  page.replace("%STA_CHECKED%", mode == "STA" ? "checked" : "");

  // TZ
  page.replace("%TZ_-5%", tz == "-5" ? "selected" : "");
  page.replace("%TZ_-4%", tz == "-4" ? "selected" : "");
  page.replace("%TZ_-3%", tz == "-3" ? "selected" : "");
  page.replace("%TZ_0%",  tz == "0"  ? "selected" : "");
  page.replace("%TZ_1%",  tz == "1"  ? "selected" : "");

  return page;
}

// ----------- Zonas disponibles -----------
struct TZOption {
  const char* name;
  const char* tz;
};

TZOption tzList[] = {
  {"UTC", "UTC0"},
  {"Europa/Madrid", "CET-1CEST,M3.5.0,M10.5.0/3"},
  {"America/New_York", "EST5EDT,M3.2.0,M11.1.0"},
  {"America/Bogota", "COT5"},
  {"Asia/Tokyo", "JST-9"}
};
const int tzCount = sizeof(tzList) / sizeof(tzList[0]);

// ----------- Aplicar TZ -----------
void applyTZ(String tz) {
  setenv("TZ", tz.c_str(), 1);
  tzset();
  configTime(0, 0, ntpServer);
}

// ----------- API JSON -----------
void handleTimeAPI() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    server.send(500, "application/json", "{\"error\":true}");
    return;
  }

  char buffer[64];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);

  String json = "{\"time\":\"" + String(buffer) + "\"}";
  server.send(200, "application/json", json);
}


String currentTime = "--:--:--";

// ----------- Handlers -----------

void handleRoot() {
  serveIndex();
}
void handleUploadForm() {
  server.send(200, "text/html", R"rawliteral(
    <!DOCTYPE html>
    <html>
    <head><title>Subir archivo</title></head>
    <body>
      <h2>Subir index</h2>
      <form method="POST" action="/upload" enctype="multipart/form-data">
        <input type="file" name="file">
        <input type="submit" value="Subir">
      </form>
      <a href="/">Volver al inicio</a>
    </body>
    </html>
  )rawliteral");
}

// guardar confi wifi y reiniciar
void handleSave() {
  String mode = server.arg("mode");
  String ssid = server.arg("ssid");
  String password = server.arg("password");
  String ip = server.arg("ip");
  String gateway = server.arg("gateway");
  String tz = server.arg("timezone");

  prefs.putString("mode", mode);
  prefs.putString("ssid", ssid);
  prefs.putString("password", password);
  prefs.putString("ip", ip);
  prefs.putString("gateway", gateway);
  prefs.putString("timezone", tz);

  server.send(200, "text/html", renderConfigPage("Configuración guardada. Reiniciando..."));
  delay(1500);
  ESP.restart();
}

void handleConfig() { server.send(200, "text/html", renderConfigPage()); }

//void handleConfig() {  server.send(200, "text/html", renderConfigPage()); }

void handleSetTZ() {
  if (server.hasArg("tz")) {
    currentTZ = server.arg("tz");

    prefs.begin("config", false);
    prefs.putString("tz", currentTZ);
    prefs.end();

    applyTZ(currentTZ);
  }

  server.sendHeader("Location", "/");
  server.send(303);
}


void webInit() {
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");

  
  // Cargar TZ guardada
  prefs.begin("config", true);
  currentTZ = prefs.getString("tz", "UTC0");
  prefs.end();
  applyTZ(currentTZ);
  configTime(0, 0, ntpServer);

  // Rutas
    server.on("/mover_manual", HTTP_GET, handleMoverManual);
    server.on("/sonido", HTTP_GET, handleReproducirSonido);
    server.on("/eventos.json", HTTP_GET, handleListarEventos);
    server.on("/eliminar", HTTP_GET, handleEliminarEvento);
    server.on("/list", HTTP_GET, handleListFiles);
    server.on("/move", HTTP_GET, handleMove);
    server.on("/guardar", HTTP_POST, handleGuardar);
  server.on("/", HTTP_GET, serveIndex);
  server.on("/config", handleConfig);
  server.on("/setTZ", handleSetTZ);
  server.on("/api/time", handleTimeAPI);
  server.on("/upload", HTTP_GET, handleUploadForm);
  server.on("/upload", HTTP_POST, []() {server.send(200, "text/plain", "Archivo subido");}, handleUpload);
  server.on("/upload_file", HTTP_POST, [](){ server.send(200); }, handleFileUpload);
  //server.onNotFound(handleNotFound);

  server.begin();
}

// --------------------- Manejo de WiFi ---------------------
void setupWiFi() {
  String mode = prefs.getString("mode", "AP");
  String ssid = prefs.getString("ssid", defaultAP_SSID);
  String password = prefs.getString("password", defaultAP_PASS);

  if (mode == "STA") {
    String ipStr = prefs.getString("ip", "");
    String gatewayStr = prefs.getString("gateway", "");

    WiFi.mode(WIFI_STA);

    if (ipStr.length() > 0 && gatewayStr.length() > 0) {
      IPAddress localIP, gateway, subnet(255, 255, 255, 0);
      localIP.fromString(ipStr);
      gateway.fromString(gatewayStr);
      WiFi.config(localIP, gateway, subnet);
    }

    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.println("Conectando en modo STA...");

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
      delay(500);
      Serial.print('.');
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConectado a STA con IP: " + WiFi.localIP().toString());
      return;
    }

    Serial.println("\nFallo en STA. Cambiando a AP...");
  }

  // Si STA falla o se seleccionó AP
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apGateway, apNetmask);
  WiFi.softAP(prefs.getString("ssid", defaultAP_SSID).c_str(), prefs.getString("password", defaultAP_PASS).c_str());
  Serial.println("Modo AP activo. IP: " + WiFi.softAPIP().toString());
}

void handleNotFound() { server.send(404, "text/plain", "Ruta no encontrada."); }

// --------------------- Archivos y servidor ---------------------
void serveIndex() {
  File file = SPIFFS.open("/index.html", "r");

  if (!file) {
    server.send(404, "text/plain", "No hay index.html");
    return;
  }

  server.streamFile(file, "text/html");
  file.close();
}

// subir html
// void handleUpload() {
//   HTTPUpload& upload = server.upload();

//   if (upload.status == UPLOAD_FILE_START) {
//     String filename = upload.filename;
//     if (!filename.startsWith("/")) {
//       filename = "/" + filename;
//     }
//     Serial.println("Subiendo: " + filename);
//     uploadFile = SPIFFS.open(filename, "w");
//   }  else if (upload.status == UPLOAD_FILE_WRITE) {
//     if (uploadFile) {
//       uploadFile.write(upload.buf, upload.currentSize);
//     }
//   }  else if (upload.status == UPLOAD_FILE_END) {
//     if (uploadFile) {
//       uploadFile.close();
//       Serial.println("Archivo subido correctamente");
//     }
//   }
// }
void handleUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Subiendo archivo: %s\n", upload.filename.c_str());
    if (SPIFFS.exists("/index.html")) SPIFFS.remove("/index.html");
    File file = SPIFFS.open("/index.html", FILE_WRITE);
    file.close();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    File file = SPIFFS.open("/index.html", FILE_APPEND);
    if (file) {
      file.write(upload.buf, upload.currentSize);
      file.close();
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    Serial.println("Subida completa.");
    server.send(200, "text/html", renderConfigPage("Archivo subido correctamente."));
  }
}

// String htmlConfigPage(const String& statusMsg) {
//   return "<p><b>" + statusMsg + "</b></p>";
// }

void handleFileUpload() {
  HTTPUpload& upload = server.upload();

  if (upload.status == UPLOAD_FILE_START) {
    String filename = upload.filename;
    if (!filename.startsWith("/")) filename = "/" + filename;
    Serial.print("📂 Abriendo archivo para escritura: ");
    Serial.println(filename);
    uploadFile = SPIFFS.open(filename, FILE_WRITE);
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    if (uploadFile) {
      uploadFile.close();
      Serial.println("✅ Archivo subido correctamente");
      server.send(200, "text/plain", "Archivo subido correctamente");
    } else {
      server.send(500, "text/plain", "Error al escribir archivo");
    }
  }
}

// --------------------- Eventos ---------------------
void cargarEventos() {
  eventos.clear();

  if (!SPIFFS.exists("/config.json")) return;
  File file = SPIFFS.open("/config.json", "r");
  if (!file) return;

  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return;

  if (!doc.containsKey("eventos")) return;
  for (JsonObject obj : doc["eventos"].as<JsonArray>()) {
    Evento e;
    e.hora = obj["hora"];
    e.minuto = obj["minuto"];
    e.pasos = obj["gramos"];
    eventos.push_back(e);
  }
}

void guardarEventos() {
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.createNestedArray("eventos");
  for (auto e : eventos) {
    JsonObject obj = arr.createNestedObject();
    obj["hora"] = e.hora;
    obj["minuto"] = e.minuto;
    obj["pasos"] = e.pasos;
  }
  File file = SPIFFS.open("/config.json", FILE_WRITE);
  if (file) {
    serializeJson(doc, file);
    file.close();
  }
}

void revisarEventos() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) return;

  for (size_t i = 0; i < eventos.size(); ++i) {
    Evento &e = eventos[i];
    if (e.hora == timeinfo.tm_hour && e.minuto == timeinfo.tm_min) {
      moverMotor(e.pasos);
    }
  }
}

void handleListarEventos() {
  DynamicJsonDocument doc(1024);
  JsonArray arr = doc.to<JsonArray>();
  for (auto e : eventos) {
    JsonObject obj = arr.createNestedObject();
    obj["hora"] = e.hora;
    obj["minuto"] = e.minuto;
    obj["pasos"] = e.pasos;
  }

  String json;
  serializeJson(arr, json);
  server.send(200, "application/json", json);
}

void handleEliminarEvento() {
  if (server.hasArg("id")) {
    int id = server.arg("id").toInt();
    if (id >= 0 && id < (int)eventos.size()) {
      eventos.erase(eventos.begin() + id);
      //lastExecMin.erase(lastExecMin.begin() + id);
      guardarEventos();
    }
  }
  server.send(200, "text/plain", "OK");
}

void handleListFiles() {
  String output = "<h2>Archivos en SPIFFS:</h2><ul>";
  File root = SPIFFS.open("/");
  File file = root.openNextFile();
  while (file) {
    output += "<li>" + String(file.name()) + " (" + String(file.size()) + " bytes)</li>";
    file = root.openNextFile();
  }
  output += "</ul><a href=\"/\">Volver</a>";
  server.send(200, "text/html", output);
}

// --------------------- Motor ---------------------
void handleMoverManual() {
  if (server.hasArg("pasos")) {
    int pasos = server.arg("pasos").toInt();
    moverMotor(pasos);
  }
  server.send(200, "text/plain", "Motor movido");
}

void handleMove() {
  moverMotor(1000);
  server.send(200, "text/plain", "Motor movido");
}

void handleGuardar() {
  DynamicJsonDocument doc(256);
  DeserializationError err = deserializeJson(doc, server.arg("plain"));
  if (err) {
    server.send(400, "text/plain", "JSON inválido");
    return;
  }

  Evento e;
  e.hora = doc["hora"];
  e.minuto = doc["minuto"];
  e.pasos = doc["pasos"];
  eventos.push_back(e);
  //lastExecMin.push_back(-1);
  guardarEventos();

  server.send(200, "text/plain", "Guardado");
}

void setupMotor() {
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stepper.setMaxSpeed(700);
  stepper.setSpeed(700);
  stepper.setAcceleration(300);
  desactivarMotor();
}

void moverMotor(int pasos) {
  stepper.move(pasos);
  while (stepper.distanceToGo() != 0) {
    stepper.run();
  }
  desactivarMotor();
}

void desactivarMotor() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void handleReproducirSonido() {
  playMouseChirp();
  server.send(200, "text/plain", "Sonido reproducido");
}

// --------------------- Sonidos (usando LEDC) ---------------------
// Usaremos el canal 0 para tono
//const int SPEAKER_LEDC_CHANNEL = 0;

void playRobotHello() {
  int tones[] = {300, 600, 400, 700};
  int durations[] = {150, 200, 150, 200};
  for (int i = 0; i < 4; i++) {
    //ledcWriteTone(SPEAKER_LEDC_CHANNEL, tones[i]);
    //delay(durations[i]);
    //ledcWriteTone(SPEAKER_LEDC_CHANNEL, 0);
    //delay(10);
    pulseIn(SPEAKER_PIN, tones[i], 20);
  }
}

void playMouseChirp() {
  const int chirpDuration = 300;
  const int steps = 15;
  const int delayPerStep = chirpDuration / steps;
  int startFreq = 1000;
  int endFreq = 3500;

  for (int i = 0; i < steps; i++) {
    int freq = startFreq + (endFreq - startFreq) * i / steps;
    //ledcWriteTone(SPEAKER_LEDC_CHANNEL, freq);
    //delay(delayPerStep);
    pulseIn(SPEAKER_PIN, freq, 20);
  }
  //ledcWriteTone(SPEAKER_LEDC_CHANNEL, 0);
}

// boton reset
void resetTotal() {
  Serial.println("RESET TOTAL: Borrando index.html y preferencias...");

  // Borrar archivo index.html
  if (SPIFFS.exists("/index.html")) {
    SPIFFS.remove("/index.html");
    Serial.println("Archivo index.html borrado.");
  } else {
    Serial.println("index.html no existía.");
  }

  // Borrar preferencias
  prefs.clear();
  Serial.println("Preferencias borradas.");

  delay(500);
  ESP.restart();
}

void webLoop() {
  server.handleClient();
}
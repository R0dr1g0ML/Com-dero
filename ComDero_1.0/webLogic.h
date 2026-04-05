#pragma once
#include <WebServer.h>

void webInit();
void webLoop();
void handleSetTZ();
void applyTZ(String tz);
void handleTimeAPI();
void handleRoot();
void handleConfig();
void handleFileUpload();
void handleUpload();
void serveIndex();
void resetTotal();
void revisarEventos();
void handleMoverManual();
void playMouseChirp();
void playRobotHello();
void handleReproducirSonido();
void handleListFiles();
void handleMoverManual();
void setupMotor();
void moverMotor(int pasos);
void mover_manual();
void handleEliminarEvento();
void handleGuardar();
void handleMove();
void desactivarMotor();
void handleSave();
void handleListarEventos();
String renderConfigPage(String statusMsg = "");


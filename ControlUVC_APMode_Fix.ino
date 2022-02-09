#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <FS.h>
#include <ArduinoJson.h>

#include <ESP8266WebServer.h>
ESP8266WebServer server(80);

#include <WebSocketsServer.h>
WebSocketsServer webSocket = WebSocketsServer(81);

#include <Ticker.h>
Ticker tickerCiclos;
Ticker tickerFecha;

#include <Wire.h>
#include "RTClib.h"
RTC_DS3231 rtc;

#include "ControlUVC.h"
ControlUVC controlUVC;

#include <Nextion.h>
#include <NextionPage.h>
#include <NextionButton.h>
#include <NextionText.h>
#include <NextionNumber.h>
#include <NextionPicture.h>

Nextion nex(Serial);

NextionPage             pageCiclos(nex, 2, 0, "ciclos");
NextionButton           btnHoras(nex, 2, 1, "btnHoras");
NextionButton           btnConfig(nex, 2, 2, "btnConfig");
NextionText             txtCiclos(nex, 2, 3, "txtCiclos");
NextionPicture          picRun(nex, 2, 4, "picRun");//5 = off, 6 = on
NextionPicture          picVol(nex, 2, 5, "picVol");//7 = off, 8 = on
NextionText             txtVolumen(nex, 2, 6, "txtVolumen");
NextionText             txtFecha(nex, 2, 7, "txtFecha");

NextionPage             pageHoras(nex, 3, 0, "horas");
NextionButton           btnCiclos(nex, 3, 1, "btnCiclos");
NextionButton           btnConfig1(nex, 3, 2, "btnConfig");
NextionNumber           txtHoras(nex, 3, 3, "txtHoras");
NextionNumber           txtMinutos(nex, 3, 4, "txtMinutos");
NextionNumber           txtSegundos(nex, 3, 5, "txtSegundos");
NextionButton           btnReset(nex, 3, 6, "btnReset");
NextionText             txtEquipo(nex, 3, 7, "txtEquipo");

NextionPage             pageConfig(nex, 4, 0, "config");
NextionButton           btnCiclos1(nex, 4, 1, "btnCiclos");
NextionButton           btnHoras1(nex, 4, 2, "btnHoras");
NextionText             txtLargo(nex, 4, 3, "txtLargo");
NextionText             txtAncho(nex, 4, 4, "txtAncho");
NextionText             txtAlto(nex, 4, 5, "txtAlto");
NextionText             txtTotal(nex, 4, 6, "txtTotal");
NextionButton           btnCalcular(nex, 4, 7, "btnCalcular");
NextionButton           btnGuardar(nex, 4, 8, "btnGuardar");

NextionPage             pageAlert(nex, 5, 0, "alert");
NextionButton           btnCancelar(nex, 5, 1, "btnCancelar");
NextionButton           btnAceptar(nex, 5, 2, "btnAceptar");

NextionPage             pageHardReset(nex, 6, 0, "hardReset");
NextionButton           btnOk(nex, 6, 3, "btnOk");

NextionPage             pageDateTime(nex, 7, 0, "dateTime");
NextionNumber           txtDD(nex, 7, 1, "txtDD");
NextionNumber           txtMM(nex, 7, 2, "txtMM");
NextionNumber           txtAA(nex, 7, 3, "txtAA");
NextionNumber           txthh(nex, 7, 4, "txthh");
NextionNumber           txtmm(nex, 7, 5, "txtmm");
NextionNumber           txtss(nex, 7, 6, "txtss");
NextionButton           btnCancelar_7(nex, 7, 7, "btnCancelar");
NextionButton           btnAceptar_7(nex, 7, 8, "btnAceptar");

NextionPage             pageChange(nex, 8, 0, "change");
NextionButton           btnAceptar_8(nex, 8, 1, "btnAceptar");

#include "index_html.h"

#ifndef APSSID
#define APSSID "Ventilador-11"
#define APPSK  "#123456#"
#endif

const char    *ssid_ap = APSSID;
const char    *password_ap = APPSK;
const char    *host_name = APSSID;

const String  chipId = "ESPUVC-" + String(ESP.getChipId(), HEX);

String webpage = "";

#define relayB          13  //D7
#define relayA          12  //D6
#define interruptorA    14  //D5

bool flagRun = false;

uint8_t nextionPage = 2;

uint8_t switchState;
uint8_t lastSwitchState = LOW;

unsigned long lastDebounceTime = 0;
unsigned long debounceDelay = 50;

DateTime tiempoInicial;
DateTime tiempoFinal;

#define countof(a) (sizeof(a) / sizeof(a[0]))

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED: {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
        handleFileList();
        String output = "{\"type\":\"cycle\",\"ciclos\":\"0.0\",\"usoUVC\":\"";
        output += unixtimeToString(controlUVC.getTuboUVC());
        output += "\"}";
        webSocket.broadcastTXT(output);
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[%u] get Text: %s\n", num, payload);
        String res = (char*)payload;
        float volumen = res.toFloat();
        if (volumen > 0) controlUVC.setVolumen(volumen);
        actualizarPicVolumen();
        // echo data back to browser
        //webSocket.sendTXT(num, payload, length);
        // send data to all connected clients
        //webSocket.broadcastTXT(payload, length);
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      hexdump(payload, length);
      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      break;
    case WStype_ERROR: {
      }
      break;
    default:
      break;
  }
}

bool csvContentType(String filename) {
  return (filename.endsWith(".csv")) ? true : false;
}

String formatBytes(size_t bytes) {
  if (bytes < 1024) {
    return String(bytes) + "B";
  } else if (bytes < (1024 * 1024)) {
    return String(bytes / 1024.0) + "KB";
  } else if (bytes < (1024 * 1024 * 1024)) {
    return String(bytes / 1024.0 / 1024.0) + "MB";
  } else {
    return String(bytes / 1024.0 / 1024.0 / 1024.0) + "GB";
  }
}

void handleRoot() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(200, "text/html", INDEX_HTML);
}

void handleNotFound() {
  String message = F("File Not Found\n\n");
  message += F("URI: ");
  message += server.uri();
  message += F("\nMethod: ");
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += F("\nArguments: ");
  message += server.args();
  message += F("\n");

  for (uint8_t i = 0; i < server.args(); i++) {
    message += String(F(" ")) + server.argName(i) + F(": ") + server.arg(i) + F("\n");
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send(404, "text/plain", message);
}

void handleWiFiSetup() {
  String data = server.arg("plain");

  Serial.println(data);
  DynamicJsonBuffer jBuffer;
  JsonObject& jObject = jBuffer.parseObject(data);
  jObject.prettyPrintTo(Serial);

  File configFile = SPIFFS.open("/config.json", "w");
  jObject.printTo(configFile);
  configFile.close();

  server.send(200, "text/plain", "...reiniciando");
  delay(1000);

  ESP.restart();
}

void handleVolumeSetup() {
  String data = server.arg("plain");
  controlUVC.setVolumen(data.toFloat());
  actualizarPicVolumen();
}

void handleFileList() {
  Dir dir = SPIFFS.openDir("/");
  String output;
  output += "{\"type\":\"file";
  output += "\",\"data\":";
  String outputData = "[";
  while (dir.next()) {
    String fileName = dir.fileName();
    //String path = String(entry.name()).substring(1);
    size_t fileSize = dir.fileSize();
    if (csvContentType(fileName)) {
      if (outputData != "[") {
        outputData += ',';
      }
      outputData += "{\"name\":\"";
      outputData += fileName.c_str();
      outputData += "\",\"size\":\"";
      outputData += formatBytes(fileSize).c_str();
      outputData += "\"}";
    }
  }
  outputData += "]";
  output += outputData + "}";
  //Serial.printf("fileList JSON: %s\r\n", output.c_str());
  webSocket.broadcastTXT(output);
}

void handleFileDownload() {
  if (server.args() > 0 ) {
    Serial.println(server.arg(0));
    File file = SPIFFS.open(server.arg(0), "r");
    server.sendHeader("Content-Type", "text/csv");
    server.sendHeader("Content-Disposition", "attachment; filename=" + server.arg(0));
    server.sendHeader("Connection", "close");
    server.streamFile(file, "application/octet-stream");
    file.close();
  }
}

void wifiConnect() {
  WiFi.softAPdisconnect(true);
  WiFi.disconnect();
  delay(1000);
  if (SPIFFS.exists("/config.json")) {
    const char * _ssid = "", *_pass = "";
    File configFile = SPIFFS.open("/config.json", "r");
    if (configFile) {
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();
      DynamicJsonBuffer jsonBuffer;
      JsonObject& jObject = jsonBuffer.parseObject(buf.get());
      jObject.prettyPrintTo(Serial);
      if (jObject.success()) {
        _ssid = jObject["ssid"];
        _pass = jObject["password"];
        WiFi.mode(WIFI_STA);
        WiFi.hostname(host_name);
        //WiFi.config(ip, gateway, subnet);
        WiFi.begin(_ssid, _pass);
        unsigned long startTime = millis();
        while (WiFi.waitForConnectResult() != WL_CONNECTED) {
          delay(500);
          Serial.print(".");
          if ((unsigned long)(millis() - startTime) >= 5000) break;
        }
      }
    }
  }
  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BUILTIN, !HIGH);
  } else {
    WiFi.mode(WIFI_AP);
    //WiFi.softAPConfig(local_ip, gateway, netmask);
    digitalWrite(LED_BUILTIN, !LOW);
    WiFi.softAP(ssid_ap, password_ap);
  }
  Serial.print("\r\n------------------------------\r\n");
  WiFi.printDiag(Serial);
  Serial.print("\r\nWiFi connected\r\nMAC address: ");
  Serial.print(WiFi.macAddress());
  Serial.print("\r\nSTA IP address: ");
  Serial.print(WiFi.localIP());
  Serial.print("\r\nAP IP address: ");
  Serial.print(WiFi.softAPIP());
  Serial.print("\r\nESP ChipID: ");
  Serial.println(chipId);
}

void actualizarFecha() {
  if (nextionPage == 2) {
    DateTime now = rtc.now();
    char buffer[32];
    snprintf_P(buffer, countof(buffer), PSTR(" %02u/%02u/%04u - %02u:%02u"),
               now.day(), now.month(), now.year(), now.hour(), now.minute());
    txtFecha.setText(buffer);
    txtFecha.setText(buffer);
  }
}

String getFileName(DateTime dateTime) {
  String fileName = "/";
  fileName += String(dateTime.month(), DEC);
  fileName += String(dateTime.year(), DEC);
  fileName += ".csv";
  return fileName;
}

void actualizarBitacora(DateTime horaInicio, DateTime horaFinal, float volumenReferencia,
                        double ciclosSesion, double volumenSesion,
                        uint32_t tiempoSesion, uint32_t tuboUVC) {

  char _horaInicio[32];
  snprintf_P(_horaInicio, countof(_horaInicio), PSTR("%02u/%02u/%04u-%02u:%02u:%02u"),
             horaInicio.day(), horaInicio.month(), horaInicio.year(),
             horaInicio.hour(), horaInicio.minute(), horaInicio.second());

  char _horaFinal[32];
  snprintf_P(_horaFinal, countof(_horaFinal), PSTR("%02u/%02u/%04u-%02u:%02u:%02u"),
             horaFinal.day(), horaFinal.month(), horaFinal.year(),
             horaFinal.hour(), horaFinal.minute(), horaFinal.second());

  //Serial.printf("\r\nActualizar bitácora: \r\n");
  String nombreArchivo = getFileName(horaFinal);
  File myFile = SPIFFS.open(nombreArchivo, "a+");

  if (myFile) {
    String line = myFile.readStringUntil('\n');
    if (line == "") {
      myFile.println("hora inicio,hora final,ambiente m3,ciclos,volumen,tiempo sesion,hrs tubo uvc");
    }

    myFile.print(_horaInicio);
    myFile.print(",");
    myFile.print(_horaFinal);
    myFile.print(",");
    myFile.print(volumenReferencia);
    myFile.print(",");
    myFile.print(ciclosSesion, 3);
    myFile.print(",");
    myFile.print(volumenSesion, 3);
    myFile.print(",");
    myFile.print(unixtimeToString(tiempoSesion));
    myFile.print(",");
    myFile.println(unixtimeToString(tuboUVC));
    myFile.close();
  }

  handleFileList();
  actualizarCiclos();
}

String unixtimeToString(uint32_t _unixtime) {
  uint32_t tiempo = _unixtime;
  uint32_t horas, minutos, segundos;

  horas = tiempo / 3600;
  tiempo %= 3600;
  minutos = tiempo / 60;
  tiempo %= 60;
  segundos = tiempo;

  char buffer[16];
  snprintf_P(buffer, countof(buffer), PSTR("%02u:%02u:%02u"), horas, minutos, segundos);

  //String salida = String(buffer);
  //Serial.printf("\r\nunix to String: %s \r\n", salida.c_str());

  return buffer;
}

void actualizarCiclos() {
  DateTime now = rtc.now();

  char buffer[10];
  dtostrf(controlUVC.getCiclos(now.unixtime() - tiempoInicial.unixtime()), 6, 3, buffer);
  txtCiclos.setText(buffer);

  String output = "{";
  output += "\"type\":\"cycle";
  output += "\",\"ciclos\":\"";
  output += String(controlUVC.getCiclos());
  output += "\",\"usoUVC\":\"";
  output += unixtimeToString(controlUVC.getTuboUVC());
  output += "\"}";

  webSocket.broadcastTXT(output);
}

void controlReles(bool flag) {
  digitalWrite(relayA, !flag);
  digitalWrite(relayB, !flag);
  actualizarPicRun(!flag);
}

void controlInterruptor() {
  if (controlUVC.getVolumen() <= 0) {
    return;
  }

  int lecturaInterruptor = !digitalRead(interruptorA);

  if (lecturaInterruptor != lastSwitchState) {
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {

    if (lecturaInterruptor != switchState) {
      switchState = lecturaInterruptor;
      //Serial.printf("Switch: %i\r\n", switchState);

      if (switchState == HIGH) {
        tiempoInicial = rtc.now();
        //tiempoInicial = DateTime(2020, 12, 3, 4, 25, 60);

        if (!tiempoInicial.isValid()) return;

        flagRun = true;
        Serial.println("\r\nActivar relés");
        controlReles(true);
        tickerCiclos.attach(1, actualizarCiclos);

      } else {
        tiempoFinal = rtc.now();
        //tiempoFinal = DateTime(2050, 12, 31, 23, 59, 59);

        if (!tiempoFinal.isValid()) tiempoFinal = tiempoInicial;

        flagRun = false;
        Serial.println("\r\nDesactivar relés");
        controlReles(false);
        tickerCiclos.detach();

        uint32_t diferencia = tiempoFinal.unixtime() - tiempoInicial.unixtime();
        uint32_t lecturaTuboUVCAnterior = controlUVC.getTuboUVC();
        uint32_t lecturaTuboUVCActual = lecturaTuboUVCAnterior + diferencia;
        double volumenSesion = controlUVC.getCiclos() * controlUVC.getVolumen();

        /*Serial.printf("\r\ntiempoInicial: %u\r\n", tiempoInicial.unixtime());
          Serial.printf("tiempoFinal: %u\r\n", tiempoFinal.unixtime());
          Serial.printf("Diferencia: %u\r\n", diferencia);
          Serial.printf("lecturaTuboUVCAnterior: %u\r\n", lecturaTuboUVCAnterior);
          Serial.printf("lecturaTuboUVCActual: %u\r\n", lecturaTuboUVCActual);*/
        controlUVC.setTuboUVC(lecturaTuboUVCActual);
        actualizarBitacora(tiempoInicial, tiempoFinal, controlUVC.getVolumen(),
                           controlUVC.getCiclos(), volumenSesion,
                           diferencia, controlUVC.getTuboUVC());
      }

    }

  }

  lastSwitchState = lecturaInterruptor;
}

void controlTuboUVC() {
  if (!controlUVC.isValidTuboUVC() && nextionPage != 3) {
    nex.sendCommand("page change");
    nex.sendCommand("page change");
    nextionPage = 3;
  }
}

void actualizarPicVolumen() {
  //7 = off, 8 = on
  char buffer[10];
  double volumen = controlUVC.getVolumen();
  Serial.printf("\r\ngetVolumen: %.3f\r\n", volumen);
  if (volumen) {
    picVol.setPictureID(8);
    picVol.setPictureID(8);
    dtostrf(volumen, 6, 3, buffer);
    nex.sendCommand("txtVolumen.picc=8");
    nex.sendCommand("txtVolumen.picc=8");
    txtVolumen.setText(buffer);
    txtVolumen.setText(buffer);
  } else {
    picVol.setPictureID(7);
    picVol.setPictureID(7);
    nex.sendCommand("txtVolumen.picc=7");
    nex.sendCommand("txtVolumen.picc=7");
    txtVolumen.setText("");
    txtVolumen.setText("");
  }
}

void actualizarPicRun(bool flag) {
  //5 = off, 6 = on
  if (flag) {
    picRun.setPictureID(5);
    picRun.setPictureID(5);
  } else {
    picRun.setPictureID(6);
    picRun.setPictureID(6);
  }
}

void callback_mostrarCiclos(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    nextionPage = 2;

    actualizarPicVolumen();
    actualizarPicRun(!flagRun);
    actualizarFecha();
    char buffer[10];
    double ciclos = controlUVC.getCiclos();
    dtostrf(ciclos, 6, 3, buffer);
    txtCiclos.setText(buffer);
    txtCiclos.setText(buffer);
  }
}

void callback_mostrarHoras(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    nextionPage = 3;

    uint32_t tiempo = controlUVC.getTuboUVC();
    if (tiempo) {
      uint32_t tiempoHoras = tiempo / 3600;
      tiempo %= 3600;
      uint32_t tiempoMinutos = tiempo / 60;
      tiempo %= 60;
      uint32_t tiempoSegundos = tiempo;

      txtHoras.setValue(tiempoHoras);
      txtHoras.setValue(tiempoHoras);

      txtMinutos.setValue(tiempoMinutos);
      txtMinutos.setValue(tiempoMinutos);

      txtSegundos.setValue(tiempoSegundos);
      txtSegundos.setValue(tiempoSegundos);
    }
    txtEquipo.setText(strdup(ssid_ap));
    txtEquipo.setText(strdup(ssid_ap));
  }
}

void callback_reset(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_PUSH || type == NEX_EVENT_POP) {
    controlUVC.setTuboUVC(0);
    txtEquipo.setText(strdup(ssid_ap));
    txtEquipo.setText(strdup(ssid_ap));
  }
}

void callback_config(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    nextionPage = 4;

    char buffer[10];
    float volumen = controlUVC.getVolumen();
    if (volumen > 0.0) {
      dtostrf(volumen, 6, 3, buffer);
      txtTotal.setText(buffer);
      txtTotal.setText(buffer);
    } else {
      txtTotal.setText("");
      txtTotal.setText("");
    }
  }
}

void callback_configFechaHora(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    nextionPage = 7;

    DateTime now = rtc.now();
    txtDD.setValue(now.day());
    txtDD.setValue(now.day());
    txtMM.setValue(now.month());
    txtMM.setValue(now.month());
    txtAA.setValue(now.year());
    txtAA.setValue(now.year());
    txthh.setValue(now.hour());
    txthh.setValue(now.hour());
    txtmm.setValue(now.minute());
    txtmm.setValue(now.minute());
    txtss.setValue(now.second());
    txtss.setValue(now.second());
  }
}

void callback_calcularVolumen(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    char buffer[50];

    txtLargo.getText(buffer, 50);
    txtLargo.getText(buffer, 50);
    float largo = atof(buffer);

    txtAncho.getText(buffer, 50);
    txtAncho.getText(buffer, 50);
    float ancho = atof(buffer);

    txtAlto.getText(buffer, 50);
    txtAlto.getText(buffer, 50);
    float alto = atof(buffer);

    float total = largo * ancho * alto;
    Serial.printf("\r\nVolúmen calculado %.3f\r\n", total);
    dtostrf(total, 6, 3, buffer);

    txtTotal.setText(buffer);
    txtTotal.setText(buffer);
  }
}

void callback_guardarVolumen(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_POP) {
    char buffer[50];

    txtTotal.getText(buffer, 50);
    txtTotal.getText(buffer, 50);
    float volumen = atof(buffer);

    controlUVC.setVolumen(volumen);

    nex.sendCommand("page ciclos");
    nex.sendCommand("page ciclos");

    actualizarPicVolumen();
  }
}

void callback_hardReset(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_PUSH || type == NEX_EVENT_POP) {
    SPIFFS.format();
    delay(1000);
    ESP.restart();
  }
}

void callback_guardarFechaHora(NextionEventType type, INextionTouchable *widget) {
  if (type == NEX_EVENT_PUSH) {
    int DD, MM, YY, hh, mm, ss;
    DD = txtDD.getValue();
    DD = txtDD.getValue();
    MM = txtMM.getValue();
    MM = txtMM.getValue();
    YY = txtAA.getValue();
    YY = txtAA.getValue();
    hh = txthh.getValue();
    hh = txthh.getValue();
    mm = txtmm.getValue();
    mm = txtmm.getValue();
    ss = txtss.getValue();
    ss = txtss.getValue();
    rtc.adjust(DateTime(YY, MM, DD, hh, mm, ss));
  }
  if (type == NEX_EVENT_POP) {
    nex.sendCommand("page ciclos");
    nex.sendCommand("page ciclos");
    nextionPage = 2;

    actualizarPicVolumen();
    actualizarPicRun(!flagRun);
    actualizarFecha();
    char buffer[10];
    double ciclos = controlUVC.getCiclos();
    dtostrf(ciclos, 6, 3, buffer);
    txtCiclos.setText(buffer);
    txtCiclos.setText(buffer);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.print(F("Configuring access point...\r\n"));

  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.printf("\n");
  }
  //SPIFFS.format();

  controlUVC.init();
  //Serial.println(controlUVC.getVolumen());
  //Serial.println(controlUVC.getCiclos(149), 3);
  //return;

  //Serial.printf("\r\nESP ChipID: %s\r\n", chipId.c_str());
  while (!chipId.equals("ESPUVC-a16c3")) {
    Serial.println("bootloader detected cloned, stop boot.");
    delay(5000);
  }

  nex.init();

  if (!rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower()) {
    Serial.println("RTC lost power, lets set the time!");
    // following line sets the RTC to the date & time this sketch was compiled
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    // rtc.adjust(DateTime(2020, 11, 30, 0, 0, 0));
  }

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, !LOW);

  pinMode(relayA, OUTPUT);
  digitalWrite(relayA, !LOW);
  pinMode(relayB, OUTPUT);
  digitalWrite(relayB, !LOW);

  pinMode(interruptorA, INPUT_PULLUP);

  controlReles(false);

  btnCiclos.attachCallback(&callback_mostrarCiclos);
  btnCiclos1.attachCallback(&callback_mostrarCiclos);
  btnHoras.attachCallback(&callback_mostrarHoras);
  btnHoras1.attachCallback(&callback_mostrarHoras);
  btnConfig.attachCallback(&callback_config);
  btnConfig1.attachCallback(&callback_config);
  txtFecha.attachCallback(&callback_configFechaHora);
  btnCalcular.attachCallback(&callback_calcularVolumen);
  btnGuardar.attachCallback(&callback_guardarVolumen);
  btnCancelar.attachCallback(&callback_mostrarHoras);
  btnAceptar.attachCallback(&callback_reset);
  btnOk.attachCallback(&callback_hardReset);
  btnCancelar_7.attachCallback(&callback_mostrarCiclos);
  btnAceptar_7.attachCallback(&callback_guardarFechaHora);
  btnAceptar_8.attachCallback(&callback_mostrarHoras);

  wifiConnect();

  if (!MDNS.begin(host_name)) {
    Serial.println(F("Error setting up MDNS responder!"));
    ESP.restart();
  }
  Serial.println("mDNS responder started");

  server.on("/", handleRoot);
  server.on("/list", handleFileList);
  server.on("/download", HTTP_GET, handleFileDownload);
  server.on("/wifiSetup", HTTP_POST, handleWiFiSetup);
  server.on("/volumeSetup", HTTP_POST, handleVolumeSetup);
  server.onNotFound(handleNotFound);
  server.begin();

  INDEX_HTML.replace("{NAME}", String(host_name));

  MDNS.addService("http", "tcp", 80);
  MDNS.addService("ws", "tcp", 81);

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  nex.sendCommand("page ciclos");
  nex.sendCommand("page ciclos");
  actualizarPicVolumen();
  actualizarFecha();
  tickerFecha.attach(60, actualizarFecha);
  //tickerFecha.attach(5, handleFileList);

  Serial.println(F("HTTP server started"));
}

void loop() {
  MDNS.update();
  server.handleClient();
  webSocket.loop();
  nex.poll();
  controlInterruptor();
  controlTuboUVC();
}

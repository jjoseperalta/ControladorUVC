#include "ControlUVC.h"

ControlUVC::ControlUVC() {
  init();
}

void ControlUVC::init(void) {
  if (!loadVolumen())
    __volumen = 0.0;
  
  __ciclos = 0.0;

  if (!loadTuboUVC())
    __tubo_UVC = 0;
}

void ControlUVC::setVolumen(float volumen) {
  __volumen = volumen;

  updateVolumen();
}

float ControlUVC::getVolumen(void) {
  return __volumen;
}

bool ControlUVC::loadVolumen(void) {
  File myFile = SPIFFS.open("/volumen.txt", "r");
  
  if (myFile) {
    
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (line != "") {
        //Serial.printf("\r\nloadVolumen: %s\r\n", line.c_str());
        __volumen = line.toFloat();
      } else
        return false;
    }
    
    myFile.close();
    return true;
  } else
    return false;
}

bool ControlUVC::updateVolumen() {
  File myFile = SPIFFS.open("/volumen.txt", "w");

  if (myFile) {
    //Serial.printf("\r\nsaveVolumen: %.2f\r\n", __volumen);
    myFile.println(__volumen);
    myFile.close();
    return true;
  } else
    return false;
}

double ControlUVC::getCiclos(uint32_t segundos) {
  double segundosCiclo;

  if (__volumen > 0.0) {
    segundosCiclo = __volumen / VOLUMEN_AIRE_SEGUNDO;
    __ciclos = segundos / segundosCiclo;
    
  }
  
  return __ciclos;
}

double ControlUVC::getCiclos(void) {
  return __ciclos;
}

void ControlUVC::setTuboUVC(uint32_t segundos) {
  __tubo_UVC = segundos;

  updateTuboUVC(segundos);
}

uint32_t ControlUVC::getTuboUVC(void) {
  return __tubo_UVC;
}

bool ControlUVC::loadTuboUVC(void) {
  File myFile = SPIFFS.open("/tiempo.txt", "r");
  
  if (myFile) {
    
    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      if (line != "") {
        //Serial.printf("\r\nloadTuboUVC: %s\r\n", line.c_str());
        __tubo_UVC = line.toInt();
      } else
        return false;
    }
    
    myFile.close();
    return true;
  } else
    return false;
}

bool ControlUVC::updateTuboUVC(uint32_t segundos) {
  File myFile = SPIFFS.open("/tiempo.txt", "w");

  if (myFile) {
    //Serial.printf("\r\nupdateTuboUVC: %u\r\n", segundos);
    myFile.println(segundos);
    myFile.close();
    return true;
  } else
    return false;
}

bool ControlUVC::isValidTuboUVC() {
  if (__tubo_UVC < VIDA_UTIL_TUBO_UVC)
    return true;
  else
    return false;
}

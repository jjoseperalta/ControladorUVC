#ifndef ControlUVC_h
#define ControlUVC_h

#include "Arduino.h"
#include <FS.h>

/**
 * 920 Volumen de Aire por Hora según especificación del Ventilador.
 * 0.2555555556 es la conversión de 920 vol/hora a vol/segundos
 */
#ifndef VOLUMEN_AIRE_SEGUNDO
#define VOLUMEN_AIRE_SEGUNDO  0.2555555556
#endif

/**
 * La vida útil del tubo UVC es de 10000 Hrs. según el fabricante
 * 36000000 es la conversión de 10000 Hrs. a segundos
 */
#ifndef VIDA_UTIL_TUBO_UVC
#define VIDA_UTIL_TUBO_UVC    36000000
#endif

/**
 * 
 */
class ControlUVC {
  
  public:

    /**
     * Constructor de la clase
     */
    ControlUVC();

    /**
     * Métodos
     */
    void      init();
    void      setVolumen(float volumen);
    float     getVolumen();
    bool      loadVolumen();
    bool      updateVolumen();
    double    getCiclos(uint32_t segundos);
    double    getCiclos();
    void      setTuboUVC(uint32_t segundos);
    uint32_t  getTuboUVC();
    bool      loadTuboUVC();
    bool      updateTuboUVC(uint32_t segundos);
    bool      isValidTuboUVC();
    
  private:

    /**
     * Variables
     */
    float     __volumen;
    float     __ciclos;
    uint32_t  __tubo_UVC;
};

#endif /* #ifndef ControlUVC_h */

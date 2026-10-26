#ifndef __SCR_SYSTEM_H__
#define __SCR_SYSTEM_H__

/**
 * System screen controller (SquareLine: scrSystem).
 *
 * Datos del SoC:
 *  - lblCpuFreq    : frecuencia de CPU
 *  - lblHeapFree   : heap libre (esp_get_free_heap_size), cada 1 s
 *  - lblHeapMin    : mínimo histórico de heap libre (reseteable con el botón)
 *  - lblIdfVersion : versión de ESP-IDF
 *  - lblChipModel  : modelo, revisión y núcleos (esp_chip_info)
 *  - lblFlashSize  : tamaño de la flash
 */
void scr_system_prepare(void);
void scr_system_init(void);
void scr_system_uninit(void);
void scr_system_step(void);

/** Acción del botón físico en esta pantalla: resetea el mínimo histórico de heap. */
void scr_system_on_button(void);

#endif

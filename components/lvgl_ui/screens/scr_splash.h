#ifndef __SCR_SPLASH_H__
#define __SCR_SPLASH_H__

/**
 * Splash screen controller (SquareLine: scrSplash).
 *
 *  - Fade-in de la retroiluminación (800 ms) al arrancar.
 *  - Animación "Iniciando" -> "Iniciando..." en loop sobre lblLoading.
 *  - Tocar cualquier punto de la pantalla navega a Home.
 */
void scr_splash_prepare(void);
void scr_splash_init(void);
void scr_splash_uninit(void);
void scr_splash_step(void);

#endif

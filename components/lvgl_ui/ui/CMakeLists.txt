SET(SOURCES screens/ui_scrHome.c
    screens/ui_scrSystem.c
    screens/ui_scrSettings.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c)

add_library(ui ${SOURCES})

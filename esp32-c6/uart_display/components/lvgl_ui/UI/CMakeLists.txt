SET(SOURCES screens/ui_ScreenSplash.c
    screens/ui_ScreenClock.c
    screens/ui_ScreenWiFi.c
    screens/ui_ScreenDebug.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c)

add_library(ui ${SOURCES})

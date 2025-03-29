set (src
    src/adc.cpp
    src/cmd/cmdFormat.cpp
    src/cmd/cmdEraseMetrics.cpp
    src/cmd/cmdSendMetrics.cpp
    src/cmd/cmdLock.cpp
    src/cmd/cmdMQTT.cpp
    src/cmd/cmdNetw.cpp
    src/cmd/cmdUpdate.cpp
    src/cmd/cmdHostOTA.cpp
    src/cmd/cmdSetOTA.cpp
    src/cmd/cmdDisplay.cpp
    src/FramI2C.cpp
    src/kbd.cpp
    src/logmanager.cpp
    src/mesh_netif.c
    src/meter.cpp
    src/meterClass.cpp
    src/sendData.cpp
    src/webserver.cpp
    src/lcd.cpp
    src/displayManager.cpp
    src/otaupdate.cpp
    kbd/cmdADC.cpp
    kbd/cmdApp.cpp
    kbd/cmdBaseTimer.cpp
    kbd/cmdConfig.cpp
    kbd/cmdOTA.cpp
    kbd/cmdEnDecrypt.cpp
    kbd/cmdErase.cpp
    kbd/cmdFindUnit.cpp
    kbd/cmdFram.cpp
    kbd/cmdLog.cpp
    kbd/cmdLogLevel.cpp
    kbd/cmdMeter.cpp
    kbd/cmdMetersreset.cpp
    kbd/cmdPrepaid.cpp
    kbd/cmdResetConf.cpp
    kbd/cmdSecurity.cpp
    )

    set (html 
    web/index-min.html
    cert/cert.pem
    )

    set(bins
    web/nak.png
    web/ok.png
    web/favicon.ico
    web/cher.png
    web/cancel.png
    )

idf_component_register(SRCS "${src}"
                       INCLUDE_DIRS inc
REQUIRES driver esp_timer nvs_flash esp_wifi esp_adc esp_event esp_netif spiffs json console app_update mqtt esp_http_client esp_https_server esp_driver_i2c esp_https_ota
esp_http_client app_update esp_https_ota esp_netif mbedtls nvs_flash
# REQUIRES driver esp_timer nvs_flash esp_https_ota esp_timer esp_wifi esp_adc esp_event esp_netif spiffs json console u8g2 app_update wifi_provisioning mqtt esp_http_client esp_https_server u8g2-hal-esp-idf

                       EMBED_TXTFILES "${html}" 
                       EMBED_FILES "${bins}"
)



set(COMPONENT_ADD_INCLUDEDIRS "")
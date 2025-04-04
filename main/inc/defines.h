
// #define FRAMSPI
//FRAM pins SPI
#ifdef  CONFIG_IDF_TARGET_ESP32
#define FMOSI							(23)
#define FMISO							(19)
#define FCLK							(18)
#define FCS								(5)
#define OLED_SDA                        (22)
#define OLED_SCL                        (21)
#define RELAY                           (14)
#define WIFILED                         (4)
#define BEATPIN                         (26)
// I2C Fram
#define FSDA                            FCLK        
#define FSCL                            FMOSI

//lcd 
#define PIN_NUM_SDA                     FMOSI 
#define PIN_NUM_SCL                     FCLK 

#endif
//s3 pcb design EasyEDA meterIoTPSRAMS3 pins
#ifdef  CONFIG_IDF_TARGET_ESP32S3
#define FMOSI							(1)
#define FMISO							(40)
#define FCLK							(39)
#define FCS								(38)

#define OLED_SDA                        (2)
#define OLED_SCL                        (42)
#define RELAY                           (8)
#define WIFILED                         (41)
#define BEATPIN                         (19)
// I2C Fram
#define FSDA                            FMOSI        
#define FSCL                            FCLK

#define DISPLAY
//lcd 
#define PIN_NUM_SDA                     FMOSI 
#define PIN_NUM_SCL                     FCLK 
#endif

#define I2C_BUS_PORT                    (1)
#define LCD_PIXEL_CLOCK_HZ              (400 * 1000)

#define PIN_NUM_RST                     (-1)
#define I2C_HW_ADDR                     (0x3C)
#define LCD_H_RES                       (128)
#define LCD_V_RES                       (64)
// Bit number used to represent command and parameter
#define LCD_CMD_BITS                    8
#define LCD_PARAM_BITS                  8

#define RELAYON                         (0)
#define RELAYOFF                        (1)
#define DEBB
#define LOCALTIME                       "GMT+5"
#define LOGOPT
#define SAVEDATE                        (5)
#define FIXEDMESH                       (1)
#define STATSM
#define STATSDELAY                      (60000)
#define MAXCMDS                         (10) 

#define CENTINEL                        (0x12345678)

// #define AMPSHORA                        (8.33)
#define BIASHOUR                        (14)

#define MQTTBIG                         (3000)  //big cause certiticate is at least 1700 bytes
#define OTADEL                          5000
#define MESH_TAG                        "MeterIoT"
#define WIFI_CONNECTED_BIT              BIT0
#define WIFI_FAIL_BIT                   BIT1

#define SNTPTRY                         (10)
#define FREEANDNULL(x)		            if(x) {free(x);x=NULL;}
#define MAXINTCMDS                      (16)
#define SUPERSECRET                     "mi mama me mima mucho y MepsiCia"
#define APPNAME                         "meter"
#define MQTTSECURE       
#define MAXNODES                        (100)                
#define MAXDEVSS                        (1)
#define LONGCLICK                       (800)
#define MAXMENU                         (10)
#define SCRFONT                         (13)
#define MQTTSENDER                      (2000)
#define MQTTSENDERWAIT                  (10)
#define SLOTSIZE                        (2)        // slot time in seconds
#define HOUR                            (60)      // in seconds
#define QUEUE                           "meter"
#define MAXMQTTERRS                     (10)
#define NUMDUPS                         (500)
#define TAG                             "meter"
#define EXPECTED_NODES                  (2500000)
#define EXPECTED_CONNS                  (10000)
#define EXPECTED_DELIVERY_TIME          (2)
#define PREPAID                         (1)
#define POSTPAID                        (0)
#define TURNOFF                         (1)
#define TURNON                          (0)
#define MINPREPAID                      (1)       //as of today aprox $1
#define MESHTIMEOUT                     (10000)
// Error codes
#define METERB                          (-4000)
#define M_AES_MALLOC                    (METERB+0)
#define M_AES_KEY                       (METERB-1)
#define M_AES_ENCRYPT                   (METERB+2)
#define M_AES_DECRYPT                   (METERB+3)
#define M_NO_CMD                        (METERB+4)
#define M_GET_TABLE                     (METERB+5)
#define M_GET_TABLE                     (METERB+5)

#define MAXMQTTERR                      (2)
#define EMERGENCY                       (0)
#define BOOTRESP                        (1)
#define SENDMETRICS                     (2)
#define METERSDATA                      (3)
#define LOCKMETER                       (4)
#define CONFIRMLOCK                     (5)
#define CONFIRMLOCKERR                  (6)
#define INSTALLATION                    (7)
#define REINSTALL                       (8)
#define CONFIRMINST                     (9)
#define FORMAT                          (10)
#define UPDATEMETER                     (11)
#define ERASEMETRICS                    (12)
#define MQTTMETRICS                     (13)
#define METRICRESP                      (14)
#define SHOWDISPLAY                     (15)
#define CONFIRMTIMER                    (1000)
#define METER_NOT_FOUND                 (0x1234)

#define ALL

// #define OTAURL                          "http://64.23.180.233/metermgr.bin"
// have the file metermgr.bin in the webserver do under /var/www/html
// in espidf use an espidf terminal and upload
// cd build
// scp metermgr.bin root@64.23.180.233:/var/www/html
// the download url is fixed at that /var/www/html directory by default

#define ADC_ATTEN                       ADC_ATTEN_DB_12
#define ADC_BIT_WIDTH                   SOC_ADC_DIGI_MAX_BITWIDTH
#define ADC_BUFF                        (256)
#define ADC_CONV_MODE                   ADC_CONV_SINGLE_UNIT_1
#define ADC_UNIT                        ADC_UNIT_1
#define ADCBIAS                         (20)
#define ADCDEL                          (300)
#define ADCFREQ                         (20)
#define ADCLOOP                         (5)
#define ADCMAXAMP                       (5.5)
#define ADCMINAMP                       (4.05)
#define ADCVREF                         (3.3)
#define VOLTS                           (118.5)
#define BEATTIMER                       (10000)
#define SSIDBLINKTIME                   (80)
#define MINTICKSISR                     (10)            //very importante, debouncing time


#define EXAMPLE_LCD_PIXEL_CLOCK_HZ    (400 * 1000)
#define EXAMPLE_PIN_NUM_SDA           2
#define EXAMPLE_PIN_NUM_SCL           42
#define EXAMPLE_PIN_NUM_RST           -1
#define EXAMPLE_I2C_HW_ADDR           0x3C

// The pixel number in horizontal and vertical
#define EXAMPLE_LCD_H_RES              128
#define EXAMPLE_LCD_V_RES              64
// Bit number used to represent command and parameter
#define EXAMPLE_LCD_CMD_BITS           8
#define EXAMPLE_LCD_PARAM_BITS         8

#define EXAMPLE_LVGL_TICK_PERIOD_MS    5
#define EXAMPLE_LVGL_TASK_STACK_SIZE   (4 * 1024)
#define EXAMPLE_LVGL_TASK_PRIORITY     2
#define EXAMPLE_LVGL_PALETTE_SIZE      8
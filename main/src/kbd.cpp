#define GLOBAL
#include "includes.h"
#include "globals.h"
#include "forwards.h"

void kbd(void *pArg)
{
  repl=NULL;
  repl_config = ESP_CONSOLE_REPL_CONFIG_DEFAULT();
  repl_config.prompt=(char*)"Login>";
  // repl_config.prompt=(char*)"LoginPls>";
  esp_console_dev_uart_config_t uart_config = ESP_CONSOLE_DEV_UART_CONFIG_DEFAULT();

  framArg.format =                arg_str0(NULL, "format", "slow | fast", "Format Fram");
  framArg.midw =                  arg_str0(NULL, "wmid","id",  "Set id of working meter ");
  framArg.midr =                  arg_int0(NULL, "rmid","dummy",  "Read id of working meter ");
  framArg.kwhstart =              arg_int0(NULL, "kwh", "value", "Set kwh start of working meter ");
  framArg.rstart =                arg_int0(NULL, "rkwh", "dummy", "Read kwh start of working meter ");
  framArg.mtime =                 arg_int0(NULL, "metert", "1 write 0 read", "Write/read working meter date ");
  framArg.mbeat =                 arg_int0(NULL, "beat", "value", "Write beat of working meter ");
  framArg.stats =                 arg_str0(NULL, "stats", "Show stats", "Root node stats");
  framArg.end =                   arg_end(8);


  loglevel.level=                 arg_str0(NULL, "l", "None-Error-Warn-Info-Debug-Verbose", "Log Level");
  loglevel.end=                   arg_end(1);
  
  basetimer.first=                arg_int0(NULL, "t", "Multiplier for firsttimer", "BaseTiemr");
  basetimer.repeat=               arg_int0(NULL, "r", "Multiplier for repeattimer", "BaseTimer");
  basetimer.end=                  arg_end(1);
  
  prepaidcmd.unit=                arg_int0(NULL, "u", "Unit to credit", "Prepaid");
  prepaidcmd.balance=             arg_int0(NULL, "b", "kwh to credit", "Prepaid");
  prepaidcmd.end=                 arg_end(2);


  resetlevel.cflags=               arg_str0(NULL, "f", "All Meter Mesh Collect Send Mqtt)", "Reset Flags");
  resetlevel.end=                  arg_end(1);

  endec.key=                       arg_int0(NULL, "k", "AES key numeric", "Aes Key");
  endec.end=                       arg_end(1);

  kbdsedcurity.password=           arg_str0(NULL, "p", "password", "Kbd Security");
  kbdsedcurity.newpass=            arg_str0(NULL, "n", "neww password", "Kbd Security");
  kbdsedcurity.nopass=             arg_str0(NULL, "a", "no password", "Kbd Security");
  kbdsedcurity.end=                arg_end(3);

  appSSID.password=                 arg_str0(NULL, "p", "password", "SSID Change");
  appSSID.newpass=                  arg_str0(NULL, "n", "neww SSID", "SSID Change");
  appSSID.nopass=                   arg_str0(NULL, "l", "nA", "SSID Change");
  appSSID.end=                      arg_end(3);

  logArgs.show =                  arg_int0(NULL, "show", "# of lines", "Show logs");
  logArgs.erase =                 arg_int0(NULL, "erase", "0/1" ,"Erase logs");
  logArgs.end =                   arg_end(2);

  adcArgs.start=                  arg_int0(NULL, "s", "Start ADC Mgr ", "ADC Mgr");
  adcArgs.kill=                   arg_int0(NULL, "k", "Stop ADC Mgr", "ADC Mgr");
  adcArgs.vref=                   arg_dbl0(NULL, "v", "Vref", "ADC Mgr");
  adcArgs.freq=                   arg_int0(NULL, "f", "Freq", "ADC Mgr");
  adcArgs.bias=                   arg_int0(NULL, "b", "Bias", "ADC Mgr");
  adcArgs.delay=                  arg_int0(NULL, "d", "Delay", "ADC Mgr");
  adcArgs.minAmp=                 arg_dbl0(NULL, "m", "MinAmp", "ADC Mgr");
  adcArgs.maxAmp=                 arg_dbl0(NULL, "x", "maxAMp", "ADC Mgr");
  adcArgs.end=                    arg_end(6);

  fram_cmd = {
        .command = "fram",
        .help = "Manage Fram",
        .hint = NULL,
        .func = &cmdFram,
        .argtable = &framArg
    };

  meter_cmd = {
        .command = "meter",
        .help = "Show Meter",
        .hint = NULL,
        .func = &cmdMeter,
        .argtable = NULL
    };

  config_cmd = {
        .command = "config",
        .help = "Show Configuration",
        .hint = NULL,
        .func = &cmdConfig,
        .argtable = NULL
    };

  ota_cmd = {
        .command = "ota",
        .help = "Start OTA",
        .hint = NULL,
        .func = &cmdOTA,
        .argtable = NULL
    };

  erase_cmd = {
        .command = "erase",
        .help = "Erase Configuration",
        .hint = NULL,
        .func = &cmdErase,
        .argtable = NULL
    };

  findunit_cmd = {
        .command = "find",
        .help = "Find Unit Blink",
        .hint = NULL,
        .func = &cmdFindUnit,
        .argtable = NULL
    };

  meshreset_cmd = {
        .command = "install",
        .help = "Set all units to config mode",
        .hint = NULL,
        .func = &cmdMetersreset,
        .argtable = NULL
    };


  loglevel_cmd = {
        .command = "loglevel",
        .help = "Set log level",
        .hint = NULL,
        .func = &cmdLogLevel,
        .argtable = &loglevel
    };

  basetimer_cmd = {
        .command = "timer",
        .help = "Sewt repeat timer basetime",
        .hint = NULL,
        .func = &cmdBasetimer,
        .argtable = &basetimer
    };

    prepaid_cmd = {
        .command = "prepaid",
        .help = "Credit a unit with kwh",
        .hint = NULL,
        .func = &cmdPrepaid,
        .argtable = &prepaidcmd
    };

    resetconf_cmd = {
        .command = "resetconf",
        .help = "Reset Conf flags",
        .hint = NULL,
        .func = &cmdResetConf,
        .argtable = &resetlevel
    };

    aes_cmd = {
        .command = "aes",
        .help = "Encrypt Decrypt",
        .hint = NULL,
        .func = &cmdEnDecrypt,
        .argtable = &endec
    };

     security_cmd = {
        .command = "login",
        .help = "Login to system",
        .hint = NULL,
        .func = &cmdSecurity,
        .argtable = &kbdsedcurity
    };

     app_cmd = {
        .command = "app",
        .help = "Set app SSID",
        .hint = NULL,
        .func = &cmdApp,
        .argtable = &appSSID
    };

       log_cmd = {
        .command = "log",
        .help = "Log options",
        .hint = NULL,
        .func = &cmdLog,
        .argtable = &logArgs
    };

     adc_cmd = {
        .command = "adc",
        .help = "ADC tests",
        .hint = NULL,
        .func = &cmdAdc,
        .argtable = &adcArgs
    };

  //only register security cmd. He will register all other cmds if password ok
    ESP_ERROR_CHECK(esp_console_cmd_register(&security_cmd));
    if(theConf.subnode==1)    //subnode used to store Autologin
    {
      loginf=true;
      repl_config.prompt=(char*)"MeterIoT>";
      ESP_ERROR_CHECK(esp_console_cmd_register(&meter_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&fram_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&config_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&erase_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&loglevel_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&resetconf_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&aes_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&basetimer_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&prepaid_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&app_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&log_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&adc_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&findunit_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&meshreset_cmd));
      ESP_ERROR_CHECK(esp_console_cmd_register(&ota_cmd));
    }
  ESP_ERROR_CHECK(esp_console_new_repl_uart(&uart_config, &repl_config, &repl));
  ESP_ERROR_CHECK(esp_console_start_repl(repl));
  vTaskDelete(NULL);   //we're done here
}
#ifndef FORWARDS_H
#define FORWARDS_H

void root_set_senddata_timer();
void kbd(void *pArg);
void displayManager(void *pArg);
void delay(uint32_t a);
uint32_t xmillis(void);
esp_err_t init_lcd();

int cmdFormat(void *argument);
int cmdEraseMetrics(void *argument);
int cmdSendMetrics(void *argument);
int cmdUpdate(void *argument);
int cmdNetw(void *argument);
int cmdMQTT(void *argument);
int cmdLock(void *argument);
int cmdHostOTA(void *argument);
int cmdSetOTA(void *argument);
int cmdDisplay(void *argument);

meshunion_t* sendData(bool forced);
void start_webserver();
void root_mqtt_sender(void *pArg);
void mqttMgr(void *pArg);
void emergencyTask(void *pArg);
static void root_mqtt_app_start();
static void pcnt_init(int* unit);    
void write_to_flash();  
void send_911_call(char *theEmergency,char *section);
void delete_dup_buffers();
void check_dups(cJSON *elArr,int nodeid);
void send_duplicate_msg();
char * convertBinaryToJson(meshunion_t* elNodo);
void erase_config();
void writeLog(char * que);
void logFileInit();
void main_app(void *parg);
// kbd cmds
int cmdFram(int argc, char **argv);
int cmdMID(int argc, char **argv);
int cmdMeter(int argc, char **argv);
int cmdErase(int argc, char **argv);
int cmdPrepaid(int argc, char **argv);
int cmdBasetimer(int argc, char **argv);
int cmdPCNT(int argc, char **argv);
int cmdLogLevel(int argc, char **argv);
int cmdZeroMeter(int argc, char **argv);
int cmdResetConf(int argc, char **argv);
int cmdLog(int argc, char **argv);
int cmdAdc(int argc, char **argv);
int cmdApp(int argc, char **argv);
int cmdEnDecrypt(int argc, char **argv);
int cmdConfig(int argc, char **argv);
int cmdFindUnit(int argc, char **argv);
int cmdSecurity(int argc, char **argv);
int cmdMetersreset(int argc, char **argv);
int cmdOTA(int argc, char **argv);
void oled_disp(lv_disp_t *disp);
esp_err_t root_send_confirmation_central(char *msg,uint16_t size,char *cualQ);
extern void showData(void * pArg);

#endif

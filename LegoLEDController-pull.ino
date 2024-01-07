//#include <UnoWiFiDevEd.h>

//#include <dummy.h>

/*
 * This sketch will set a PIN high or low depending on communication via a REST/HTTP interface
 * PUT /lights/on          - Sets the pin high and turns on the lights
 * PUT /lights/off         - Sets the pin low and turns off the lights
 * GET /lights/status      - gets the current state of the lights
 * GET /admin/vcc          - gets the VCC voltage
 * 
 * The WiFi configuration can be set with new SSID/Password 
 * POST /admin/wifi
 */

#include <ESP8266WiFi.h>
//#include <Arduino_JSON.h>
#include <ArduinoJson.h>
#include <EEPROM.h> //The EEPROM libray 

#include <ESP8266HTTPClient.h>
//#include "EspMQTTClient.h"
#include "PubSubClient.h"

#include "esp_sleep.h"
//#include "esp_deep_sleep.h"
//#include "soc/rtc_cntl_reg.h"
//#include "soc/rtc.h"
//#include "driver/rtc_io.h"

const int EEPROM_SIZE          = 512;
const int WIFI_DATA_START_ADDR = 0;
const int WIFI_SSID_LEN        = 50;
const int WIFI_PWD_LEN         = 50;
const int PIN_DATA_START_ADDR  =150;

const int CONNECT_TIMEOUT_MS  = 30000;
const int CONNECT_INTERVAL_MS = 500;

const int REST_COMMAND_COMPONENTS = 3;
const int WIFI_CLIENT_READ_TIMEOUT_MS = 5000;

const int SERVER_PORT = 80;


const char* const REST_PUT          ="PUT";
const char* const REST_POST         ="POST";
const char* const REST_GET          ="GET";
const char* const ADMIN_COMMAND     ="admin";
const char* const LIGHTS_COMMAND    ="lights";
const char* const LIGHTS_ON         ="on";
const char* const LIGHTS_OFF        ="off";
const char* const LIGHTS_STATUS     ="status";
const char* const VCC               ="vcc";
const char* const WIFI              ="wifi";
const char* const WIFI_SSID         ="ssid";
const char* const WIFI_PASSWORD     ="password";


const char* const STATIC_IP          = "staticip";
const char* const STATIC_GATEWAY_IP  = "gateway";
const char* const STATIC_SUBNET_MASK = "subnetmask";


const char* const HTTP_CONTENT_LEN_HDR = "Content-Length";
const char* const END_POINT_SEPARATOR      = "/";
const char* const HTTP_LINE_SEPARATOR      = " ";
const char* const POST_PARAM_SEPARATOR     = "&";
const char* const POST_KEY_VALUE_SEPARATOR = "=";


//const String url_admin_wifi_set = "/admin/wifi";
const char* const SET_WIFI_URL = "/admin/wifi";
const char* const SET_WIFI_DETAILS_COMMAND = "/set-wifi-details";


//const char* g_ssid = "Anfield";
//const char* g_password = "1985Dalglish1991";
//const char* g_ssid = "SSID";
//const char* g_password = "PASSWORD";

const byte BUILTIN_LED_PIN=2;
const byte BUILTIN_TX_PIN=1;
const byte BUILTIN_RX_PIN=3;

struct storeStruct_t{
  char myVersion[3];
  char ssid[WIFI_SSID_LEN];
  char password[WIFI_PWD_LEN];
};

//DynamicJsonDocument json_doc(1024);

/**
 * Define AP WIFI Details
 */
const char* const AP_SSID = "ESP8266";
const char* const AP_PASS = "bruckfexit";
const int AP_MAX_CON = 1;

const IPAddress AP_LOCAL_IP(192,168,1,1);
const IPAddress AP_GATEWAY(192,168,1,9);
const IPAddress AP_SUBNET(255,255,255,0);

const IPAddress DEFAULT_STATIC_IP(192,168,0,134);
const IPAddress DEFAULT_STATIC_IP_GATEWAY(192,168,0,1);
const IPAddress DEFAULT_STATIC_IP_SUBNET(255,255,255,0);


//EspMQTTClient client(
//  "Anfield",
//  "1985Dalglish1991",
//  "192.168.0.124",  // MQTT Broker server ip
//  "alex",   // Can be omitted if not needed
//  "Pl@5m0d!um",   // Can be omitted if not needed
//  "db5lights"      // Client name that uniquely identify your device
//);

//PubSubClient mqttClient(wifiClient);

/**
 * Define Global Variables
 */
// Create an instance of the server
//WiFiServer server(SERVER_PORT);

storeStruct_t g_settings = {
  "01",
  "SSID"
  "Password"
};

bool g_pin_state = false;


//Needed to get VCC values
ADC_MODE(ADC_VCC);

/**
 * This sets everything up, it is run once
 */
void setup() {

  //Turn off WiFi on start up, don't turn on until ready to connect
  WiFi.mode( WIFI_OFF );
  WiFi.forceSleepBegin();
  delay(1);
  
  Serial.begin(115200);
  delay(10);
  Serial.println("\nStarting...\n");

  //Prepares Memory
  //EEPROM.begin(512);

  // prepare GPIO2
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  digitalWrite(BUILTIN_LED_PIN, 1); //Turn OFF

//  pinMode(BUILTIN_TX_PIN, OUTPUT);
  pinMode(BUILTIN_RX_PIN, OUTPUT);

  loadWifiData();
  loadPinState();
  setLights(g_pin_state);
  
  if(!connectToWiFiNetwork()){
    setupAccessPoint();    
  }
//  server.begin();
//  Serial.println("Server started");

   doWork();

}

/**
 * Connect to wifi network
 */
int connectToWiFiNetwork(){

  bool rc = false;

  Serial.print("WL_IDLE_STATUS:");
  Serial.println(WL_IDLE_STATUS);
  Serial.print("WL_NO_SSID_AVAIL:");
  Serial.println(WL_NO_SSID_AVAIL);
  Serial.print("WL_SCAN_COMPLETED:");
  Serial.println(WL_SCAN_COMPLETED);  
  Serial.print("WL_CONNECTED:");
  Serial.println(WL_CONNECTED);
  Serial.print("WL_CONNECT_FAILED:");
  Serial.println(WL_CONNECT_FAILED);
  Serial.print("WL_CONNECTION_LOST:");
  Serial.println(WL_CONNECTION_LOST);  
  Serial.print("WL_DISCONNECTED:");
  Serial.println(WL_DISCONNECTED);
  Serial.print("WL_NO_SHIELD:");
  Serial.println(WL_NO_SHIELD);

  // Connect to WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.print(g_settings.ssid);

  int attempts = CONNECT_TIMEOUT_MS / CONNECT_INTERVAL_MS;
  Serial.print(" with password:");
  Serial.print(g_settings.password);
  Serial.print(" for ");
  Serial.print(CONNECT_TIMEOUT_MS);
  Serial.print("ms (");
  Serial.print(attempts);
  Serial.println(") attempts");

  //Now turn on WiFi at last possible minute
  WiFi.forceSleepWake();
  delay( 1 );
  WiFi.begin(g_settings.ssid, g_settings.password);
  WiFi.config(DEFAULT_STATIC_IP, DEFAULT_STATIC_IP_GATEWAY, DEFAULT_STATIC_IP_SUBNET);

  while ((WiFi.status() != WL_CONNECTED) && (attempts > 0)) {
    delay(CONNECT_INTERVAL_MS);
    Serial.print(".");
    Serial.print(WiFi.status());

    --attempts;
  }
  Serial.println(WiFi.status());

  if(WiFi.status() == WL_CONNECTED) {
     Serial.println("");
     Serial.println("WiFi connected");
  
     // Print the IP address
     Serial.println(WiFi.localIP());    
     rc = true;
  } else {
     Serial.println("");
     Serial.print("Unable to connect to ");
     Serial.print(g_settings.ssid);
     Serial.print(" using ");
     Serial.println(g_settings.password);
     WiFi.disconnect();
  }

  return rc;
}


/**
 * Sets up the wireless access point, so the ESP8266 can be accessed with no wifi details
 */
void setupAccessPoint(){

  WiFi.softAPConfig(AP_LOCAL_IP, AP_GATEWAY, AP_SUBNET);

  Serial.print("Starting as Access Point: Connect to ");
  Serial.print(AP_SSID);
  Serial.print(" using ");
  Serial.println(AP_PASS);
  
  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, /*channel no*/ NULL, /*hidden ssid*/ NULL, AP_MAX_CON);
//  Serial.print("IP address for network ");
//  Serial.print(AP_SSID);
//  Serial.print(" : ");

  Serial.println("Then access the URL:");
  Serial.print("http://");
  Serial.print(WiFi.softAPIP());
  Serial.println(SET_WIFI_URL);
  //Serial.println("/newssid$newpassword");
  Serial.println("And set new password.");

  uint8_t macAddr[6];
  WiFi.softAPmacAddress(macAddr);
  Serial.printf("MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);

}

/**
 * Sets up the wireless access point, so the ESP8266 can be accessed with no wifi details
 */
void removeAccessPoint(){

  Serial.print("Removing Access Point.");  
  WiFi.softAPdisconnect (true);
  WiFi.mode(WIFI_AP);  
}


/**
 * Stores the WiFi config in EEPROM
 */
//void saveWifiData(String& aSSID, String& aPass) {
void saveWifiData() {
  
  Serial.println("Saving WiFi Config to EEPROM.");
//  Serial.print("   WIFI_DATA_START_ADDR:");
//  Serial.println(WIFI_DATA_START_ADDR);
  Serial.print("   G_SETTINGS_SSID:");
  Serial.println(g_settings.ssid);
  Serial.print("   G_SETTINGS_PASSWORD:");
  Serial.println(g_settings.password);
//  Serial.print("   G_PIN_STATE:");
//  Serial.println(g_pin_state);


  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put (WIFI_DATA_START_ADDR, g_settings);
//  EEPROM.put (PIN_DATA_START_ADDR, g_pin_state);
  EEPROM.commit();
  EEPROM.end();
}

/**
 * Stores PIN State in EEPROM
 */
void savePinState() {

//  Serial.print("   PIN_DATA_START_ADDR:");
//  Serial.println(PIN_DATA_START_ADDR);
  Serial.print("Saving PIN State:");
  Serial.println(g_pin_state);

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.put (PIN_DATA_START_ADDR, g_pin_state);
  EEPROM.commit();
  EEPROM.end();
}

/**
 * Stores PIN State in EEPROM
 */
void loadPinState() {

//  Serial.print("PIN_DATA_START_ADDR:");
//  Serial.println(PIN_DATA_START_ADDR);
  Serial.print("Loading PIN State:");

  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get( PIN_DATA_START_ADDR, g_pin_state);
  EEPROM.end();
  Serial.println(g_pin_state);  
}

/**
 * Reads in from EEPROM Memory the WIFI settings
 */
void loadWifiData(){

  Serial.println("Loading config");
  storeStruct_t load;
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.get( WIFI_DATA_START_ADDR, load);
//  EEPROM.get( PIN_DATA_START_ADDR, g_pin_state);
  EEPROM.end();

//  Serial.print("Data Size:");
//  Serial.println(sizeof(load));
  
  Serial.println("WiFi Config from EEPROM");
  Serial.println("-----------------------");
  Serial.print("SSID:");
  Serial.println(load.ssid);
  Serial.print("Password:");
  Serial.println(load.password);
  Serial.println("-----------------------");
  Serial.print("size:");
  Serial.println(sizeof(load));
  Serial.println("-----------------------");
//  Serial.print("PIN State:");
//  Serial.println(g_pin_state);
//  Serial.println("-----------------------");

  g_settings = load;  
}

/**
 * Gets the number of occurrances of a char in a given string
 */
int getNumOccurrances(char* aString, const char* aChar){
  int i = 0;
  char *pch=strchr(aString, *aChar);

  while (pch!=NULL) {
    i++;
    pch=strchr(pch+1, *aChar);
  }

  return i;
}

/**
 * Splits a string into an array
 */
void split(char* aString, const char *aSeparator, char *aArray[], const int aMax){

  int i=0;
  char* token = strtok(aString, aSeparator);

  while ((token != NULL) && (i<=aMax)){
//    Serial.print("token found:");
//    Serial.print(token);
//    Serial.print(" at ");
//    Serial.println(i);       
    aArray[i] = token;
    token = strtok(0, aSeparator);
    ++i;
  }
  
  return;
}

/**
 * Gets the HTTP Header
 */
void getHeader(WiFiClient& aClient, int& aBodySize){

   bool header_found = false;
   int num_lines = 0;
   int max_header_line = 20;

   while(!header_found && num_lines < max_header_line){
      String hdr_line = aClient.readStringUntil('\r');

      Serial.print("HEADER LINE:");
      Serial.println(hdr_line);

      char* hdr_line_elements[2];
      int hdr_line_len = hdr_line.length();
      char hdr_char_buf[hdr_line_len+1];
      hdr_line.toCharArray(hdr_char_buf, hdr_line_len+1);
      
      split(hdr_char_buf, ":", hdr_line_elements, 2);

      if(NULL != strstr(hdr_line_elements[0], HTTP_CONTENT_LEN_HDR)){
//      if(-1 != *header_elements[0].indexOf("Content-Length")){
         Serial.print("Found length:");
         Serial.println(hdr_line_elements[1]);      
         aBodySize = atoi(hdr_line_elements[1]);
      }        

      if (hdr_line=="\n"){
//         Serial.print("END OF HEADER:");
//         Serial.println(hdr_line);      
         header_found = true;
       }    
   }

}

void handleWifiSettings(WiFiClient& aClient){
     int body_size = 0;
     getHeader(aClient, body_size);

     bool found_ssid = false;
     bool found_password = false;

     char* result = "Failure";

     Serial.print("Body length:");
     Serial.println(body_size);      

     String body = aClient.readString(); 
     body.trim();
   
     Serial.print("Body:");
     Serial.println(body);      

     int body_len = body.length();
     char body_buf[body_len+1];
     body.toCharArray(body_buf, body_len+1);

     Serial.print("Len:");
     Serial.println(body_len);      

     int num_sep = getNumOccurrances(body_buf, POST_PARAM_SEPARATOR);
     Serial.print("Num occurances of '");
     Serial.println(POST_PARAM_SEPARATOR);      
     Serial.print("':");
     Serial.println(num_sep);      
     if (num_sep > 0){
        char* parameters[num_sep+1];
        split(body_buf, POST_PARAM_SEPARATOR, parameters, num_sep);

         Serial.print("Looping:");
         Serial.println(num_sep+1);      
        for(int i=0; i<(num_sep+1); i++){
           int kv_sep = getNumOccurrances(parameters[i], POST_KEY_VALUE_SEPARATOR);

           Serial.print("Num occurances of '");
           Serial.print(POST_KEY_VALUE_SEPARATOR);      
           Serial.print("' in ");
           Serial.print(parameters[i]);
           Serial.print(" :");
           Serial.println(kv_sep);      
           
           if (kv_sep > 0){
              char* key_value[2];
              split(parameters[i], POST_KEY_VALUE_SEPARATOR, key_value, 1);

              Serial.print("WIFI_SSID:");
              Serial.println(WIFI_SSID);
              Serial.print("key_value[0]:");
              Serial.println(key_value[0]);

              if(0==strcmp(WIFI_SSID, key_value[0])){
                 Serial.print("Found SSID:");
//                 g_ssid = key_value[1];
//                 Serial.print("SSID:");
                 Serial.println(key_value[1]);
                 strcpy(g_settings.ssid, key_value[1]);
                 found_ssid = true;
          
              } else if (0==strcmp(WIFI_PASSWORD, key_value[0])){
                 Serial.print("Found Password:");
//                 g_password = key_value[1];        
//                 Serial.print("Password:");
                 Serial.println(key_value[1]);
                 strcpy(g_settings.password, key_value[1]);
                 found_password = true;
              }
           }
        }
     }

//     Serial.print("G_SSID:");
//     Serial.println(g_ssid);
//     Serial.print("G_Password:");
//     Serial.println(g_password);
     Serial.print("G_SETTINGS_SSID:");
     Serial.println(g_settings.ssid);
     Serial.print("G_SETTINGS_PASSWORD:");
     Serial.println(g_settings.password);


     if (found_ssid && found_password ){
//     saveWifiData(g_settings.ssid, g_settings.password);
        saveWifiData();
        result = "Success";
     }
     
     //JSONVar json;
     DynamicJsonDocument json(1024);
     json["wifi-reset"] = result;

     // Prepare the response
     String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
//     s += JSON.stringify(json);
     serializeJson(json, s); //Appends to end
     s += "\r\n";

     aClient.flush();
     aClient.print(s);
     delay(10);
       
     ESP.restart();
}

/**
 * Handles all Put Requests
 */
void handlePost(WiFiClient& aClient, char* end_point){

  Serial.print("end_point:");
  Serial.println(end_point);      
  Serial.print("SET_WIFI_DETAILS_COMMAND:");
  Serial.println(SET_WIFI_DETAILS_COMMAND);      
  if(0==strcmp(SET_WIFI_DETAILS_COMMAND, end_point)){
     handleWifiSettings(aClient);
  } else {
     handleUnsupported(aClient, end_point);    
  }
}
/**
 * Handles all Put Requests
 */
void handlePut(WiFiClient& aClient, char* end_point){
  int num_seps = getNumOccurrances(end_point, END_POINT_SEPARATOR);

  char* ep_commands[num_seps];
  split(end_point, END_POINT_SEPARATOR, ep_commands, num_seps);

  if (0==strcmp(LIGHTS_COMMAND, ep_commands[0])){
    handleLightsChange(aClient, ep_commands);
  } else {
    handleUnsupported(aClient, end_point);    
  }
  
}

/**
 * Handles all Get Requests
 */
void handleGet(WiFiClient& aClient, char* end_point){
  int num_seps = getNumOccurrances(end_point, "/");
  Serial.print("num_seps:");
  Serial.println(num_seps);
  
  char* ep_commands[num_seps];
  split(end_point, "/", ep_commands, num_seps);

  if(0==strcmp(ADMIN_COMMAND, ep_commands[0])){
    handleAdminView(aClient, ep_commands);
  } else if (0==strcmp(LIGHTS_COMMAND, ep_commands[0])){
    handleLightsView(aClient, ep_commands);
  } else {
    handleUnsupported(aClient, end_point);    
  }

}

/**
 * Creates WiFi SSID/ Password details page
 */
void showWifiChangeForm(WiFiClient& aClient){
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";

  s += "<form action=\"";
  s += SET_WIFI_DETAILS_COMMAND;
  s += "\" method=\"post\">"
        "   <ul>"
        "     <li>"
        "       <label for=\"name\">SSID:</label>"
        "       <input type=\"text\" id=\"";
  s += WIFI_SSID;
  s += "\" name=\"";
  s += WIFI_SSID;
  s += "\" />"
        "     </li>"        
        "     <li>"
        "        <label for=\"mail\">Password:</label>"
        "       <input type=\"text\" id=\"";
  s += WIFI_PASSWORD;
  s += "\" name=\"";
  s += WIFI_PASSWORD;
  s += "\" />"
//        "        <input type=\"email\" id=\"mail\" name=\"user_email\" />"
        "     </li>"


        "     <li>"
        "        <label for=\"mail\">Static IP:</label>"
        "       <input type=\"text\" id=\"";
  s += STATIC_IP;
  s += "\" name=\"";
  s += STATIC_IP;
  s += "\" />"
//        "        <input type=\"email\" id=\"mail\" name=\"user_email\" />"
        "     </li>"
        "     <li>"
        "        <label for=\"mail\">Gateway:</label>"
        "       <input type=\"text\" id=\"";
  s += STATIC_GATEWAY_IP;
  s += "\" name=\"";
  s += STATIC_GATEWAY_IP;
  s += "\" />"
//        "        <input type=\"email\" id=\"mail\" name=\"user_email\" />"
        "     </li>"
        "     <li>"
        "        <label for=\"mail\">Subnet Mask:</label>"
        "       <input type=\"text\" id=\"";
  s += STATIC_SUBNET_MASK;
  s += "\" name=\"";
  s += STATIC_SUBNET_MASK;
  s += "\" />"
//        "        <input type=\"email\" id=\"mail\" name=\"user_email\" />"
        "     </li>"
        

        "     <li class=\"button\">"
        "        <button type=\"submit\">Set</button>"
        "     </li>"
        "   </ul>"
        "</form>";

  aClient.flush();
  aClient.print(s);

}

/**
 * Handles admin commands
 */
void handleAdminView(WiFiClient& aClient, char** aCommandArray){
//  Serial.print(*aCommandArray);  
//  Serial.println(" is supported");  

//  JSONVar json;
//  DynamicJsonDocument json(1024);
  if(0==strcmp(WIFI, aCommandArray[1])){
    showWifiChangeForm(aClient);     
//    Serial.println("WIFI:");
  } else if (0==strcmp(VCC, aCommandArray[1])){

     handleVccView(aClient);
//    vcc = ESP.getVcc()/1000.00;
//    Serial.print("VCC:");
//    Serial.println(vcc);
    
  } else {
    handleUnsupported(aClient, aCommandArray[1]);    
  }

}

/**
 * Handles lights commands
 */
void handleLightsChange(WiFiClient& aClient, char** aCommandArray){
  Serial.print("Lights management. Checking:");  
  Serial.println(aCommandArray[1]);  

  int val = 0;

  DynamicJsonDocument json(1024);
//  JSONVar json;
  char *status = "unknown";
  if(0==strcmp(LIGHTS_ON, aCommandArray[1])){
     status = "on";
     setLights(true);
//     g_pin_state = true;
//     val = 1;
  } else if (0==strcmp(LIGHTS_OFF, aCommandArray[1])){
    status = "off";
    setLights(false);
//    g_pin_state = false;
//    val = 0;
  } else {
    handleUnsupported(aClient, aCommandArray[1]);    
  }


//  digitalWrite(BUILTIN_LED_PIN, val);
//  digitalWrite(BUILTIN_TX_PIN, val);
//  digitalWrite(BUILTIN_RX_PIN, val);
//  savePinState();
  json["lights"] = status;

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
//  s += JSON.stringify(json);
  serializeJson(json, s); //Appends to end
  s += "\r\n";

  Serial.print("JSON:");  
//  Serial.println(json); 
  serializeJson(json,Serial);  
  Serial.println(s);  

  // Send the response to the client
  aClient.flush();
  aClient.print(s);
  //aClient.print(JSON.stringify(json));

}

/**
 * Sets the PIN to be the state
 */
void setLights(bool aState){
   g_pin_state = aState;
   Serial.println("Setting Lights");    

   int val = 0;
   if(g_pin_state){
      val = 1;
   }
     
//  digitalWrite(BUILTIN_LED_PIN, val);
//  digitalWrite(BUILTIN_TX_PIN, val);
  digitalWrite(BUILTIN_RX_PIN, val);
   Serial.println("Saving State");    
   savePinState();  
   Serial.println("Loading State");    
   loadPinState();        
   
   Serial.print("setLights:Lights are ");    
   Serial.println(val);    

}


/**
 * Handles lights commands
 */
void handleLightsView(WiFiClient& aClient, char** aCommandArray){
  Serial.print("Lights management. Checking:");  
  Serial.println(aCommandArray[1]);  

  int val = 0;

  DynamicJsonDocument json(1024);
//  JSONVar json;
  char *status = "unknown";
  if (0==strcmp(LIGHTS_STATUS, aCommandArray[1])){
     status = "off";
     if (g_pin_state == true){
        status = "on";     
     } 
  } else {
    handleUnsupported(aClient, aCommandArray[1]);    
  }

  json["lights"] = status;

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
//  s += JSON.stringify(json);
  serializeJson(json, s); //Appends to end
  s += "\r\n";

  Serial.print("JSON:");  
//  Serial.println(json);  
  serializeJson(json, Serial); //Appends to end
  Serial.println(s);  

  // Send the response to the client
  aClient.flush();
  aClient.print(s);
  //aClient.print(JSON.stringify(json));

}

/**
 * Handles lights commands
 */
void handleVccView(WiFiClient& aClient){
  Serial.print("Input Voltage. Checking:");  
//  Serial.println(aCommandArray[1]);  

  int val = 0;

//  JSONVar json;
  DynamicJsonDocument json(1024);
//  char *status = "unknown";
//  if (0==strcmp(LIGHTS_STATUS, aCommandArray[1])){
//     status = "off";
//     if (g_pin_state == true){
//        status = "on";     
//     } 
//  } else {
//    handleUnsupported(aClient, aCommandArray[1]);    
//  }
  float vcc = ESP.getVcc()/1000.00;
  Serial.print("VCC:");
  Serial.println(vcc);

  json["vcc"] = vcc;

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n";
//  s += JSON.stringify(json);
  serializeJson(json, s); //Appends to end
  s += "\r\n";

  Serial.print("JSON:");  
//  Serial.println(json);  
  serializeJson(json,Serial); 
  Serial.println(s);  

  // Send the response to the client
  aClient.flush();
  aClient.print(s);
  //aClient.print(JSON.stringify(json));

}


//void callback(char* topic, byte* payload, unsigned int length) {
//   Serial.print("Message arrived on topic: '");
//   Serial.print(topic);
//   Serial.print("' with payload: ");
//   for (unsigned int i = 0; i < length; i++) {
//      Serial.print((char)payload[i]);
//   }
//
//   Serial.println();
//   String myCurrentTime = timeClient.getFormattedTime();
//   mqttClient.publish(mqttTopicOut,("Time: " + myCurrentTime).c_str());
//}

/**
 * Handles requests that are not supported, such as POST. PATCH, DELETE
 */
void handleUnsupported(WiFiClient& aClient, char* command){
  Serial.print(command);  
  Serial.println(" is not supported");  
}


/*****************************************************************
 * 
 * This is the main loop. This runs once the setup is complete
 * 
 */
void loop() {
//  doWork();
}

void doWork() {

//   client.loop(); // Starte den MQTT Client im loop

/***********************************************************************************************/

//   Serial.print("connecting to "); 
//   Serial.println(host);
//
//   // Use WiFiClient class to create TCP connections
//   WiFiClient client;
//   const int httpPort = 80;
//   if (!client.connect(host, httpPort)) {
//      Serial.println("connection failed");
//      return;
//   }
//
//   // We now create a URI for the request
//   String url = "/input/";
//   url += streamId;
//   url += "?private_key=";
//   url += privateKey;
//   url += "&value=";
//   url += value;
//
//   Serial.print("Requesting URL: ");
//   Serial.println(url);
//
//   // This will send the request to the server
//   client.print(String("GET ") + url + " HTTP/1.1\r\n" +
//      "Host: " + host + "\r\n" +
//      "Connection: close\r\n\r\n");
//   delay(10);
//
//   // Read all the lines of the reply from server and print them to Serial
//   while(client.available()){
//      String line = client.readStringUntil('\r');
//      Serial.print(line);
//   }
//
//   Serial.println();
//   Serial.println("closing connection");

/***********************************************************************************************************/

   WiFiClient wifi_client;  // or WiFiClientSecure for HTTPS
   HTTPClient http;

/******************************************** MQTT ******************************************************/
   const char* mqtt_server = "192.168.0.124";
   const uint16_t mqtt_server_port = 1883; 
   const char* mqtt_user = "alex";
   const char* mqtt_password = "Pl@5m0d!um";
   const char *topic = "esp8266/db5";
   
//   PubSubClient mqtt_client(wifi_client);
//   mqtt_client.setServer(mqtt_server, mqtt_server_port);
//
//   while (!mqtt_client.connected()) {
//      String client_id = "esp8266-client-";
//      client_id += String(WiFi.macAddress());
//      Serial.printf("The client %s connects to the public mqtt broker\n", client_id.c_str());
//      if (mqtt_client.connect(client_id.c_str(), mqtt_user, mqtt_password)) {
//      } else {
//         Serial.print("failed with state ");
//         Serial.print(mqtt_client.state());
//         delay(2000);
//      }
//   }
//   mqtt_client.publish(topic, "hello emqx");

/*******************************************************************************************************/

   // Send request
   const char *server_url = "http://192.168.0.124:3000/db5/light/status";

//   http.useHTTP10(true);
   Serial.printf("Sending request to %s\n", server_url);
   http.begin(wifi_client, server_url);
//   http.begin(wifi_client, "http://192.168.0.124:3000/db5/light/status");
   Serial.print("Calling GET:");
   int resp_code = http.GET();
   Serial.println(resp_code);

   if(resp_code > 0) {
      // Print the response

      String http_response = http.getString();
      Serial.print("Response:");
      Serial.println(http_response);

      
//      Serial.print("Response[0]:");
//      Serial.println(http_response[0]);

      if (http_response.length() > 3) { // remove UTF8 BOM
         if (http_response[0] == char(0xEF) && http_response[1] == char(0xBB) && http_response[2] == char(0xBF)) {
            Serial.println("remove BOM from JSON");
            http_response = http_response.substring(3);
         }
      }
      

//      Serial.println("Here1");


      DynamicJsonDocument json(1024);
//      deserializeJson(json, http_response);
//      DeserializationError deserialisation_error = deserializeJson(json, http.getStream());
      DeserializationError deserialisation_error = deserializeJson(json, http_response);
      //JsonObject& root = jsonBuffer.parseObject(http.getString());

      Serial.print("DeserializationError::Ok:");
      Serial.println(DeserializationError::Ok);
      Serial.print("DeserializationError::InvalidInput:");
      Serial.println(DeserializationError::InvalidInput);
      Serial.print("DeserializationError::NoMemory:");
      Serial.println(DeserializationError::NoMemory);
      Serial.print("DeserializationError::EmptyInput:");
      Serial.println(DeserializationError::EmptyInput);
      Serial.print("DeserializationError::IncompleteInput:");
      Serial.println(DeserializationError::IncompleteInput);
      Serial.print("DeserializationError::TooDeep:");
      Serial.println(DeserializationError::TooDeep);
      
      Serial.print("deserialisation_error:");
      Serial.println(deserialisation_error.code());

      if(deserialisation_error.code() == DeserializationError::Ok){

         const char* state = json["lights"];

         Serial.print("JSON:");
         Serial.println(state);
      
         if(0 == strcmp(json["lights"], "on")){
            Serial.println("Server says lights are on");
            setLights(true); 
         } else if (0 == strcmp(json["lights"], "off")){
            Serial.println("Server says lights are off");
            setLights(false); 
         }
      }
    
   }else {
      Serial.println("Error Connecting");    
   }

   http.end();

//   deserializeJson(json_doc, http.getString());

//   if(!root.success()) {
//     Serial.println("parseObject() failed");
//     return false;
//   }

   // Disconnect

//   mqtt_client.publish("mytopic/test", "This is a message");


//   int loop_delay = 60000;
//   int loop_delay_ms = 60000;
   int loop_delay_ms = 10000;
   int led_duration_ms = 500;
   digitalWrite(BUILTIN_LED_PIN, 0);
   delay(led_duration_ms);
   digitalWrite(BUILTIN_LED_PIN, 1);

//   Serial.print("Sleeping for ");
//   Serial.print((loop_delay - led_duration));
//   Serial.println("ms");
//   delay(loop_delay - led_duration);


   gpio_hold_en((gpio_num_t)BUILTIN_RX_PIN);


//   long sp_duration = 10e6;
   long sp_duration_us = loop_delay_ms * 1000;
   Serial.print("Deep Sleep for ");
   Serial.print((sp_duration_us - (led_duration_ms * 1000)));
   Serial.println("us");
   ESP.deepSleep((sp_duration_us - (led_duration_ms * 1000))); // 20e6 is 20 microseconds


// // Check if a client has connected
//  WiFiClient client = server.available();
//
//  if (!client) {
//    delay(1);
//    return;
//  }
//  client.setTimeout(WIFI_CLIENT_READ_TIMEOUT_MS);
// 
//  // Wait until the client sends some data
////  Serial.print(client.localIP());
//  Serial.print("\nNew client connected from ");
//  Serial.print(client.remoteIP());
//  
//  while(!client.available()){
//    delay(1);
//  }
//  
//  // Read the first line of the request
//  String req = client.readStringUntil('\r');
//  Serial.print("REQUEST:");
//  Serial.println(req);
////  client.flush();
//
//  int req_len = req.length();
//  char req_char_buf[req_len+1];
//
//  req.toCharArray(req_char_buf, req_len+1);
//  int num_seps = getNumOccurrances(req_char_buf, HTTP_LINE_SEPARATOR);
//  Serial.print("num_seps:");
//  Serial.println(num_seps);
//
//
//  if (2 <= num_seps){
//    char* components[num_seps+1];  
//    split(req_char_buf, HTTP_LINE_SEPARATOR, components, num_seps);
//  
//    char* command = components[0];
//    char* end_point = components[1];
//  
//    Serial.print("COMMAND:");
//    Serial.println(command);
//    Serial.print("End Point:");
//    Serial.println(end_point);
//  
//    if(0==strcmp(REST_GET, command)){
//      handleGet(client, end_point);
//    } else if (0==strcmp(REST_PUT, command)){
//      handlePut(client, end_point);
//    } else if (0==strcmp(REST_POST, command)){
//      handlePost(client, end_point);
//    } else {
//      handleUnsupported(client, command);    
//    }
//  }
//   
//  // Match the request
//  //int val;
//
//  client.flush();
//
//  delay(1);
//  Serial.println("Client disconnected");
//
//  // The client will actually be disconnected 
//  // when the function returns and 'client' object is detroyed
}

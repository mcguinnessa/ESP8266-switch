/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO2 low,
 *    http://server_ip/gpio/1 will set the GPIO2 high
 *  server_ip is the IP address of the ESP8266 Arduino module, will be 
 *  printed to Serial when the module is connected.
 */

#include <ESP8266WiFi.h>

#include <EEPROM.h> //The EEPROM libray 

#define WIFI_DATA_START_ADDR 0
#define WIFI_DATA_SEP $

#define WIFI_SSID_LEN 50
#define WIFI_PWD_LEN 50

// Set Access Point credentials
#define AP_SSID "ESP8266"
#define AP_PASS "bruckfexit"
#define AP_MAX_CON 1


const char* url_lights_stub = "/lights/";
//const char* url_admin_wifi_set = "/admin/wifi";
const String url_admin_wifi_set = "/admin/wifi";

char on_url[80];
char off_url[80];

const char* ssid = "Anfield";
const char* password = "1985Dalglish1991";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);

byte BUILTIN_LED_PIN=2;

byte BUILTIN_TX_PIN=1;
byte BUILTIN_RX_PIN=3;

struct storeStruct_t{
  char myVersion[3];
  char ssid[WIFI_SSID_LEN];
  char password[WIFI_PWD_LEN];
};

storeStruct_t settings = {
  "01",
  "Anfield2"
  "Password"
};

void setup() {
  String in_string_ssid;
  String out_string_ssid;

  
  Serial.begin(115200);
  delay(10);
  Serial.print("\nStarting...");

  // prepare GPIO2
  pinMode(BUILTIN_LED_PIN, OUTPUT);
  pinMode(BUILTIN_TX_PIN, OUTPUT);
  pinMode(BUILTIN_RX_PIN, OUTPUT);
  digitalWrite(BUILTIN_LED_PIN, 0);

  digitalWrite(BUILTIN_TX_PIN, 0);
  digitalWrite(BUILTIN_RX_PIN, 0);

  


  /** WIFI STORAGE ***************/
//  in_string_ssid=String(ssid);
//  in_string_ssid=string_ssid+";";
//
//  //Write one char at a time
//  EEPROM.begin(512);  
//  for(int n=0; n< string_ssid.length();n++){
//    EEPROM.write(n,string_ssid[n]);
//  }
//  EEPROM.commit();

  /** WIFI STORAGE ***************/


//  saveWifiData();
  loadWifiData();


////  for (uint8_t n = p; n < l+p; ++n) {
//  for (int n = 0; n < 100; ++n) {
//    Serial.print("readmem - ");
//    Serial.println(n);
//    if(char(EEPROM.read(n))!=';'){
//      temp += String(char(EEPROM.read(n)));      
//     }
//  }
//
//  
//  for(int i=0;i<14;i++) 
//  {
//    out_string_ssid = strText + char(EEPROM.read(0x0F+i)); 
//  }

  setupAccessPoint();

  
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  
  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());


  strcpy(on_url, url_lights_stub);
  strcat(on_url, "on");
  strcpy(off_url, url_lights_stub);
  strcat(off_url, "off");
}

void setupAccessPoint(){

  IPAddress local_IP(192,168,4,1);
  IPAddress gateway(192,168,4,9);
  IPAddress subnet(255,255,255,0);
  WiFi.softAPConfig(local_IP, gateway, subnet);
  
  // Begin Access Point
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(AP_SSID, AP_PASS, /*channel no*/ NULL, /*hidden ssid*/ NULL, AP_MAX_CON);
  Serial.print("IP address for network ");
  Serial.print(AP_SSID);
  Serial.print(" : ");
  Serial.print(WiFi.softAPIP());

  uint8_t macAddr[6];
  WiFi.softAPmacAddress(macAddr);
  Serial.printf("MAC address = %02x:%02x:%02x:%02x:%02x:%02x\n", macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);


}

void saveWifiData(String& aSSID, String& aPass) {
  
  Serial.println("Saving config");
  aSSID.toCharArray(settings.ssid, WIFI_SSID_LEN);
  aPass.toCharArray(settings.password, WIFI_PWD_LEN);

//  settings.ssid=aSSID.toCharArray();
//  settings.password = aPass;

  EEPROM.begin(512);
  EEPROM.put (WIFI_DATA_START_ADDR, settings);
  EEPROM.commit();
  EEPROM.end();
}

void loadWifiData(){

  Serial.println("Loading config");
  storeStruct_t load;
  EEPROM.begin(512);
  EEPROM.get( WIFI_DATA_START_ADDR, load);
  EEPROM.end();

//  Serial.print("Version:");
//  Serial.println(load.myVersion);
  Serial.print("Name:");
  Serial.println(load.ssid);
  Serial.print("Password:");
  Serial.println(load.password);

  settings = load;
  
}

////Reads a string out of memory
//String read_string(uint8_t l, uint8_t p){
//  String temp;
//  for (uint8_t n = p; n < l+p; ++n)
//    {
//      Serial.print("readmem - ");
//      Serial.println(n);
//     if(char(EEPROM.read(n))!=';'){
//        if(isWhitespace(char(EEPROM.read(n)))){
//          //do nothing
//        }else temp += String(char(EEPROM.read(n)));
//      
//     }else n=l+p;
//     
//    }
//  return temp;
//}

void loop() {
 // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }
  
  // Wait until the client sends some data
  Serial.print(client.localIP());
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }
  
  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();
  
  // Match the request
  int val;

//  Serial.print("Does it match ON:");
//  Serial.println(on_url);
//  Serial.print("Does it match OFF:");
//  Serial.println(off_url);

  Serial.print("URL:");
  Serial.println(req);
  if (req.indexOf(off_url) != -1)
    val = 1;
  else if (req.indexOf(on_url) != -1)
    val = 0;
  else if (req.indexOf(url_admin_wifi_set) != -1){

    int idx = req.indexOf(url_admin_wifi_set);
    Serial.print("Wifi Admin request idx:");
    Serial.println(idx);

    String wifi_params = req.substring(url_admin_wifi_set.length()+idx+1);
    Serial.print("Data:");
    Serial.println(wifi_params);

    int ssid_pass_sep_idx = wifi_params.indexOf("$");
    Serial.print("SSID/PASS IDX:");
    Serial.println(ssid_pass_sep_idx);
    String url_ssid = wifi_params.substring(0, ssid_pass_sep_idx);
    String url_pass = wifi_params.substring(ssid_pass_sep_idx+1);
    Serial.print("SSID:");
    Serial.println(url_ssid);
    Serial.print("Password:");
    Serial.println(url_pass);

    saveWifiData(url_ssid, url_pass);
    
  } else {
    Serial.print("invalid request:");
    Serial.println(req);
    client.stop();
    return;
  }

  // Set GPIO2 according to the request
  digitalWrite(BUILTIN_LED_PIN, val);
  digitalWrite(BUILTIN_TX_PIN, val);
  digitalWrite(BUILTIN_RX_PIN, val);


  
  client.flush();

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
  s += (val)?"high":"low";
  s += "</html>\n";

  // Send the response to the client
  client.print(s);
  delay(1);
  Serial.println("Client disonnected");

  // The client will actually be disconnected 
  // when the function returns and 'client' object is detroyed
}

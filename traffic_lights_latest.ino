//////////////////////////////////////////////
//
//  Fully tested 1 FEB 2023
//
//  username        : admin
//  login_password  : 123456
//
// HARDWARE RESET function added
//
//
////////////////////////////////////////////


#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>

#ifndef APSSID
#define APSSID "TrafficLight"
#define APPSK  ""
#define APIP   "192.168.4.1"
#endif

#define RED                 1
#define YELLOW              2
#define GREEN               4

#define MODE_NORMAL         0
#define MODE_FLASH          1

#define BLOCKS_QTY          12

#define FLASH_SETTINGS_ADDR 0
#define SETTINGS_ID         0x01020304

#define PIN_RED             12    // D6
#define PIN_YELLOW          13    // D7
#define PIN_GREEN           15    // D8

#define PULSE_LENGTH_MIN_MS 100
#define PULSE_LENGTH_MAX_MS 5000

typedef struct
{
  uint8_t active;
  uint8_t colours;
  uint8_t flash_mode;
  uint8_t time;
  uint8_t qty;
  uint16_t speed;
} TMode;

typedef struct
{
  uint32_t id;
  char ssid[20];
  char password[20];
  char ip_addr[16];
  char username[20];
  char login_password[20];
  TMode modes[BLOCKS_QTY];
  uint8_t run;
} TSettings;

const char *ssid = APSSID;
const char *password = APPSK;

char html[4000];
uint8_t access_allowed = 0;

ESP8266WebServer server(80);

TSettings settings;

void settingsInitDefault (void)
{
  memset(&settings, 0, sizeof(settings));
  settings.id = SETTINGS_ID;
  sprintf(settings.ssid, APSSID);
  sprintf(settings.password, APPSK);
  sprintf(settings.ip_addr, APIP);
  sprintf(settings.username, "admin");
  sprintf(settings.login_password, "123456");
}

int settingsCheck (void)
{
  int result = 0;

  return result;
}

int settingsLoad (void)
{
  int result = 0;
  uint8_t * p_settings = (uint8_t *) &settings;

  for (int i = FLASH_SETTINGS_ADDR; i < sizeof(settings); i++)
    p_settings[i] = EEPROM.read(i);

  if (settings.id != SETTINGS_ID)
    result = -1;
  for (uint8_t i = 0; i < BLOCKS_QTY; i++)
  {
    if ((settings.modes[i].active != 0) && (settings.modes[i].active != 1))
      result = -1;
  }
  if ((settings.run != 0) && (settings.run != 1))
      result = -1;
  if (result)
  {
    Serial.println("No settings found");
    result = -1;
  }

  return result;
}

int settingsSave (void)
{
  int result = 0;
  uint8_t * p_settings = (uint8_t *) &settings;
  
  for (int i = FLASH_SETTINGS_ADDR; i < sizeof(settings); i++)
  {
     EEPROM.write(i, p_settings[i]);
     EEPROM.commit();
  }

  return result;
}

void settingsPrint (void)
{
  Serial.println("Settings:");
  Serial.print("\tSSID=");
  Serial.println(settings.ssid);
  Serial.print("\tPASSWORD=");
  Serial.println(settings.password);
  Serial.print("\tUSERNAME=");
  Serial.println(settings.username);
  Serial.print("\tLOGIN_PASSWORD=");
  Serial.println(settings.login_password);
  for (uint8_t i = 0; i < BLOCKS_QTY; i++)
  {
    Serial.print("\tBLOCK=");
    Serial.println(i);
    Serial.print("\t\tactive=");
    Serial.println(settings.modes[i].active);
    Serial.print("\t\tcolours=");
    Serial.println(settings.modes[i].colours);
    Serial.print("\t\tflash_mode=");
    Serial.println(settings.modes[i].flash_mode);
    Serial.print("\t\ttime=");
    Serial.println(settings.modes[i].time);
    Serial.print("\t\tqty=");
    Serial.println(settings.modes[i].qty);
    Serial.print("\t\tspeed=");
    Serial.println(settings.modes[i].speed);
  }
}

void handleRoot (void)
{
  snprintf(html,sizeof(html),
           #include "index.h"
           );
  server.send(200, "text/html", html);
  String addy = server.client().remoteIP().toString();
  Serial.print("Client IP: ");
  Serial.println(addy);
  access_allowed = 0;
}

void handleLogin (void)
{ // If a POST request is made to URI /login
  if( ! server.hasArg("username") || ! server.hasArg("password") || server.arg("username") == NULL || server.arg("password") == NULL)
  { // Нет имени или пароля в запросе
    server.send(400, "text/plain", "400: Invalid Request");
    handleNotFound();
    return;
  }
  if(server.arg("username") == settings.username && server.arg("password") == settings.login_password)
  { // Имя и пароль совпадают
    access_allowed = 1;
    handleTraffic();
  }
  else
  { // Username and password don't match
    handleRoot();
  }
}

void handleTraffic (void)
{
  if (access_allowed)
  {
    snprintf(html,sizeof(html),
             #include "traffic_head.h"
            );
    server.client().write((const char*)html, strlen(html));
    int cntr = 1;
    for (int i = 0; i < BLOCKS_QTY; i++)
    {
      if ( (settings.modes[i].active) || (!settings.modes[i].active && i == (BLOCKS_QTY - 1)) )
      {
        snprintf(html, sizeof(html),
                "\n\t\t<table style=\"background:%s\" class=\"rounded_MAIN\" align=\"center\">\
                  \n\t\t\t<tbody>\
                    \n\t\t\t\t<tr align=\"center\" height=\"50px\">\
                      \n\t\t\t\t\t<th><b>BLOCK %d</b></th>\
                    \n\t\t\t\t</tr>\
                    \n\t\t\t\t<tr>\
                      \n\t\t\t\t\t<td><input type=\"checkbox\" name=\"colour_red_%d\" value=\"red\" %s><b> RED</b></td>\
                      \n\t\t\t\t\t<td><input type=\"radio\" name=\"mode_%d\"  value=\"normal\" %s><b> NORMAL</b></td>\
                      \n\t\t\t\t\t<td><b>TIME, sec :</b></td>\
                      \n\t\t\t\t\t<td><input name=\"time_%d\" style=\"width:100px\" value=\"%d\" type=\"text\"></td>\
                      \n\t\t\t\t</tr>\
                    \n\t\t\t\t<tr>\
                      \n\t\t\t\t\t<td><input type=\"checkbox\" name=\"colour_yellow_%d\" value=\"yellow\" %s><b> YELLOW</b></td>\
                      \n\t\t\t\t\t<td><input type=\"radio\" name=\"mode_%d\"  value=\"flash\" %s><b> FLASH</b></td>\
                      \n\t\t\t\t\t<td><b>QUANTITY :</b></td>\
                      \n\t\t\t\t\t<td><input name=\"qty_%d\" style=\"width:100px\" value=\"%d\" type=\"text\"></td>\
                    \n\t\t\t\t</tr>\
                    \n\t\t\t\t<tr>\
                      \n\t\t\t\t\t<td><input type=\"checkbox\" name=\"colour_green_%d\" value=\"green\" %s><b> GREEN</b></td>\
                      \n\t\t\t\t\t<td></td>\
                      \n\t\t\t\t\t<td><b>SPEED :</b></td>\
                      \n\t\t\t\t\t<td><input name=\"speed_%d\" style=\"width:100px\" value=\"%d\" type=\"text\"></td>\
                    \n\t\t\t\t</tr>\
                    \n\t\t\t\t<tr>\
                      \n\t\t\t\t\t<td></td>\
                      \n\t\t\t\t\t<td align=\"right\"><input class=\"cellbut_TABLE\" type=\"submit\" value=\"%s\" name=\"add_remove_%d\"></td>\
                      \n\t\t\t\t\t<td align=\"left\"><input class=\"cellbut_TABLE\" type=\"submit\" value=\"REMOVE\" name=\"add_remove_%d\"></td>\
                    \n\t\t\t\t\t<td></td>\
                    \n\t\t\t\t</tr>\
                  \n\t\t\t</tbody>\
                \n\t\t</table>\
                <p></p>", settings.modes[i].active ? "PaleGreen" : "lightgrey", cntr, cntr, (settings.modes[i].colours & RED) ? "checked" : " ", cntr, (!settings.modes[i].flash_mode) ? "checked" : " ", cntr, settings.modes[i].time,
                           cntr, (settings.modes[i].colours & YELLOW) ? "checked" : " ", cntr, (settings.modes[i].flash_mode) ? "checked" : " ", cntr, settings.modes[i].qty,
                           cntr, (settings.modes[i].colours & GREEN) ? "checked" : " ", cntr, settings.modes[i].speed, settings.modes[i].active ? "UPDATE" : "ADD", cntr, cntr);
        server.client().write((const char*)html, strlen(html));
        cntr++;
      }
    }
    snprintf(html,sizeof(html),
           #include "traffic_tail.h"
           );
    server.client().write((const char*)html, strlen(html));
  }
  else
    server.send(404, "text/html", "<h1>ACCESS DENIED</h1>");
}

void handleBlockAddRemove (void)
{ // If a POST request is made to URI /add_remove
  char tmp[20];
  int i;
  
  for (i = 1; i <= BLOCKS_QTY; i++)
  {
    sprintf(tmp, "add_remove_%d", i);
    Serial.print("\n");
    Serial.print(tmp);
    if (server.hasArg(tmp))
    {
      Serial.print(" found");
      Serial.print("\nBlock ");
      Serial.print(i);
      Serial.println(" request");
      if ((server.arg(tmp) == "ADD") || (server.arg(tmp) == "UPDATE"))
      { // Добавить блок
        settings.modes[i-1].active = 1;
        settings.modes[i-1].colours = 0;
        sprintf(tmp, "colour_red_%d", i);
        if (server.hasArg(tmp) && server.arg(tmp) == "red")
          settings.modes[i-1].colours |= RED;
        sprintf(tmp, "colour_yellow_%d", i);
        if (server.hasArg(tmp) && server.arg(tmp) == "yellow")
          settings.modes[i-1].colours |= YELLOW;
        sprintf(tmp, "colour_green_%d", i);
        if (server.hasArg(tmp) && server.arg(tmp) == "green")
          settings.modes[i-1].colours |= GREEN;
        sprintf(tmp, "mode_%d", i);
        if (server.arg(tmp) == "flash")
          settings.modes[i-1].flash_mode = MODE_FLASH;
        else
          settings.modes[i-1].flash_mode = MODE_NORMAL;
        sprintf(tmp, "time_%d", i);
        if (server.hasArg(tmp))
          settings.modes[i-1].time = server.arg(tmp).toInt();
        sprintf(tmp, "qty_%d", i);
        if (server.hasArg(tmp))
          settings.modes[i-1].qty = server.arg(tmp).toInt();
        sprintf(tmp, "speed_%d", i);
        if (server.hasArg(tmp))
          settings.modes[i-1].speed = server.arg(tmp).toInt();
        if (settings.modes[i-1].speed < PULSE_LENGTH_MIN_MS)
          settings.modes[i-1].speed = PULSE_LENGTH_MIN_MS;
        if (settings.modes[i-1].speed > PULSE_LENGTH_MAX_MS)
          settings.modes[i-1].speed = PULSE_LENGTH_MAX_MS;
      }
      else
      { // Удалить блок
        settings.modes[i-1].active = 0;
      }
    }
  }

  if (server.hasArg("start"))
  {
    if (server.arg("start") == "START")
      settings.run = 1;
  }
  if (server.hasArg("stop"))
  {
    if (server.arg("stop") == "STOP")
      settings.run = 0;
  }
  
  handleTraffic();

  printRequest();

  // Собираем блоки в кучу, чтобы не было разрывов в массиве
  int last_empty = -1;
  for (i = 0; i < BLOCKS_QTY; i++)
  {
    if (settings.modes[i].active)
    {
      if (last_empty >= 0)
      {
        settings.modes[last_empty] = settings.modes[i];
        settings.modes[i].active = 0;
        last_empty = i;
      }
     }
     else
     {
      if (last_empty < 0)
        last_empty = i;
     }
  }

  settingsSave();
  settingsPrint();
}

void handleSettings (void)
{
  if (access_allowed)
  {
    snprintf(html,sizeof(html),
             "<html>\
                <head>\
                  <meta http-equiv=\"content-type\" content=\"text/html; charset=iso-8859-1\">\
                  <title>Settings</title>\
                </head>\
                <body>\
                  <form action=\"/save_settings\" method=\"post\" name=\"form\">\
                    <br>\
                    <style>\
                      td {\
                          padding: 5px;\
                          height: 40px;\
                          font-weight: bold;\
                          font-size: 18px;\
                         }\
                      .cellbut {\
                                width: 220px;\
                                border-radius: 10px;\
                                height: 40px;\
                                font-size: 18px;\
                                font-weight: bold;\ 
                               }\
                      .button {\
                                width: 220px;\
                                border-radius: 10px;\
                                height: 40px;\
                                font-size: 18px;\
                                font-weight: bold;\ 
                              }\
                      input[type=\"text\"]{\
                                           width: 220px;\
                                           border-radius: 10px;\
                                           height: 35px;\
                                           font-size: 18px;\
                                           font-weight: bold;\ 
                                           text-align: center;\
                                          }\   
                      table.roundedCorners1 {\ 
                                             background: #44bcd8;\
                                             width: 450px;\
                                             height: 80px;\
                                             border-radius: 10px;\
                                             border-spacing: 3px;\
                                             font-size: 18px;\
                                            }\                                           
                    </style>\
                    <table class=\"roundedCorners1\" align=\"center\">\
                    <tbody>\
                    <tr><td align=\"center\"><b>DEVICE SETTINGS</b></td>\
                    </tr></tbody>\
                    </table>\
                    <table class=\"table\" align=\"center\">\
                    <tbody>\
                    <tr><td align=\"center\"></td>\
                    </tr></tbody>\
                    </table>\
                    <table align=\"center\" border=\"0\">\
                    <tbody>\
                      <tr>\
                        <td align=\"right\"><input type=\"button\" class=\"button\" onclick=\"location.href=\'traffic.html\';\" value=\"BACK\" name=\"back\"></td>\
                        <td align=\"left\"><input class=\"cellbut\" type=\"submit\" value=\"SAVE\" name=\"save\"></td>\
                      </tr>\
                      <tr>\
                      <td align=\"center\"></td>\
                      </tr>\
                      <tr>\
                        <td><b>SSID :</b></td>\
                        <td> <input name=\"ssid\" type=\"text\" value=\"%s\"></td>\
                      </tr>\
                      <tr>\
                        <td><b>PASSWORD :</b></td>\
                        <td><input name=\"password\" type=\"text\" value=\"%s\"></td>\
                      </tr>\
                      <tr>\
                        <td><b>IP ADDRESS :</b></td>\
                        <td><input name=\"ip_address\" type=\"text\" value=\"%s\"></td>\
                      </tr>\
                      <tr>\
                        <td><b>USERNAME :</b></td>\
                        <td><input name=\"username\" type=\"text\" value=\"%s\"></td>\
                      </tr>\
                      <tr>\
                        <td><b>LOGIN PASSWORD :</b></td>\
                        <td><input name=\"login_password\" type=\"text\" value=\"%s\"></td>\
                      </tr>\
                    </tbody>\
                  </table>\
                </form>\
              </body>\
            </html>",
            settings.ssid, settings.password, settings.ip_addr, settings.username, settings.login_password);
    server.send(200, "text/html", html);
  }
  else
    server.send(404, "text/html", "<h1>ACCESS DENIED</h1>");
}

void handleSettingsSave (void)
{ // If a POST request is made to URI /login
  if (server.hasArg("ssid"))
    server.arg("ssid").toCharArray(settings.ssid, server.arg("ssid").length()+1);
  if (server.hasArg("password")) 
    server.arg("password").toCharArray(settings.password, server.arg("password").length()+1);
  if (server.hasArg("ip_address"))
    server.arg("ip_address").toCharArray(settings.ip_addr, server.arg("ip_address").length()+1);
  if (server.hasArg("username"))
    server.arg("username").toCharArray(settings.username, server.arg("username").length()+1);
  if (server.hasArg("login_password"))
    server.arg("login_password").toCharArray(settings.login_password, server.arg("login_password").length()+1);

  settingsSave();
  
  handleSettings();
  
  settingsPrint();
}

void printRequest (void)
{
  String message = "\nRequest\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  Serial.println(message);
}

void handleNotFound (void)
{
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }

  server.send(404, "text/plain", message);
  Serial.println(message);
}

void colourSet (uint8_t colour)
{
  if (colour & RED)
    digitalWrite(PIN_RED, HIGH);
  else
    digitalWrite(PIN_RED, LOW);

  if (colour & YELLOW)
    digitalWrite(PIN_YELLOW, HIGH);
  else
    digitalWrite(PIN_YELLOW, LOW);

  if (colour & GREEN)
    digitalWrite(PIN_GREEN, HIGH);
  else
    digitalWrite(PIN_GREEN, LOW);
    
  return;
}

int isColoursOn (void)
{
  if (digitalRead(PIN_RED) || digitalRead(PIN_YELLOW) || digitalRead(PIN_GREEN))
    return 1;
  else
    return 0;
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  Serial.println();
  Serial.print("Configuring access point...");

  pinMode(PIN_RED, OUTPUT);
  pinMode(PIN_YELLOW, OUTPUT);
  pinMode(PIN_GREEN, OUTPUT);

  pinMode(14, INPUT_PULLUP);  // GPIO14 , D5 input pin for factory reset

  EEPROM.begin(sizeof(settings));

  if (settingsLoad())
  {
    settingsInitDefault();
    settingsSave();
  }

  // Установка требуемого IP. IP по умолчанию 192.168.4.1
  char tmp[15];
  char * p_char;
  int ip[4], cntr = 0;
  strcpy(tmp, settings.ip_addr);
  p_char = strtok(tmp, ".");
  while (p_char && (cntr < 4))
  {
    sscanf(p_char, "%d", &(ip[cntr]));
    p_char = strtok(NULL, ".");
    cntr++;
  }
  Serial.print(ip[0]);Serial.print('.');Serial.print(ip[1]);Serial.print('.');Serial.print(ip[2]);Serial.print('.');Serial.println(ip[3]);
  IPAddress    apIP(ip[0], ip[1], ip[2], ip[3]);

  //WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));   // Маска подсети FF FF FF 00  
  
  WiFi.softAP(settings.ssid, settings.password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
  server.on("/", handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/traffic.html", handleTraffic);
  server.on("/add_remove", HTTP_POST, handleBlockAddRemove);
  server.on("/settings.html", handleSettings);
  server.on("/save_settings", HTTP_POST, handleSettingsSave);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");

  settingsPrint();

 if (digitalRead(14) == 0)
  {
    Serial.println(">>> FACTORY RESET <<<");

    settingsInitDefault();
    settingsSave();

   do {
      digitalWrite(PIN_RED, HIGH); digitalWrite(PIN_YELLOW, HIGH); digitalWrite(PIN_GREEN, HIGH); delay(200);  
      digitalWrite(PIN_RED, LOW); digitalWrite(PIN_YELLOW, LOW); digitalWrite(PIN_GREEN, LOW); delay(200);   
   } while(1);
  }



  
} // end of SETUP void

int block_ndx = BLOCKS_QTY, flash_qty;
uint32_t timer_ms, timer_flash_ms, timeout_ms;

void loop()
{
  server.handleClient();

  if (settings.run)
  {
    if (millis() >= timer_ms)
    { // Смена блока
      colourSet(0);
      if (++block_ndx >= BLOCKS_QTY)
        block_ndx = 0;
      if (settings.modes[block_ndx].active)
      { // Есть актуальный блок в массиве
        if (settings.modes[block_ndx].flash_mode == MODE_FLASH)
        {
          timer_ms = 0xFFFFFFFF;
          timer_flash_ms = 0;
          flash_qty = 0;
        }
        else
          timer_ms = millis() + settings.modes[block_ndx].time * 1000;
      }
    }
    else
    {
      if (settings.modes[block_ndx].flash_mode == MODE_FLASH)
      { // Режим мигания
        if (millis() >= timer_flash_ms)
        {
          timer_flash_ms = millis() + settings.modes[block_ndx].speed;
          
          if (!isColoursOn())
            colourSet(settings.modes[block_ndx].colours);
          else
          {
            colourSet(0);
            flash_qty++;
            if (flash_qty >= settings.modes[block_ndx].qty)
                timer_ms = 0;        
          }
        }
      }
      else
      { // Нормальный режим
        colourSet(settings.modes[block_ndx].colours);
      }
    }
  }
  else
  {
    colourSet(0);
    block_ndx = BLOCKS_QTY;
    timer_ms = 0;
    timer_flash_ms = 0;
    flash_qty = 0;
  }
  
  if (millis() >= timeout_ms)
  {
    timeout_ms = millis() + 900000;
    WiFiClient client = server.client();
    if (client)
    {
      if (!client.connected())
      {
        Serial.println("Client disconnected");
        access_allowed = 0;
      }
    }
    else
    {
      Serial.println("Client disconnected");
      access_allowed = 0;
    }
  }
}

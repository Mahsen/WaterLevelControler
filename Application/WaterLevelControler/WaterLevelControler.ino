#include <NTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
//////////////////////////////////////////////////////////////////////////////////////////////////////////
const int Pin_LED_MAIN =         2;
const int Pin_LED_Red =          9; 
const int Pin_LED_Green =        10; 
const int Pin_Valve =            4; 
const int Pin_Sense_Low =        12; 
const int Pin_Sense_High =       13; 
const int Pin_Buzzer =           14; 

String Request = "";
String Respond = "";
bool Value_Valve =               false;
bool Value_Sense_Low =           false;
bool Value_Sense_High =          false;
int  Valve_Time_On =            0;

struct struct_Log
{
  char Valid;
  unsigned long long DateTime;
  char State;
} Log;
const int Log_Locate_Max =       100; 
char Log_Locate =                0; 

struct struct_Config
{
  char serial[16+1];
  char ssid[32+1];
  char password[32+1];
  char local[16+1];
  char gateway[16+1];
  char subnet[16+1];
  int Validation;
} Config;

IPAddress local;
IPAddress gateway;
IPAddress subnet;

ESP8266WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 12600);
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void Set_Value_Valve(bool Value){
  Value_Valve = Value;
  digitalWrite(Pin_Valve, Value_Valve?HIGH:LOW);
  digitalWrite(Pin_LED_Red, Value_Valve?HIGH:LOW);
  digitalWrite(Pin_LED_Green, Value_Valve?LOW:HIGH);
}
bool Get_Value_Valve(void){
  return Value_Valve;
}
bool Get_Value_Sense_High(void)
{
  int Value = 0;
  for(int Index=0; Index<100; Index++)
  {
    Value += digitalRead(Pin_Sense_High);
    delay(10);
  }
  return (Value>60)?true:false;
}
bool Get_Value_Sense_Low(void)
{
  int Value = 0;
  for(int Index=0; Index<100; Index++)
  {
    Value += digitalRead(Pin_Sense_Low);
    delay(10);
  }
  return (Value>60)?true:false;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
String Handle(String Receive) {
  /*
  if(Receive.indexOf("ON") != -1)
  {
    digitalWrite(Pin_LED_CPU, LOW);
    Set_Value_LAMP(true);
    Serial.println("Set_Value_LAMP(true)");
    return "ON\r\n";
  }
  else if(Receive.indexOf("OFF") != -1)
  {
    digitalWrite(Pin_LED_CPU, HIGH);
    Set_Value_LAMP(false);
    Serial.println("Set_Value_LAMP(false)");
    return "OFF\r\n";
  }
  else if(Receive.indexOf("?") != -1)
  {
    return Get_Value_LAMP()?"ON\r\n":"OFF\r\n";
  }
*/
  return "";
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
unsigned long long RTC_DateTimeTo40Bit(int Year, int Month, int Day, int Hour, int Minute, int Second)
{
  unsigned long long _40Bit = 0;

  _40Bit |= ((((unsigned long long)(Year & 0x3FFF)) << 26) | \
      (((unsigned long long)(Month & 0xF)) << 22) | \
      (((unsigned long long)(Day & 0x1F)) << 17) | \
      (((unsigned long long)(Hour & 0x1F)) << 12) | \
      (((unsigned long long)(Minute & 0x3F)) << 6) | \
      (((unsigned long long)(Second & 0x3F)) << 0));
  
  _40Bit &= 0xFFFFFFFFFF;
  
  return _40Bit;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
/*RTC_DateTime RTC_40BitToDateTime(unsigned long long _40Bit)
{
  RTC_DateTime DateTime;

  DateTime.Date.SetYear((_40Bit >> 26) & 0x3FFF);
  DateTime.Date.SetMonth((_40Bit >> 22) & 0xF);
  DateTime.Date.SetDay((_40Bit >> 17) & 0x1F);
  
  DateTime.Time.SetHour((_40Bit >> 12) & 0x1F);
  DateTime.Time.SetMinute((_40Bit >> 6) & 0x3F);
  DateTime.Time.SetSecond((_40Bit >> 0) & 0x3F);
  
  return DateTime;
}*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void Log_Clear()
{
    for(char Index=0; Index<Log_Locate_Max; Index++)
    {
        Log.Valid = 0;
        Log.DateTime = 0;
        Log.State = 0;
        
        EEPROM.put(((sizeof(Log)*Index)+4), Log);
        EEPROM.commit();
    }
    EEPROM.put(0, 0);
    EEPROM.commit();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void Log_Add(bool State)
{
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    
    Log.Valid = 123;
    Log.DateTime = RTC_DateTimeTo40Bit(ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds());
    Log.State = State?1:0;

    EEPROM.put(((sizeof(Log)*Log_Locate)+4), Log);
    EEPROM.commit();
    Log_Locate++;
    if(Log_Locate >= Log_Locate_Max)
    {
        Log_Locate = 0;      
    }
    EEPROM.put(0, Log_Locate);
    EEPROM.commit();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
String Log_Get()
{
    String text = "";
    char buffer[64];

    for(char Index=0; Index<Log_Locate_Max; Index++)
    {
        EEPROM.get(((sizeof(Log)*Index)+4), Log);
        if(Log.Valid == 123)
        {
            sprintf(buffer, "DateTime 2022/11/25 10/15/34 - - - Valve = %s \r\n", (Log.State==1)?"Opened":"Closed");
            text += "<li class='ui-widget-content'>" + String(buffer) + "</li>";
        }
    }

    return text;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void PrintStatus()
{
    digitalWrite(Pin_LED_MAIN, LOW);
    char buffer[128];
    timeClient.update();
    unsigned long epochTime = timeClient.getEpochTime();
    struct tm *ptm = gmtime ((time_t *)&epochTime);
    
    //sprintf(buffer, "%04d/%02d/%02d %02d:%02d:%02d - - - Valve = %s", ptm->tm_year+1900, ptm->tm_mon+1, ptm->tm_mday, timeClient.getHours(), timeClient.getMinutes(), timeClient.getSeconds(), Get_Value_Valve()?"Opened":"Closed");
    sprintf(buffer, "Sense_Low=%s - - - Sense_High=%s - - - Valve = %s - - - TimeOut = %d - - - Status = %s", (Value_Sense_Low==0)?"Ok":"No", (Value_Sense_High==0)?"Ok":"No", Get_Value_Valve()?"Opened":"Closed", Valve_Time_On, (Valve_Time_On==60)?"Fault":"Normal");

    Serial.println(buffer);
    digitalWrite(Pin_LED_MAIN, HIGH);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void Proccess_Page() 
{
  String Page = "";

  if(server.argName(0).indexOf("Command") != -1) 
  {
    if(server.arg(0).indexOf("Clear_Log") != -1) 
    {
      Log_Clear();
      Page += "<script>";
      Page += "alert('Log Cleared');";
      Page += "history.go(-1);";
      Page += "location.reload();";
      Page += "</script>"; 
    }
  }
  else
  {
      Page += "<html lang='en'>";
      Page += "<head>";
      Page += "<meta http-equiv='refresh' content='10'>";
      Page += "<meta charset='utf-8'>";
      Page += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
      Page += "<title>Water Level Control</title>";
      Page += "<link rel='stylesheet' href='https://code.jquery.com/ui/1.13.2/themes/base/jquery-ui.css'>";
      Page += "<link rel='stylesheet' href='https://jqueryui.com/resources/demos/style.css'>"; 
      Page += "<style>";
      Page += "#feedback { font-size: 1.4em; }";
      Page += "#selectable .ui-selecting { background: #FECA40; }";
      Page += "#selectable .ui-selected { background: #F39814; color: white; }";
      Page += "#selectable { list-style-type: none; margin: 0; padding: 0; width: 100%; }";
      Page += "#selectable li { margin: 3px; padding: 0.4em; font-size: 1.4em; height: 18px; }";
      Page += "</style>";
      Page += "<script src='https://code.jquery.com/jquery-3.6.0.js'></script>";
      Page += "<script src='https://code.jquery.com/ui/1.13.2/jquery-ui.js'></script>";
      Page += "</head>";
      Page += "<body style='background-image: url(https://static.vecteezy.com/system/resources/previews/004/478/034/non_2x/drop-water-ice-snow-seamless-design-background-vector.jpg); background-repeat: repeat;'>"; 
      Page += "<h1 style='text-align:center;'> WaterLevel Control</h1>";
      Page += "<fieldset style='background-color: rgba(255,255,255, 0.6);'>";
      Page += "<legend>Log : </legend>";
      Page += "<div style='overflow-y:scroll; height:400px; background-color: rgba(255,255,255, 0.6);'>"; 
      Page += "<ol id='selectable'>";
      Page += Log_Get();
      Page += "</ol>";
      Page += "</div>";
      Page += "</fieldset>";
      Page += "<br>";
      Page += "<fieldset style='background-color: rgba(255,255,255, 0.6);'>";
      Page += "<legend>Control Button : </legend>";
      Page += "<div>";
      Page += "<a href='?Command=Clear_Log' id='Clear_Log'>Clear Log</a>";
      Page += "</div>";
      Page += "</fieldset>";
      Page += "</body>";
      Page += "<script>";
      Page += "$(function(){ ";
      Page += "$('#selectable').selectable();";
      Page += "$('#Clear_Log').button();";
      Page += "$('#Clear_Log').on('click', function(event){";  
      Page += "});";
      Page += "}); ";
      Page += "</script>";  
      Page += "</html>";
  }
  
  server.send(200, "text/html", Page);
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup(void) {

  /// Serial
  //{
  Serial.begin(2400);
  Serial.println("");
  Serial.println("Starting...");
  //}

  /// IO
  //{
  pinMode(Pin_LED_MAIN, OUTPUT);
  pinMode(Pin_LED_Red, OUTPUT);
  pinMode(Pin_LED_Green, OUTPUT);
  pinMode(Pin_Valve, OUTPUT);
  pinMode(Pin_Buzzer, OUTPUT);
  pinMode(Pin_Sense_Low, INPUT_PULLUP);
  pinMode(Pin_Sense_High, INPUT_PULLUP);

  digitalWrite(Pin_LED_MAIN, HIGH);
  digitalWrite(Pin_Buzzer, LOW);
  Value_Sense_Low = digitalRead(Pin_Sense_Low);
  Value_Sense_High = digitalRead(Pin_Sense_High);  
  Set_Value_Valve(false);
  //}

  /// EEprom Log
  //{
  EEPROM.begin((sizeof(Log)*Log_Locate_Max)+4);
  EEPROM.get(0, Log_Locate);
  Serial.println("Loading EEprom Log");
  //}

  /*
  /// EEprom
  //{
  EEPROM.begin(512);
  Serial.println("Loading Config");
  EEPROM.get(0, Config);
  if(Config.Validation != 5)
  {
    Serial.println("Reset Factory Config");
    strcpy(Config.serial, "serial");
    strcpy(Config.ssid, "ssid");
    strcpy(Config.password, "password");
    strcpy(Config.local, "192.168.1.1");
    strcpy(Config.gateway, "192.168.1.1");
    strcpy(Config.subnet, "255.255.255.255");
    Config.Validation = 5;
    EEPROM.put(0, Config);
  }
  Serial.println("Loaded Config");
  EEPROM.end();
  //}
  */
  
  strcpy(Config.serial, "O903000000");
  strcpy(Config.ssid, "Mahsen_1000");
  strcpy(Config.password, "03100mahsen3mik");
  strcpy(Config.local, "192.168.88.250");
  strcpy(Config.gateway, "192.168.88.1");
  strcpy(Config.subnet, "255.255.255.0");
  
  /// WIFI
  //{
  //WiFi.mode(WIFI_STA);
  //WiFi.hostname(Config.serial);         
  
  Serial.print("Configuration is ");
  //WiFi.setAutoConnect(false);   // Not working by its own
  //WiFi.disconnect();
  local.fromString(Config.local);
  gateway.fromString(Config.gateway);
  subnet.fromString(Config.subnet);
  Serial.println(WiFi.config(local, gateway, subnet) ? "Ready" : "Failed!");

  Serial.print("Server is ");
  WiFi.begin(Config.ssid, Config.password);
  for (int TimeOut=0; ((TimeOut<5) && (WiFi.status() != WL_CONNECTED)); TimeOut++)
  {
     delay(1000);
  }  
  if((WiFi.status() == WL_CONNECTED))
  {
      Serial.println("Ready");
    
      Serial.print("IP = ");
      Serial.println(WiFi.localIP());
    
      server.on("/", HTTP_GET, []() 
      {
        Proccess_Page();
      });
      server.begin();    
      Serial.println("Server started");
    
      timeClient.begin();
      Serial.println("ntpUDP started");
  }
  else
  {
      Serial.println("Error");
  }
  //}

}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop(void)
{

  Value_Sense_Low = Get_Value_Sense_Low(); 
  Value_Sense_High = Get_Value_Sense_High();  
  if(Value_Sense_Low == HIGH)
  {
      if(Valve_Time_On < 60)
      {
          Set_Value_Valve(true);
          Valve_Time_On++;
      }
      if(Valve_Time_On == 60)
      {
          digitalWrite(Pin_Buzzer, LOW);
          Set_Value_Valve(false);
      }          
  }
  else if(Value_Sense_High == LOW)
  {      
      Set_Value_Valve(false);
      digitalWrite(Pin_Buzzer, HIGH);
      Valve_Time_On = 0;
  }

  PrintStatus();
  
  delay(1000);
  server.handleClient();
  
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////

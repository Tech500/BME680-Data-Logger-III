/*
**
**                      BME680 Data Logger III  3/5/2021 @ 22:05 EST  --ESPAsyncWebserver version
**                      William Lucid
**
 * Original code from Rui Santos
 * Project Details https://RandomNerdTutorials.com
 * *
 * *    Robert Gillespie's  "OLED_BME680.odt"  --bases of modified code
 * *
 * *    BME680_DataLogger_Webserver.ino modifion code 3/3/2021 William Lucid's
 *
 */

#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>  //https://github.com/me-no-dev/AsyncTCP
#include <ESPAsyncWebServer.h>  //https://github.com/me-no-dev/ESPAsyncWebServer
#include "FTPServer.h"
#include <WiFiUdp.h>
#include "FS.h"
#include <LittleFS.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <ThingSpeak.h>   //https://github.com/mathworks/thingspeak-arduino  --> Used for ThingSpeak.com graphing. Get it using the Library Manager.  Requires an account


#import "index1.h"  //BME680 HTML; do not remove
#import "index2.h"  //SdBrowse HTML; do not remove
#import "index3.h"  //Graphs HTML; do not remove

//String linkAddress = "http://73.102.122.239:8060";  //WAN --publicIP and PORT for URL link
String linkAddress = "http://10.0.0.110:8060";  //LAN --privateIP and PORT for URL link

// Replace with your network details
const char* ssid = "R2D2";
const char* password = "sissy4357";

//AsyncWebServer
AsyncWebServer serverAsync(8060);
AsyncWebSocket ws("/ws"); // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)

void onRequest(AsyncWebServerRequest *request)
{
     //Handle Unknown Request
     request->send(404);
}

void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
{
     //Handle body
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
{
     //Handle upload
}

void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len)
{
     //Handle WebSocket event
}

WiFiClient client;

IPAddress ipREMOTE;

char* filelist[12];

String logging;

char *filename;
String fn;
char str[] = {0};
char MyBuffer[13];
String fileRead;
String PATH;

int flag;

#define online 19  //pin for online LED indicator

#define SPIFFS LittleFS

// tell the FtpServer to use LittleFS
FTPServer ftpSrv(LittleFS);

///Are we currently connected?
boolean connected = false;

WiFiUDP udp;
// local port to listen for UDP packets
const int udpPort = 1337;
char incomingPacket[255];
char replyPacket[] = "Hi there! Got the message :-)";
const char * udpAddress1 = "us.pool.ntp.org";
const char * udpAddress2 = "time.nist.gov";

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME680 bme; // I2C

float temperature;
float temperatureF;  //Degrees F.
float humidity;
float pressure;
float baro;  //inHg
float gasResistance;  // Resistance KOhms
float elevation;  //Altitude

String IAQ;  //String for Air Quality

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3D ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32 <<-- My Oled uses 0x3D since it is 128 X 64 SCREEN ADDRESS
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//unsigned long lastTime = 0;
//unsigned long timerDelay = 30000;  // send readings timer

//Time settings
#define TZ "EST+5EDT,M3.2.0/2,M11.1.0/2"
int DOW, MONTH, DATE, YEAR, HOUR, MINUTE, SECOND;
char daysOfTheWeek[7][12] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
char strftime_buf[64];
String dtStamp(strftime_buf);
String lastUpdate;
int lc = 0;
time_t tnow = 0;

void wifi_Start()
{

     WiFi.mode(WIFI_STA);
     WiFi.disconnect();

     Serial.println();
     Serial.print("MAC: ");
     Serial.println(WiFi.macAddress());

     // We start by connecting to a WiFi network
     Serial.print("Connecting to ");
     Serial.println(ssid);

     WiFi.begin(ssid, password);

     //setting the static addresses in function "wifi_Start
     IPAddress ip {10,0,0,110};
     IPAddress gateway {10,0,0,1};
     IPAddress subnet {255,255,255,0};
     IPAddress dns {10,0,0,1};

     WiFi.config(ip, gateway, subnet, dns);

     // Starting the web server
     //server.begin();
     Serial.println("Waiting for the ESP32 IP...");

     // Printing the ESP IP address
     Serial.print("Server IP:  ");
     Serial.println(WiFi.localIP());
     Serial.print("Port:  ");
     Serial.print("80");
     Serial.println("\n");

     WiFi.waitForConnectResult();

     Serial.printf("Connection result: %d\n", WiFi.waitForConnectResult());

     if ((WiFi.status() != WL_CONNECTED))
     {

          if(MINUTE % 20 == 0)
          {

               wifi_Start();

          }

          serverAsync.begin();



     }


}

/*
     This is the ThingSpeak channel number for the MathwWorks weather station
     https://thingspeak.com/channels/YourChannelNumber.  It senses a number of things and puts them in the eight
     field of the channel:

     Field 6 - Temperature (Degrees F)
     Field 6 - Humidity (%RH)
     Field 7 - Barometric Pressure (in Hg)
     Field 8 - Gas Resistance KOhms
*/

//Graphing requires "FREE" "ThingSpeak.com" account..
//Enter "ThingSpeak.com" data here....
unsigned long myChannelNumber = 1320323;
const char * myWriteAPIKey = "95NA12W3LS7IOE65";


void setup()
{

     Serial.begin(9600);

     while(!Serial);

     Serial.println(F("\n\n\nBME680 Data Logger III  3/5/2021 @ 22:05 EST"));

     wifi_Start();

     ThingSpeak.begin(client);  // Initialize ThingSpeak

     serverAsync.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request)
     {
          PATH = "/FAVICON";
          //accessLog();
          if (! flag == 1)
          {
               request->send(SPIFFS, "/favicon.png", "image/png");

          }
          //end();
     });

     /*
     erverAsync.on("/", HTTP_GET, [](AsyncWebServerRequest * request)
       {

         PATH = "/";
         accessLog();

         ipREMOTE = request->client()->remoteIP();

         if (! flag == 1)
         {
           request->send_P(200, PSTR("text/html"), HTML1, processor1);
         }
         end();
       });
       */

     serverAsync.on("/BME680", HTTP_GET, [](AsyncWebServerRequest * request)
     {

          PATH = "/BME680";
          accessLog();

          ipREMOTE = request->client()->remoteIP();

          if (! flag == 1)
          {
               request->send_P(200, PSTR("text/html"), HTML1, processor1);
          }
          end();
     });

     serverAsync.on("/SdBrowse", HTTP_GET, [](AsyncWebServerRequest * request)
     {
          PATH = "/SdBrowse";
          accessLog();
          if (! flag == 1)
          {
               request->send_P(200, PSTR("text/html"), HTML2, processor2);

          }
          end();
     });

     serverAsync.on("/Graphs", HTTP_GET, [](AsyncWebServerRequest * request)
     {
          PATH = "/Graphs";
          accessLog();
          AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML3, processor3);
          response->addHeader("Server", "ESP Async Web Server");
          if (! flag == 1)
          {
               request->send(response);

          }
          end();
     });

     /*
          serverAsync.on("/ACCESS.TXT", HTTP_GET, [](AsyncWebServerRequest * request)
          {
               PATH = "/ACCESS";
               accessLog();
               AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML3);
               response->addHeader("Server","ESP Async Web Server");
               if(! flag == 1)
               {
                    request->send(SPIFFS, "/ACCESS610.TXT");

               }
               end();
          });


     serverAsync.on("/RESTART", HTTP_GET, [](AsyncWebServerRequest * request)
     {
       PATH = "/START";
       accessLog();
       AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", HTML4);
       response->addHeader("Server", "ESP Async Web Server");
       if (! flag == 1)
       {
         request->send(response);

       }
       Serial2.flush();
       end();
       ESP.restart();

     });
     */

     serverAsync.onNotFound(notFound);

     serverAsync.begin();

     bool fsok = LittleFS.begin();
     Serial.printf_P(PSTR("FS init: %s\n"), fsok ? PSTR("ok") : PSTR("fail!"));

     //LittleFS.format();

     // setup the ftp server with username and password
     // ports are defined in FTPCommon.h, default is
     //   21 for the control connection
     //   50009 for the data connection (passive mode by default)
     ftpSrv.begin(F("ftp"), F("ftp")); //username, password for ftp.  set ports in ESP8266FtpServer.h  (default 21, 50009 for PASV)


     // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
     display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
     // init done
     display.display();

     display.clearDisplay();
     display.display();
     display.setTextSize(1);
     display.setTextColor(WHITE);

     if (!bme.begin())
     {
          Serial.println("Could not find a valid BME680 sensor, check wiring!");
          while (1);
     }

     // Set up oversampling and filter initialization
     bme.setTemperatureOversampling(BME680_OS_8X);
     bme.setHumidityOversampling(BME680_OS_2X);
     bme.setPressureOversampling(BME680_OS_4X);
     bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
     bme.setGasHeater(320, 150); // 320*C for 150 ms

     configTime(0, 0, udpAddress1, udpAddress2);
     setenv("TZ", "EST+5EDT,M3.2.0/2,M11.1.0/2", 3);   // this sets TZ to Indianapolis, Indiana
     tzset();

     Serial.print("wait for first valid timestamp ");


     while (time(nullptr) < 100000ul)
     {
          Serial.print(".");

          delay(5000);

     }


     Serial.println("\nSystem Time set");

     getWeatherData();

}

void loop()
{

     //udp only send data when connected
     if (connected)
     {

          //Send a packet
          udp.beginPacket(udpAddress1, udpPort);
          udp.printf("Seconds since boot: %u", millis() / 1000);
          udp.endPacket();
     }

     for(int x; x < 5000; x++)
     {
          ftpSrv.handleFTP();
     }

     getDateTime();


     if(SECOND ==  0)
     {

          delay(1000);

          screenOne();

          serialMonitor();

     }

     if(SECOND == 30)
     {

          delay(1000);

          screenTwo();

          serialMonitor();

     }


     if((MINUTE % 15 == 0) && (SECOND < 3))
     {

          delay(1000);

          getWeatherData();

          logtoSPIFFS();

     }

     if((MINUTE % 15 == 0) && (SECOND == 45))
     {

          speak();

     }

     if ((HOUR == 23) && (MINUTE == 58) && (SECOND == 30)) //Start new kog file..
     {

          fileStore();

     }

}

String processor1(const String& var)
{

     //index1.h

     String str;

     airQuality();

     if (var == F("TEMP"))
          return String(temperatureF);

     if (var == F("HUM"))
          return String(humidity) ;

     if (var == F("SEALEVEL"))
          return String(baro);

     if (var == F("GAS"))
          return String(gasResistance);

     if (var == F("AIR"))
          return (IAQ);

     if (var == F("CLIENTIP"))
          return ipREMOTE.toString().c_str();

     if (var == F("%LINK%"))
          return linkAddress;


     return String();

}

String processor2(const String& var)
{

     //index2.h

     String str;

     if (!SPIFFS.begin())
     {
          Serial.println("SPIFFS failed to mount !");
     }
     Dir dir = SPIFFS.openDir("/");
     while (dir.next())
     {

          String file_name = dir.fileName();

          if (file_name.startsWith("LOG"))
          {

               str += "<a href=\"";
               str += dir.fileName();
               str += "\">";
               str += dir.fileName();
               str += "</a>";
               str += "    ";
               str += dir.fileSize();
               str += "<br>\r\n";
          }
     }

     client.print(str);

     if (var == F("URLLINK"))
          return  str;

     if (var == F("FILENAME"))
          return  dir.fileName();

     if (var == F("%LINK%"))
          return linkAddress;

     return String();

}

String processor3(const String& var)
{

     //index3.h

     //if (var == F("LINK"))
     //return linkAddress;

     if (var == F("%LINK%"))
          return (linkAddress);

     return String();

}

void accessLog()
{

     digitalWrite(online, HIGH);  //turn on online LED indicator

     getDateTime();

     String ip1String = "10.0.0.146";   //Host ip address
     String ip2String = ipREMOTE.toString().c_str();   //client remote IP address
     String returnedIP = ip2String;

     Serial.println("");
     Serial.println("Client connected:  " + dtStamp);
     Serial.print("Client IP:  ");
     Serial.println(returnedIP);
     Serial.print("Path:  ");
     Serial.println(PATH);
     Serial.println(F("Processing request"));

     //Open a "access.txt" for appended writing.   Client access ip address logged.
     File logFile = SPIFFS.open("/ACCESS.TXT", "a");

     if (!logFile)
     {
          Serial.println("File 'ACCESS.TXT'failed to open");
     }

     if ((ip1String == ip2String) || (ip2String == "0.0.0.0") || (ip2String == "(IP unset)"))
     {

          //Serial.println("HOST IP Address match");
          logFile.close();

     }
     else
     {

          Serial.println("Log client ip address");

          logFile.print("Accessed:  ");
          logFile.print(dtStamp);
          logFile.print(" -- Client IP:  ");
          logFile.print(ip2String);
          logFile.print(" -- ");
          logFile.print("Path:  ");
          logFile.print(PATH);
          logFile.println("");
          logFile.close();

     }

}

void end()
{

     digitalWrite(online, LOW);   //turn-off online LED indicator

     getDateTime();

     Serial.println("Client closed:  " + dtStamp);

}

void fileStore()   //If Midnight, rename "LOGXXYYZZ.TXT" to ("log" + month + day + ".txt") and create new, empty "LOGXXYYZZ.TXT"
{

     int temp;
     String Date;
     String Month;

     temp = (DATE);
     if (temp < 10)
     {
          Date = ("0" + (String)temp);
     }
     else
     {
          Date = (String)temp;
     }

     temp = (MONTH);
     if (temp < 10)
     {
          Month = ("0" + (String)temp);
     }
     else
     {
          Month = (String)temp;
     }

     String logname;  //file format /LOGxxyyzzzz.txt
     logname = "/LOG";
     logname += Month;   ///logname += Clock.getDate();
     logname += Date; ////logname += Clock.getMonth(Century);
     logname += YEAR;
     logname += ".TXT";

     //Open file for appended writing
     File log = SPIFFS.open(logname.c_str(), "a");

     if (!log)
     {
          Serial.println("file open failed");
     }

}

String getDateTime()
{
     struct tm *ti;

     tnow = time(nullptr) + 1;
     ti = localtime(&tnow);
     DOW = ti->tm_wday;
     YEAR = ti->tm_year + 1900;
     MONTH = ti->tm_mon + 1;
     DATE = ti->tm_mday;
     HOUR  = ti->tm_hour;
     MINUTE  = ti->tm_min;
     SECOND = ti->tm_sec;

     strftime(strftime_buf, sizeof(strftime_buf), "%a , %m/%d/%Y , %H:%M:%S %Z", localtime(&tnow));
     dtStamp = strftime_buf;
     return (dtStamp);

}

void getWeatherData()
{

     if (! bme.performReading())
     {
          Serial.println("Failed to perform reading :(");
          return;
     }

     temperature = bme.temperature;  //Degrees C
     temperatureF = ((bme.temperature * 1.8 ) + 32);  //Degrees F.
     pressure = bme.pressure / 100.0;  //hPa (millibars)
     baro =  (bme.pressure * 0.0002953);  //inHg
     humidity = bme.humidity;  //rH %
     gasResistance = bme.gas_resistance / 1000.0;  //KOhms
     elevation = (bme.readAltitude(SEALEVELPRESSURE_HPA));

}

float logtoSPIFFS()   //Output to SPIFFS every fifthteen minutes
{

     int header = 0;;

     int tempy;
     String Date;
     String Month;

     tempy = (DATE);
     if (tempy < 10)
     {
          Date = ("0" + (String)tempy);
     }
     else
     {
          Date = (String)tempy;
     }

     tempy = (MONTH);
     if (tempy < 10)
     {
          Month = ("0" + (String)tempy);
     }
     else
     {
          Month = (String)tempy;
     }

     String logname;
     logname = "/LOG";
     logname += Month;   ///logname += Clock.getDate();
     logname += Date; ////logname += Clock.getMonth(Century);
     logname += YEAR;
     logname += ".TXT";

     // Open a "log.txt" for appended writing
     File log = SPIFFS.open(logname.c_str(), "a");

     if (!log)
     {
          Serial.println("file 'LOG.TXT' open failed");
     }

     if ((HOUR == 0) && (MINUTE == 0) && (SECOND < 3)) //Create header
     {
          header = 1;
     }

     if(header == 1)
     {

          log.print("BME680 Data Logger III   ");
          log.println(dtStamp);
          log.println("");

          header = 0;

     }

     log.print(dtStamp);
     log.print("  , ");

     airQuality();

     log.print("Air Qualitiy:  ");
     log.print(IAQ);
     log.print("  , ");

     log.print("Gas Resistance:  ");
     log.print(gasResistance);
     log.print(" KOhms , ");

     log.print("Temperature:  ");
     log.print(temperatureF, 1);
     log.print(" F. , ");

     log.print("Humidity:  ");
     log.print(humidity);
     log.print(" % ");
     log.print(" , ");

     log.print("Pressure:  ");
     log.print(baro, 3);
     log.print(" inHg. ");
     log.println("");

     log.close();

     Serial.println("\nData written to  " + logname + "  " + dtStamp);

}


//readFile  --AsyncWebServer version with much help from Pavel
void notFound(AsyncWebServerRequest *request)
{

     digitalWrite(online, HIGH);   //turn-on online LED indicator

     if (! request->url().endsWith(F(".TXT")))
     {
          request->send(404);
     }
     else
     {
          if (request->url().endsWith(F(".TXT")))
          {
               //.endsWith(F(".txt")))

               // here comes some mambo-jambo to extract the filename from request->url()
               int fnsstart = request->url().lastIndexOf('/');

               fn = request->url().substring(fnsstart);

               PATH = fn;

               accessLog();

               Serial.print("File:  ");
               Serial.println(fn);



               File webFile = SPIFFS.open(fn, "r");

               Serial.print("File size: ");

               Serial.println(webFile.size());

               if (!webFile)
               {

                    Serial.println("File:  " + fn + " failed to open");
                    Serial.println("\n");

               }
               else if (webFile.size() == 0)
               {

                    webFile.close();

               }
               else
               {

                    //request->send(SPIFFS, fn, String(), true);  //Download file
                    request->send(SPIFFS, fn, String(), false);  //Display file

                    webFile.close();

               }

               fn = "";

               end();

          }

     }

     digitalWrite(online, LOW);   //turn-off online LED indicator

}

void screenOne()
{


     //----------- Start First Screen

     //Convert float to String
     char fltemperatureF[15];
     dtostrf(temperatureF, 2, 2, fltemperatureF);
     char flpressure[15];
     dtostrf((baro), 2, 2, flpressure);
     char flhumidity[15];
     dtostrf(bme.humidity, 2, 2, flhumidity);
     char flgas[15];
     dtostrf((bme.gas_resistance / 1000.0),6, 2, flgas);
     char flalt[15];
     dtostrf((bme.readAltitude(SEALEVELPRESSURE_HPA)),3, 2, flalt);


     display.setCursor(0,0);
     display.clearDisplay();
     display.setCursor(0,18);
     display.setTextSize(1);
     display.print("Temp: ");
     display.setCursor(45,18);
     display.print(fltemperatureF);
     display.setCursor(82,18);
     display.print("F");

     display.setCursor(0,30);
     display.setTextSize(1);
     display.print("Press: ");
     display.setCursor(45,30);
     display.print(flpressure);
     display.setCursor(82,30);
     display.print("inHg");

     display.setCursor(0,42);
     display.setTextSize(1);
     display.print("Hum: ");
     display.setCursor(45,42);
     display.print(flhumidity);
     display.setCursor(82,42);
     display.print("%");

     display.setCursor(0,54);
     display.setTextSize(1);
     display.print("Alt: ");
     display.setCursor(40,54);
     display.print(flalt);
     display.setCursor(82,54);
     display.print("M");

     display.display();

}

void screenTwo()
{

     //-------------- Start Second Screen

     //Convert float to String
     char fltemperatureF[15];
     dtostrf(temperatureF, 2, 2, fltemperatureF);
     char flpressure[15];
     dtostrf((baro), 2, 2, flpressure);
     char flhumidity[15];
     dtostrf(bme.humidity, 2, 2, flhumidity);
     char flgas[15];
     dtostrf((bme.gas_resistance / 1000.0),6, 2, flgas);
     char flalt[15];
     dtostrf((bme.readAltitude(SEALEVELPRESSURE_HPA)),3, 2, flalt);

     airQuality();

     display.setCursor(0,0);
     display.clearDisplay();
     display.setCursor(10,18);
     display.setTextSize(1);
     display.print("Air Quality");
     display.setCursor(10,30);
     display.print(IAQ);
     display.display();

     display.setCursor(10,52);
     display.setTextSize(1);
     display.print("Gas: ");
     display.setCursor(42,52);
     display.print(flgas);
     display.setCursor(82,52);
     display.print("KOhms");

     display.display();

}

void airQuality()
{

     gasResistance = (bme.gas_resistance / 1000.0);  //Gas Resistance KOhms

     if((gasResistance >= 0) && (gasResistance <= 50))
     {

          IAQ = "Very Poor";
     }

     if((gasResistance >= 51) && (gasResistance <= 100))
     {

          IAQ = "Worst";
     }

     if((gasResistance >= 101) && (gasResistance <= 150))
     {

          IAQ = "Bad";
     }

     if((gasResistance >= 151) && (gasResistance <= 200))
     {

          IAQ = "Less Average";
     }

     if((gasResistance >= 201) && (gasResistance <= 300))
     {

          IAQ = "Average";

     }

     if((gasResistance >= 301) && (gasResistance <= 500))
     {

          IAQ = "Good";

     }

}

void serialMonitor()
{

     Serial.print("\nTemperature:     " + String(temperatureF) + " °F "); ;
     Serial.print("\nPressure:        " + String(baro) + " inHg");
     Serial.print("\nHumidity:        " + String(humidity) + " %");
     Serial.print("\nGas Resistance: " + String(gasResistance) + " KOhms");
     Serial.println("\nAltitude:       " + String(elevation) + " M");

     airQuality();

     Serial.println("\nAir Quality:  " + IAQ + "");
     Serial.println(dtStamp);

}

void speak()
{

     getWeatherData();

     //Convert float to String
     char fltemperatureF[15];
     dtostrf(temperatureF, 2, 3, fltemperatureF);
     char flhumidity[15];
     dtostrf(bme.humidity, 2, 3, flhumidity);
     char flpressure[15];
     dtostrf((baro), 2, 3, flpressure);
     char flgas[15];
     dtostrf((bme.gas_resistance / 1000.0),6, 3, flgas);

     // set the fields with the values
     ThingSpeak.setField(1, fltemperatureF);
     ThingSpeak.setField(2, flhumidity);
     ThingSpeak.setField(3, flpressure);
     ThingSpeak.setField(4, flgas);

     //ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

     // write to the ThingSpeak channel
     int x = ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);

     if(x == 200)
     {
          Serial.println("Channel update successful.");
     }
     else
     {
          Serial.println("Problem updating channel. HTTP error code " + String(x));
     }

     delay(1000);

}

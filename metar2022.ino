

// *********************************** WIFI vars

//"https://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow="+str(metar_age)+"&stationString="
// https://www.aviationweather.gov/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow=1&stationString=kdca


// https://aviationweather.gov/api/data/metar?ids=KDCA&format=xml

#include <SPI.h>
#include <WiFiNINA.h>



int nCurrentAirport = 0;

// default pw and airport codes
#include "metar_config.h"
//String ssid = MYSSID;        // your network SSID (name)
//String pass = WIFIPASS;    // your network password (use for WPA, or use as key for WEP)

int keyIndex = 0;            // your network key index number (needed only for WEP)

int status = WL_IDLE_STATUS;

// Initialize the WiFi client library
//WiFiClient client;
WiFiSSLClient client;

// server address:
char server[] = "aviationweather.gov";

unsigned long lastConnectionTime = 0;            // last time you connected to the server, in milliseconds
//const unsigned long postingInterval = 10L * 1000L; // delay between updates, in milliseconds
//const unsigned long postingInterval = 10L * 500L; // delay between updates, in milliseconds
//const unsigned long postingInterval = 10L * 500L; // delay between updates, in milliseconds
const unsigned long postingInterval = 10L * 500L; // delay between updates, in milliseconds
/***********************  LED SETUP ******************/

#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif


unsigned int nTime = 0;

// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        6 // On Trinket or Gemma, suggest changing this to 1

// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 50 // zero based 50 LED string

// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


// which mode is chosen now, changed via press button  weather, winds, flight category etc.
#define MAXDISPLAYMODES 4

#define DISPLAYBUTTON_PIN 11
#define BRIGHTBUTTON_PIN 12

// INDICATOR ON MAP SHOWING THE SELECTED MODE
#define DISPLAYMODE1 2
#define DISPLAYMODE2 4
#define DISPLAYMODE3 5
#define DISPLAYMODE4 3


// use as a divisor for all colors eg.   150 / 2 makes it half as bright  pressing buttons loops through the dimmness options.
int brightvalues[] = {1, 2, 4, 8, 16, 32, 64};


#define MAXIGNORETIMESETTING 10*NUMPIXELS   // ignore the time settings for 10 times through the LED string.
unsigned int nIgnoreTimeSetting = MAXIGNORETIMESETTING+1;


/***********************  FLASH MEMORY SETUP ******************/

#include <FlashStorage.h>

// Create a structure that is big enough to contain a name
// and a surname. The "valid" variable is set to "true" once
// the structure is filled with actual data for the first time.

//  char ssid[50];
//  char pass[50];
//  
typedef struct {
  boolean valid;
  char ssid[50];
  char pass[50];
  int nDisplayMode;
  int nBrightSetting; 
  bool lBlink;
  int nWebServicePause;
  int On24hr;
  int Off24hr;
  int nZuluOffset ;
} MetarSettings;



FlashStorage(my_flash_store, MetarSettings);
MetarSettings Settings;

// *********************************** SETUP

void setup() {

  pinMode(DISPLAYBUTTON_PIN, INPUT_PULLUP);    // sets the digital pin  as input
  pinMode(BRIGHTBUTTON_PIN, INPUT_PULLUP);    // sets the digital pin  as input

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  delay(5000);
    
  InitializePixel();
  pixels.setPixelColor(0, pixels.Color(150, 150, 150));   // white  30f
  pixels.show();

  ProcessFlash();
  pixels.setPixelColor(1, pixels.Color(150, 150, 150));   // white  30f
  pixels.show();
 
  SetDisplayIndicator();
  pixels.setPixelColor(2, pixels.Color(150, 150, 150));   // white  30f
  pixels.show();
 

  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }
  pixels.setPixelColor(3, pixels.Color(150, 150, 150));   // white  30f
  pixels.show();


  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }
  pixels.setPixelColor(4, pixels.Color(150, 150, 150));   // white  30f
  pixels.show();

  // attempt to connect to WiFi network:
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(Settings.ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(Settings.ssid, Settings.pass);
    // wait x seconds for connection:
    delay(5000);
  }

  pixels.setPixelColor(5, pixels.Color(150, 150, 150));   // white  30f
  pixels.show();

  // you're connected now, so print out the status:
  printWifiStatus();
  pixels.setPixelColor(6, pixels.Color(150, 150, 150));   // white  30f
  pixels.show();

  delay(5000);
  
  InitializePixel();

}




//
//
//#define HMCRAMC0_SIZE     0x8000UL
//#define HMCRAMC0_ADDR    (0x20000000U)
//#define MAGIC_VALUE         0x07738135U
//
//
//void ResetToBoot()
//{
//    Serial.println("rebooting Now");
//    *((volatile uint32_t *)(HMCRAMC0_ADDR + HMCRAMC0_SIZE - 4)) = MAGIC_VALUE;
//    NVIC_SystemReset();
//}
//
//


void SetDisplayIndicator()
{

  digitalWrite(DISPLAYMODE1, LOW); // Turn the LED off
  digitalWrite(DISPLAYMODE2, LOW); // Turn the LED off
  digitalWrite(DISPLAYMODE3, LOW); // Turn the LED off
  digitalWrite(DISPLAYMODE4, LOW); // Turn the LED off


  if (Settings.nDisplayMode == 1)
  {
    digitalWrite(DISPLAYMODE1, HIGH); // Turn the LED on
  }
  else if (Settings.nDisplayMode == 2)
  {
    digitalWrite(DISPLAYMODE2, HIGH); // Turn the LED on
  }
  else if (Settings.nDisplayMode == 3)
  {
    digitalWrite(DISPLAYMODE3, HIGH); // Turn the LED on
  }
  else if (Settings.nDisplayMode == 4)
  {
    digitalWrite(DISPLAYMODE4, HIGH); // Turn the LED on
  }

}



void InitializePixel()
{

  //  Serial.println("begin pixel ***************");

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();   // Send the updated pixel colors to the hardware.

  if (Settings.nDisplayMode != MAXDISPLAYMODES)
  {
        //  // The first NeoPixel in a strand is #0, second is 1, all the way up
        //  // to the count of pixels minus one.
        for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
      
          // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
          // Here we're using a moderately bright green color:
          pixels.setPixelColor(i, pixels.Color(10, 10, 0));
        }
        //
        pixels.show();   // Send the updated pixel colors to the hardware.
      
        //  Serial.println("end pixel ***************");

  }

  
}




void ClearPixels()
{

  //  Serial.println("begin pixel ***************");

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
  pixels.clear(); // Set all pixel colors to 'off'
  pixels.show();   // Send the updated pixel colors to the hardware.
  
//
//
//  //  // The first NeoPixel in a strand is #0, second is 1, all the way up
//  //  // to the count of pixels minus one.
//  for (int i = 0; i < NUMPIXELS; i++) { // For each pixel...
//
//    // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
//    // Here we're using a moderately bright green color:
//    pixels.setPixelColor(i, pixels.Color(10, 10, 0));
//  }
//  //
//  pixels.show();   // Send the updated pixel colors to the hardware.
//
//  //  Serial.println("end pixel ***************");

}





String GetAirportCode(int nAirport)
{

  switch (nAirport) {
    case 0:
      return AP0;
      break;
    case 1:
      return AP1;
      break;
    case 2:
      return AP2;
      break;
    case 3:
      return AP3;
      break;
    case 4:
      return AP4;
      break;
    case 5:
      return AP5;
      break;
    case 6:
      return AP6;
      break;
    case 7:
      return AP7;
      break;
    case 8:
      return AP8;
      break;
    case 9:
      return AP9;
      break;
    case 10:
      return AP10;
      break;
    case 11:
      return AP11;
      break;
    case 12:
      return AP12;
      break;
    case 13:
      return AP13;
      break;
    case 14:
      return AP14;
      break;
    case 15:
      return AP15;
      break;
    case 16:
      return AP16;
      break;
    case 17:
      return AP17;
      break;
    case 18:
      return AP18;
      break;
    case 19:
      return AP19;
      break;
    case 20:
      return AP20;
      break;
    case 21:
      return AP21;
      break;
    case 22:
      return AP22;
      break;
    case 23:
      return AP23;
      break;
    case 24:
      return AP24;
      break;
    case 25:
      return AP25;
      break;
    case 26:
      return AP26;
      break;
    case 27:
      return AP27;
      break;
    case 28:
      return AP28;
      break;
    case 29:
      return AP29;
      break;
    case 30:
      return AP30;
      break;
    case 31:
      return AP31;
      break;
    case 32:
      return AP32;
      break;
    case 33:
      return AP33;
      break;
    case 34:
      return AP34;
      break;
    case 35:
      return AP35;
      break;
    case 36:
      return AP36;
      break;
    case 37:
      return AP37;
      break;
    case 38:
      return AP38;
      break;
    case 39:
      return AP39;
      break;
    case 40:
      return AP40;
      break;
    case 41:
      return AP41;
      break;
    case 42:
      return AP42;
      break;
    case 43:
      return AP43;
      break;
    case 44:
      return AP44;
      break;
    case 45:
      return AP45;
      break;
    case 46:
      return AP46;
      break;
    case 47:
      return AP47;
      break;
    case 48:
      return AP48;
      break;
    case 49:
      return AP49;
      break;
    default:
      return "";
      break;
  }

}


void SaveFlash()
{
      // ...and finally save everything into "my_flash_store"
      Serial.print("SAVED SETTINGS ");
      my_flash_store.write(Settings);     
}




int GetXmlTagValue(char *RespBuffer, char *Tag, char *TagValue)
{

  //    Serial.println("enter xml ");

  int len = 0, pos = 0;
  char FirstTag[100] = {0}; //First Tag
  char SecondTag[100] = {0};//Second Tag
  int PosFirstTag = 0, PosSecondTag = 0;
  //check enter buffer
  len = strlen(RespBuffer);
  if (len <= 0)
  {
    return -1;
  }
  //Create first Tag
  memset(FirstTag, 0, sizeof(FirstTag));
  strcpy(FirstTag, "<");
  strcat(FirstTag, Tag);
  strcat(FirstTag, ">");
  //Create second tag
  memset(SecondTag, 0, sizeof(SecondTag));
  strcpy(SecondTag, "</");
  strcat(SecondTag, Tag);
  strcat(SecondTag, ">");
  //Get first tag position
  for (pos = 0; pos < len; pos++)
  {
    if ( !memcmp(FirstTag, RespBuffer + pos, strlen(FirstTag)) )
    {
      PosFirstTag = pos;
      break;
    }
  }
  //Get second tag position
  for (pos = 0; pos < len; pos++)
  {
    if ( !memcmp(SecondTag, RespBuffer + pos, strlen(SecondTag)) )
    {
      PosSecondTag = pos;
      break;
    }
  }
  if ( (PosFirstTag != 0) && (PosSecondTag != 0) )
  {
    if (PosSecondTag - PosFirstTag - strlen(FirstTag))
    {
      //Get tag value
      memcpy( TagValue, RespBuffer + PosFirstTag + strlen(FirstTag), PosSecondTag - PosFirstTag - strlen(FirstTag) );
      if (strlen(TagValue))
      {
        return 1;
      }
    }
  }
  return -1;
}


void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


void FlickerSelection(String returnVal)
{

  if (Settings.lBlink)
  {
      pixels.setPixelColor(nCurrentAirport, pixels.Color(0, 0, 0));
      pixels.show();
      delay(100);
    
      SetLEDColor(returnVal, nCurrentAirport);
      pixels.show();
      delay(100);
    
      pixels.setPixelColor(nCurrentAirport, pixels.Color(0, 0, 0));
      pixels.show();
      delay(100);
    
      SetLEDColor(returnVal, nCurrentAirport);
      pixels.show();
      delay(100);
    
      pixels.setPixelColor(nCurrentAirport, pixels.Color(0, 0, 0));
      pixels.show();
      delay(100);
  }

  SetLEDColor(returnVal, nCurrentAirport);

}


int nLoops = 0;
//int nRetries = 0;

void loop() {

  String cWebResponse = "";   // build string from web response
  String PassNode = "";
  char returnVal[10] = {0};    // result passed into and updated by xml function

  BtnPress();

  if  (Settings.nDisplayMode != MAXDISPLAYMODES) 
  {
    
    if (CheckClock())
      {         
          // if there's incoming data from the net connection.
          // send it out the serial port.  This is for debugging
          // purposes only:
          while (client.available()) {
            char c = client.read();
            //    Serial.write(c);
            cWebResponse = cWebResponse + c;
            BtnPress();
          }
        
          if (cWebResponse.length() > 0)
          {
            //  display
        
            //*********************  search result based upon what mode is being displayed
        
            if (Settings.nDisplayMode == 1)  // temperature
              PassNode = "temp_c";
            else if (Settings.nDisplayMode == 2)    // flight category
              PassNode = "flight_category";
            else if (Settings.nDisplayMode == 3)    // winds
              PassNode = "wind_speed_kt";
            else
              PassNode = "temp_c";
        
            char tempnode[20] = "";
            PassNode.toCharArray(tempnode, 20);
        
            char buffer[10000] = "";
            cWebResponse.toCharArray(buffer, 10000);

               Serial.println(buffer);

               
            // found the tag and value in the response xml from service
            if (GetXmlTagValue(buffer, tempnode, returnVal) == 1)
            {
              // got the value from xml in the service, set the LED appropriately
        
              //       Serial.println("flicker");
        
              FlickerSelection(returnVal);
        
              Serial.println();
              Serial.print("LED SET ");
              Serial.print(returnVal);
              Serial.print(" ");
              Serial.print(nCurrentAirport);
              Serial.print(" ");
              Serial.print( GetAirportCode(nCurrentAirport));
              Serial.print(" LOOP ");
              Serial.print( nLoops);
              Serial.println();
        
        
              // once the LED is set, you are done, move to the next airport.
              nCurrentAirport++;
              if (nCurrentAirport > NUMPIXELS - 1)
                nCurrentAirport = 0;
        
            }
            else
            {
              Serial.println();
              Serial.print("*********************************************   NODE NOT FOUND ");
              Serial.print(tempnode);
              Serial.print(" ");
              Serial.print(returnVal);
              Serial.print(" ");
              Serial.print( GetAirportCode(nCurrentAirport));
              Serial.print(" LOOP ");
              Serial.print( nLoops);
              Serial.println();

              nCurrentAirport++;
                        
              SetLEDColor("", nCurrentAirport);    /// turn selection off
//        
//              if (nRetries == 1)   // already retried, increment and move to next airport
//              {
//                nRetries = 0;
//        
//                // once the LED is set, you are done, move to the next airport.
//                nCurrentAirport++;
//                if (nCurrentAirport > NUMPIXELS - 1)
//                  nCurrentAirport = 0;
//        
//              }
//              else
//              {
//                // dont increment airport, but set retries flag
//                nRetries = 1;
//                //          Serial.print("*********************************************  FAILED ONCE ");
//              }
//        
        
            }
        
        
          }
        
        
          // if ten seconds have passed since your last connection,
          // then connect again and send data:
          if (millis() - lastConnectionTime > postingInterval) {
            httpRequest();
            nLoops++;
            nIgnoreTimeSetting++;

//             Serial.print("time setting increment ");
//             Serial.println(nIgnoreTimeSetting);
              
            //    Serial.println();
            //    Serial.print("******************************888888888888888888888888888888*********** LOOPS ");
            //    Serial.println(nLoops);
          }


      }   // check clock
      else
      {
        pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)
        pixels.clear(); // Set all pixel colors to 'off'
        pixels.show();   // Send the updated pixel colors to the hardware.      
      }
      
   }  // DISPLAY MODE MAX?

}



// this method makes a HTTP connection to the server:
void httpRequest() {
  // close any connection before send a new request.
  // This will free the socket on the NINA module

  client.flush();
  client.stop();

  // if there's a successful connection:
  if (client.connect(server, 443)) {
    //    Serial.println("connecting...");
    // send the HTTP GET request:
    //    client.println("GET / HTTP/1.1");
    //    client.println("Host: example.org");

// ORIGINAL
//    client.println("GET /adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow=1&stationString=K" + GetAirportCode(nCurrentAirport) + " HTTP/1.1");

  
//    client.println("GET /adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&mostRecentForEachStation=constraint&hoursBeforeNow=1&stationString=K" + GetAirportCode(nCurrentAirport) + " HTTP/1.1");
//
//
//curl -X 'GET' \
//  'https://aviationweather.gov/api/data/metar?ids=KDCA&format=xml' \
//  -H 'accept: */*'
//
//https://aviationweather.gov/api/data/metar?ids=KDCA&format=xml

// fixed input format due to service change 3/2/2024 djp

    client.println("GET https://aviationweather.gov/api/data/metar?ids=K"+GetAirportCode(nCurrentAirport)+ "&format=xml HTTP/1.1");
//    Serial.println("GET https://aviationweather.gov/api/data/metar?ids=K"+GetAirportCode(nCurrentAirport)+ "&format=xml HTTP/1.1");



    
    client.println("Host: aviationweather.gov");
    client.println("User-Agent: ArduinoWiFi/1.1");
    client.println("Connection: close");
    client.println();




    // note the time that the connection was made:
    lastConnectionTime = millis();
  } else
  {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    client.flush();
    client.stop();


    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println(WiFi.status());
      Serial.println("Conn lost");
      WiFi.end();
      WiFi.disconnect();
      status = WiFi.begin(Settings.ssid, Settings.pass);
      Serial.println("***************************************************************** Tried restart");
      delay(5000);
    }


    //
    //        // attempt to connect to WiFi network:
    //        while (status != WL_CONNECTED) {
    //          Serial.print("Attempting to connect to SSID: ");
    //          Serial.println(Settings.ssid);
    //          // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    //          status = WiFi.begin(Settings.ssid, Settings.pass);
    //
    //          // wait x seconds for connection:
    //          delay(5000);
    //        }

  }

}






bool SetLEDColor(String cValue, int nLed)
{
  //  strip.setPixelColor(i, (0, 63, 0));  //1/4 bright green
  //strip.setPixelColor(i, (255, 0, 0));  //full-bright red
  //strip.setPixelColor(i, (0, 255, 255)); //full-bright cyan
  //strip.setPixelColor(i, (127, 127, 0)); //half-bright yellow
  //strip.setPixelColor(i, (255, 192, 255)); //orange
  //strip.setPixelColor(i, (63, 63, 63)); //1/4-bright white

  //    Serial.println(cValue);
  //    Serial.println(nLed);
  //

  //
  //// change via pushbutton
  //int Settings.nBrightSetting = 1;
  //
  //// use as a divisor for all colors eg.   150 / 2 makes it half as bright  pressing buttons loops through the dimmness options.
  //int brightvalues[] = {1,2,3,4,8};
  //
  //


  if (cValue == "")
    pixels.setPixelColor(nLed, pixels.Color(0, 0, 0));   // off
  else if (Settings.nDisplayMode == 1)   // temperature
  {

    if (cValue.toFloat() < -5.0)
      pixels.setPixelColor(nLed, pixels.Color(0, int(150 / brightvalues[Settings.nBrightSetting]), int(150 / brightvalues[Settings.nBrightSetting]) )); // purple  30f
    else if (cValue.toFloat() < 5.0)
      pixels.setPixelColor(nLed, pixels.Color(0, 0,  int(150 / brightvalues[Settings.nBrightSetting]) )); // blue    50f
    else if (cValue.toFloat() < 10.0)
      pixels.setPixelColor(nLed, pixels.Color( int(150 / brightvalues[Settings.nBrightSetting]), 0, 0)); // green   60f8452
    else if (cValue.toFloat() < 20.0)
      pixels.setPixelColor(nLed, pixels.Color( int(150 / brightvalues[Settings.nBrightSetting]) ,  int(150 / brightvalues[Settings.nBrightSetting]), 0)); // yellow    80f
    else if (cValue.toFloat() < 25.0)
      pixels.setPixelColor(nLed, pixels.Color( int(250 / brightvalues[Settings.nBrightSetting]),  int(250 / brightvalues[Settings.nBrightSetting]), 0)); // orange    80f
    else
      pixels.setPixelColor(nLed, pixels.Color(0,  int(150 / brightvalues[Settings.nBrightSetting]) , 0)); // red


  }
  else if (Settings.nDisplayMode == 2)   // flight category
  {

    //green - VFR (Visual Flight Rules)
    //blue - MVFR (Marginal Visual Flight Rules)
    //red - IFR (Instrument Flight Rules)
    //purple - LIFR (Low Instrument Flight Rules)

    if (cValue == "VFR")
      pixels.setPixelColor(nLed, pixels.Color(int(150 / brightvalues[Settings.nBrightSetting]), 0, 0));     // green
    else if (cValue == "MVFR")
      pixels.setPixelColor(nLed, pixels.Color(0, 0, int(150 / brightvalues[Settings.nBrightSetting])));     // blue
    else if (cValue == "IFR")
      pixels.setPixelColor(nLed, pixels.Color(0, int(150 / brightvalues[Settings.nBrightSetting]), 0));      // red
    else if (cValue == "LIFR")
      pixels.setPixelColor(nLed, pixels.Color(0, int(150 / brightvalues[Settings.nBrightSetting]), int(150 / brightvalues[Settings.nBrightSetting])));     // purple

    else
      pixels.setPixelColor(nLed, pixels.Color(0, 0, 0));

  }
  else if (Settings.nDisplayMode == 3)  //wind_speed_kt
  {

    if (cValue.toFloat() < 5.0)
      pixels.setPixelColor(nLed, pixels.Color(int(150 / brightvalues[Settings.nBrightSetting]), int(150 / brightvalues[Settings.nBrightSetting]), int(150 / brightvalues[Settings.nBrightSetting]))); // white  30f
    else if (cValue.toFloat() < 17.0)
      pixels.setPixelColor(nLed, pixels.Color(0, int(150 / brightvalues[Settings.nBrightSetting]), int(150 / brightvalues[Settings.nBrightSetting]))); // purple    50f
    else if (cValue.toFloat() < 25.0)
      pixels.setPixelColor(nLed, pixels.Color(0, 0, int(150 / brightvalues[Settings.nBrightSetting]))); // blue    50f
    else if (cValue.toFloat() < 33.0)
      pixels.setPixelColor(nLed, pixels.Color(int(150 / brightvalues[Settings.nBrightSetting]), 0, 0)); // green   60f
    else if (cValue.toFloat() < 37.0)
      pixels.setPixelColor(nLed, pixels.Color(int(50 / brightvalues[Settings.nBrightSetting]), int(50 / brightvalues[Settings.nBrightSetting]), 0)); // yellow    80f
    else
      pixels.setPixelColor(nLed, pixels.Color(0, int(150 / brightvalues[Settings.nBrightSetting]), 0)); // red


  }
  else if (Settings.nDisplayMode == MAXDISPLAYMODES)     // visibility
  {
//    pixels.setPixelColor(nLed, pixels.Color(0, 0, 0)); // BLACK but it should never get here.
  }


  pixels.show();   // Send the updated pixel colors to the hardware.

}




void BtnPress()
{

  int btnState = digitalRead(DISPLAYBUTTON_PIN);

  if (btnState == LOW)
  {

    Settings.nDisplayMode++;
    if (Settings.nDisplayMode >  MAXDISPLAYMODES)
    {
      Settings.nDisplayMode = 1;
    }

    SetDisplayIndicator();

    SaveFlash();

    nCurrentAirport = 0;
    Serial.print("changing display mode to: ");
    Serial.println(Settings.nDisplayMode);
    InitializePixel();
    
    nIgnoreTimeSetting = 0;   // start the time setting ignore temporarily

    delay(1000);

  }


  btnState = digitalRead(BRIGHTBUTTON_PIN);

  if (btnState == LOW)
  {
    Settings.nBrightSetting++;

    if (Settings.nBrightSetting > (  (sizeof(brightvalues) / sizeof(int)) - 1) )
      Settings.nBrightSetting = 0;

    SaveFlash();

    delay(1000);

    Serial.print("brightness: " );
    Serial.println(Settings.nBrightSetting);
     
    nIgnoreTimeSetting = 0;   // start the time setting ignore temporarily

  }


}

 

bool CheckClock()
{

  // button press starts temporary ignoring of time settings - wake up
  if (nIgnoreTimeSetting < MAXIGNORETIMESETTING)
  {
    //    Serial.print( "ignore time settings now " );
    //    Serial.println( nIgnoreTimeSetting );    
    return true;
  }

  // user turned off clock restrictions
  if ((Settings.On24hr==0) && (Settings.Off24hr==0))
    return true;

  // Variable to represent epoch
  unsigned long epoch =0;
 
 // Get epoch
  epoch = WiFi.getTime();
  
  while (epoch ==0)
  {  
     // try restarting the wifi

    if (WiFi.status() != WL_CONNECTED)
    {
      Serial.println(WiFi.status());
      Serial.println("Conn lost");
      WiFi.end();
      WiFi.disconnect();
      status = WiFi.begin(Settings.ssid, Settings.pass);
      Serial.println("***************************************************************** EPOCH Tried restart");
      delay(5000);
    }

    epoch = WiFi.getTime();
    Serial.println( "epoc " );
  
  }


//  Serial.print("epoch ");
//  Serial.println(epoch);
//
//  unsigned int nTodayhours = ((epoch%86400)/3600)-4;
//  Serial.print("hours spent today ");
//  Serial.println(nTodayhours);

  unsigned int nSecs =  ((epoch%86400));
//  Serial.print("seconds spent today ");
//  Serial.println( nSecs   );

  
  nSecs = nSecs -  (Settings.nZuluOffset*3600);
//  Serial.print("seconds offset for Zulu ");
//  Serial.println( nSecs );

  // handle the hours part of military time eg. 0815
  unsigned int nShutdown = (Settings.Off24hr/100)*3600;
  // miltary time 1215, add the 15 minutes at the end.
  nShutdown = nShutdown +  ((Settings.Off24hr%100) * 60);


  // handle the hours part of military time eg. 0815
  unsigned int nStartup = (Settings.On24hr/100)*3600;
  // miltary time 1215, add the 15 minutes at the end.
  nStartup = nStartup +  ((Settings.On24hr%100) * 60);


  //11:42:31.132 -> epoch 1650123750
  //11:42:31.132 -> hours spent today 11
  //11:42:31.132 -> seconds spent today 56550
  //11:42:31.132 -> seconds offset for Zulu 42150
  //11:42:31.132 -> shutdown time 43200
  //  


  

  //  return false if the startup time is 
  // current time after startup time
  // current time less than shutdown time

  if (Settings.On24hr < Settings.Off24hr) // both time during the same day
  {
    if ( (nSecs > nStartup) && (nSecs < nShutdown) )
       return true;
  }
  else
  {   
    
    // time period crosses midnight

    if (  (nSecs < nShutdown) )
       return true;
        
    // what if end time is less than start time?
    //      Settings.On24hr = 618;
    //      Settings.Off24hr = 100;
    
  }

  
  Serial.println("");
  Serial.print("start ");
  Serial.println(nStartup);
  Serial.print(" stop ");
  Serial.println(nShutdown);
  Serial.print(" time ");
  Serial.println(nSecs);
  
  return false;
  
}




// ***********************  GET SETTINGS FROM FLASH MEMORY  **********************

void ProcessFlash()
{

  Serial.println("process flash ***************");
  // Read the content of "my_flash_store" into the Settings variable
  
  Settings = my_flash_store.read();

  // If this is the first run the "valid" value should be "false"...
  if (Settings.valid == false) {
  
      //String ssid = MYSSID;        // your network SSID (name)
      //String pass = WIFIPASS;    // your network password (use for WPA, or use as key for WEP)
      
      //  
      //      // set default from .h file
      //      ssid.toCharArray(Settings.ssid, 50);
      //      pass.toCharArray(Settings.pass, 50);


      String ssid = MYSSID;
      String pass = WIFIPASS;
      
      ssid.toCharArray(Settings.ssid, 50);
      pass.toCharArray(Settings.pass, 50);
      
//      Settings.valid = true;
//      Settings.nDisplayMode = 1;
//      Settings.nBrightSetting = 1;
//      Settings.lBlink = true;
//      Settings.nWebServicePause = 10;
//      Settings.On24hr = 600;
//      Settings.Off24hr = 1910;
//      Settings.nZuluOffset =4;      // DC setting 
      
      Settings.valid = true;
      Settings.nDisplayMode = DISPLAYMODE;
      Settings.nBrightSetting = BRIGHTSETTING;
      Settings.lBlink = BLINK;
      Settings.nWebServicePause = WEBSERVICEPAUSE;
      Settings.On24hr = ON24HR;
      Settings.Off24hr = OFF24HR;
      Settings.nZuluOffset =ZULUOFFSET;      // DC setting 
      
      // ...and finally save everything into "my_flash_store"
      my_flash_store.write(Settings);
  
      // Print a confirmation of the data inserted.
      
      Serial.println("FIRST TIME EXECUTION FLASH MEMORY");
  
      Serial.println();
      Serial.print("Your ssid: ");
      Serial.println(Settings.ssid);
      
      Serial.print("and your pass: ");
      Serial.println(Settings.pass);

      Serial.print("displaymode: ");
      Serial.println(Settings.nDisplayMode);
  
      Serial.print("brightness: ");
      Serial.println(Settings.nBrightSetting);
      
      Serial.print("blink: ");
      Serial.println(Settings.lBlink);
  
      Serial.print("pause: ");
      Serial.println(Settings.nWebServicePause);
  
      Serial.print("ontime: ");
      Serial.println(Settings.On24hr);
  
      Serial.print("offtime: ");
      Serial.println(Settings.Off24hr);
    

  } else {
  
      // Print a confirmation of the data inserted.

      
      Serial.println("SETTINGS FROM MEMORY");
  

      Serial.print("Your ssid: ");
      Serial.println(Settings.ssid);
      
      Serial.print("and your pass: ");
      Serial.println(Settings.pass);
  
      Serial.print("last display mode: ");
      Serial.println(Settings.nDisplayMode);
      
      Serial.print("last brightness: ");
      Serial.println(Settings.nBrightSetting);
  
      Serial.print("last blink setting: ");
      Serial.println(Settings.lBlink);
      
      Serial.print("cycle pause: ");
      Serial.println(Settings.nWebServicePause);
  
      Serial.print("ontime: ");
      Serial.println(Settings.On24hr);
  
      Serial.print("offtime: ");
      Serial.println(Settings.Off24hr);

      Serial.print("zulu offset: ");
      Serial.println(Settings.nZuluOffset);


  
//      Serial.print("DATA retrieved from prior execution ");

//
////      ssid = Settings.ssid;
////      pass = Settings.pass;
//      Settings.nDisplayMode = Settings.nDisplayMode;
//      Settings.nBrightSetting = Settings.nBrightSetting; 
//      Settings.lBlink = Settings.Settings.lBlink ;
//      nWebServicePause = Settings.nWebServicePause;    
//      nOnTime = Settings.On24hr ;
//      nOffTime = Settings.Off24hr ;
    
  }

}

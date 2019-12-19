
#include <Arduino.h>
/// ESP32 WiFi Libraries
#include <WiFi.h>

/// ESP32 Graphic  Libraries
#include <WROVER_KIT_LCD.h>
#include <Adafruit_GFX.h> // Hardware-specific library
#include "comic8pt.c"
// Output Time Status Line 
#include "StatusBar4Time.h"
#include <time.h>
// Font For Output
#include "comici10pt.c"
// #include "comici_grad10pt.c" // Not Used here
// NTP Functions
#include "NTPProcedure.h"
/// Script Credentials
#include "WiFi_config.inc"

//// 

/// Used Object
WROVER_KIT_LCD tft = WROVER_KIT_LCD();
StatusBar4TimeClass StatusBar;

// Prototypes
void SetupNTP();
void InitTimer0();
void InitILI9341();
void IRAM_ATTR ontimer0();
void connectToWiFi(const char * ssid, const char * pwd);
void WiFiEvent(system_event_id_t event);
void PrintText(uint16_t X, uint16_t Y, const char* Text, uint16_t Color, uint16_t EraseColor);

/// setup() --------------------------------
void setup() {
 

  Serial.begin(115200);
 InitILI9341();
 //tft.setTextSize(2);
tft.setFont(&_comic8pt_);
connectToWiFi(sBufferSSID, sBufferPASS);

// Give it 500 tries to connect
uint32_t Versuch = 0;
while ((bWiFiConnected == false) & (Versuch++ < 500))
{
	Serial.printf(". %d", Versuch);
	delay(50);
}

SetupNTP();
InitTimer0(); // Init Timer0
  
}

void loop() {
      // put your main code here, to run repeatedly:

if (bSecondTicker){
    StatusBar.SetTime(NewTime, OldTime);
    bSecondTicker = false;		
}

if (bWiFiConnected)
{

   if (bTriggerNTP)
   {	

 bGotNTPData = getNTPtime(10, NewTime);
 bTriggerNTP = false;
     }     
	   }

if (bGotNTPData)
{
	
PrintText(10,50,"Got NTP Data",ILI9341_LIGHTSALMON, ILI9341_DARKGREEN);
bGotNTPData = false;
}

if (bGotNTPTextOff)
{
PrintText(10,70,"# -- #",ILI9341_LIME, ILI9341_DARKGREEN);
bGotNTPTextOff = false;

}
// Give ESP Timer time to count
timerAlarmEnable(timer0);
 delay(10);
 timerAlarmDisable(timer0);
}


//// void SetupNTP() -------------------------
//// if ESP is not connected to WiFi it will crah
void SetupNTP()
{
SetNTP_SERVER();
 if (getNTPtime(10,NewTime))
 {
 bGotNTPData = true;
 bYouCanTriggerNTPCounter = true;
 }
else
bGotNTPData = false;

 StatusBar.init(&tft,ILI9341_AQUAMARINE, ILI9341_RED, ILI9341_RED,FullAll);
 StatusBar.SetTime(NewTime, OldTime);

}


//// void connectToWiFi(const char * ssid, const char * pwd)
/// connect to WiFi via Callback
void connectToWiFi(const char * ssid, const char * pwd)
{
  	Serial.printf(Buffer, "Connecting to WiFi network : %s", ssid);

  // delete old config
  WiFi.disconnect(true);

  //register event handler
  WiFi.onEvent(WiFiEvent);
  //Initiate connection
  WiFi.begin(ssid, pwd);
  WiFi.setHostname("ESP32_NTP_xxx");
   }

//WiFiEvent(system_event_id_t event) ----------------------
//WiFi event handler --------------------------------------
void WiFiEvent(system_event_id_t event)
{
	Serial.print("event ");
Serial.print(event);

switch (event)
{
case     SYSTEM_EVENT_WIFI_READY:
 Serial.print("Wifi Ready");
	break;
case     SYSTEM_EVENT_STA_START:
 Serial.print("Wifi Event Start");
	break;
case     SYSTEM_EVENT_STA_STOP:
 Serial.print("Wifi Event Stop");
	break;
case     SYSTEM_EVENT_STA_CONNECTED:
{
	bWiFiConnected = true;
PrintText(10,20, "WiFi Connected",ILI9341_LIGHTGOLDENRODYELLOW,ILI9341_DARKGREEN);
}
	break;
case     SYSTEM_EVENT_STA_DISCONNECTED:
{
PrintText(10,20, "WiFi Disconnected", ILI9341_FIREBRICK,ILI9341_DARKGREEN);
}

   break;
case     SYSTEM_EVENT_STA_GOT_IP:
 Serial.println(WiFi.localIP());
 //udp.begin(WiFi.localIP(),UDP_PORT);
 bWiFiConnected = true;

	break;
case     SYSTEM_EVENT_STA_LOST_IP:
 Serial.print("Wifi lost IP");
 bWiFiConnected = false;
	break;

 default:
 Serial.print("Not recognized Wifi Event ");

}
}

// void InitTimer0() -----------------------
// Interupt Timer via Callback
void InitTimer0()
{
	// Inalize Timer
	timer0 = timerBegin(0, 80, true);
	Serial.println("Timer ******");
	// Attach onTimer function to our timer
	timerAttachInterrupt(timer0, &ontimer0, true);
	// Set alarm to call onTimer function every second 1 tick is 1us
	// => 1 second is 1000000us
	// Repeat the alarm (third parameter)
	timerAlarmWrite(timer0, 1000000, true);
    // timerAlarmDisable(timer0);

}
// void InitILI9341() -------------------------
// Init ILI9341 Grafhic 

void InitILI9341()
{
	Serial.print("****** TFT Mode  ******");
	tft.begin();

	// read diagnostics (optional but can help debug problems)
	uint8_t x = tft.readcommand8(WROVER_RDDST);
	Serial.print("Status:          0x"); Serial.println(x, HEX);
	x = tft.readcommand8(WROVER_RDDPM);
	Serial.print("Power Mode:      0x"); Serial.println(x, HEX);
	x = tft.readcommand8(WROVER_RDDMADCTL);
	Serial.print("MADCTL Mode:     0x"); Serial.println(x, HEX);
	x = tft.readcommand8(WROVER_RDDCOLMOD);
	Serial.print("Pixel Format:    0x"); Serial.println(x, HEX);
	x = tft.readcommand8(WROVER_RDDIM);
	Serial.print("Image Format:    0x"); Serial.println(x, HEX);
	x = tft.readcommand8(WROVER_RDDSDR);
	Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX);

	tft.setTextSize(1);
	tft.fillScreen(ILI9341_NAVY);
	tft.setTextWrap(false);
	tft.setRotation(1); // horizontal display - in this App it's a "MUST"
  

}

/// Timer0 Interrupt Callback
void IRAM_ATTR ontimer0()
{
	//This is for refresh Time with SNMP all 5 Minutes
	if (TriggerSNMPCounter++ > 60*5)
	{
		TriggerSNMPCounter = 0;
		bTriggerSNMP = true;
	}

    if (TriggerNTPCounter++ > 60 * 10) // every 10 Minutes Get NTP Datas
	{
		TriggerNTPCounter = 0;
		bTriggerNTP = true;
		bYouCanTriggerNTPCounter = true;
		
	}	

if ((GotNTPDataCounter++ > 7) & (bYouCanTriggerNTPCounter == true) )
{
	GotNTPDataCounter = 0;
	bGotNTPTextOff = true;
	bYouCanTriggerNTPCounter = false;
}	
  OldTime = NewTime;
  NewTime++;
  bSecondTicker = true;
}

// PrintText(uint16_t X, uint16_t Y, const char* Text, uint16_t Color, uint16_t EraseColor) --
// Print LCD Text makes it easier 
void PrintText(uint16_t X, uint16_t Y, const char* Text, uint16_t Color, uint16_t EraseColor)
{
	tft.fillRect(X-10, Y-12, 200, 16, EraseColor);
	tft.setTextColor(Color);
	tft.setCursor(X, Y);
    tft.print(Text);
}




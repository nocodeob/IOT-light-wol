/***************************************************
  Adafruit MQTT Library ESP8266 Example

  Must use ESP8266 Arduino from:
    https://github.com/esp8266/Arduino

  Works great with Adafruit's Huzzah ESP board & Feather
  ----> https://www.adafruit.com/product/2471
  ----> https://www.adafruit.com/products/2821

  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Tony DiCola for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/
#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "WiFiUDP.h"
#include "WakeOnLan.h"
/************ WOL ******************/
IPAddress computer_ip(192, 168, 5, 67);
IPAddress broadcast_ip(192, 168, 5, 255);
byte mac[] = { 0x00, 0x30, 0xF1, 0x09, 0x0E, 0x5A };
WiFiUDP UDP;


/************************* WiFi Access Point *********************************/

#define WLAN_SSID       ""
#define WLAN_PASS       ""

/************************* Adafruit.io Setup *********************************/

#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883                   // use 8883 for SSL
#define AIO_USERNAME    ""
#define AIO_KEY         ""

/************ Global State (you don't need to change this!) ******************/

// Create an ESP8266 WiFiClient class to connect to the MQTT server.
WiFiClient client;
// or... use WiFiFlientSecure for SSL
//WiFiClientSecure client;

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

/****************************** Feeds ***************************************/

// Setup a feed called 'photocell' for publishing.
// Notice MQTT paths for AIO follow the form: <username>/feeds/<feedname>
Adafruit_MQTT_Publish Komp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Komp");
Adafruit_MQTT_Publish Light = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/Light");

// Setup a feed called 'onoff' for subscribing to changes.
//const char Komp1[] PROGMEM = AIO_USERNAME "/feeds/Komp";
Adafruit_MQTT_Subscribe KompCitaj = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Komp");
Adafruit_MQTT_Subscribe LightCitaj = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/feeds/Light");

/*************************** Sketch Code ************************************/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();
void sendWOL();

void setup() {
  pinMode(5,OUTPUT);
  Serial.begin(115200);
  delay(10);


  Serial.println(F("Adafruit MQTT demo"));

  // Connect to WiFi access point.
  Serial.println(); Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WLAN_SSID);

  WiFi.begin(WLAN_SSID, WLAN_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  Serial.println("WiFi connected");
  Serial.println("IP address: "); Serial.println(WiFi.localIP());

  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&KompCitaj);
  mqtt.subscribe(&LightCitaj);
  //  UDP.begin(9); //start UDP client, not sure if really necessary.
}

uint32_t x = 0;
int brojac = 1;

void loop() {
  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here

  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(5000))) {
    if (subscription == &KompCitaj) 
    {
      uint16_t num = atoi ((char *)KompCitaj.lastread);
      if (num == 1) 
      {
        for (int i = 0; i < 5; i++ )
        {
        WakeOnLan::sendWOL(computer_ip, UDP, mac, sizeof mac);
        WakeOnLan::sendWOL(broadcast_ip, UDP, mac, sizeof mac);
        delay(500); //sending WOL packets every 4th second.
        }
        
        do 
        {
          if (! Komp.publish(0))
          {
            Serial.println(F("Failed"));
            brojac++;

           } else 
           {
              Serial.println(F("OK!"));
               brojac = 0;
            }
         } while (brojac != 0);

        }
      }
    //-------------------------------LIGHT------------------------------------
    if (subscription == &LightCitaj) 
    {
      uint16_t num = atoi ((char *)LightCitaj.lastread);
      if (num == 1)
        digitalWrite(5, HIGH);
      else
        digitalWrite(5, LOW);    
     }
  }


  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds

  if (! mqtt.ping()) {
    mqtt.disconnect();
  }

}

// Function to connect and reconnect as necessary to the MQTT server.
// Should be called in the loop function and it will take care if connecting.
void MQTT_connect() {
  int8_t ret;

  // Stop if already connected.
  if (mqtt.connected()) {
    return;
  }

  Serial.print("Connecting to MQTT... ");

  uint8_t retries = 3;
  while ((ret = mqtt.connect()) != 0) { // connect will return 0 for connected
    Serial.println(mqtt.connectErrorString(ret));
    Serial.println("Retrying MQTT connection in 5 seconds...");
    mqtt.disconnect();
    delay(5000);  // wait 5 seconds
    retries--;
    if (retries == 0) {
      // basically die and wait for WDT to reset me
      while (1);
    }
  }
  Serial.println("MQTT Connected!");
}

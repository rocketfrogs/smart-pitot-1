/************************* WiFi Access Point *********************************/

#include <ESP8266WiFi.h>        // Include the Wi-Fi library
#include "credentials.h"  "     // Include Credentials (you need to create that file in the same folder if you cloned it from git)

/*
Content of "credentials.h" that matters for this section

// WIFI Credentials

#define WIFI_SSID        "[REPLACE BY YOUR WIFI SSID (2G)]"     // The SSID (name) of the Wi-Fi network you want to connect to
#define WIFI_PASSWORD    "[REPLACE BY YOUR WIFI PASSWORD]"      // The password of the Wi-Fi 
*/

const char* ssid     = WIFI_SSID;         // The SSID (name) of the Wi-Fi network you want to connect to
const char* password = WIFI_PASSWORD;     // The password of the Wi-Fi 

/************************* MQTT Setup *********************************/

#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include "credentials.h"

/*
// MQTT Credentials

Content of "credentials.h" that matters for this section

#define AIO_SERVER      "[REPLACE BY YOUR MQTT SERVER IP ADDRESS OR ITS FQDN]"
#define AIO_SERVERPORT  [REPLACE BY THE PORT NUMBER USED FOR THE MQTT SERVICE ON YOUR MQTT SERVEUR (DEFAULT IS 1883)]       // use 8883 for SSL"
#define AIO_USERNAME    ""  // USE THIS IF YOU HAVE USERNAME AND PASSWORD ENABLED ON YOUR MQTT SERVER
#define AIO_KEY         ""  // USE THIS IF YOU HAVE USERNAME AND PASSWORD ENABLED ON YOUR MQTT SERVER
*/

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
Adafruit_MQTT_Publish stat_raw_sensor_1_reading = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/pitot_1/stat/raw_sensor_1_reading");
Adafruit_MQTT_Publish stat_raw_sensor_2_reading = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/pitot_1/stat/raw_sensor_2_reading");
Adafruit_MQTT_Publish stat_sensor_1_pressure_reading = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/pitot_1/stat/sensor_1_pressure_reading");
Adafruit_MQTT_Publish stat_sensor_2_pressure_reading = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/pitot_1/stat/sensor_2_pressure_reading");

/*
// Setup a feed called 'onoff' for subscribing to changes.
Adafruit_MQTT_Subscribe cmnd_target_temperature = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/cmnd/heater_1/target_temperature");
Adafruit_MQTT_Subscribe cmnd_status = Adafruit_MQTT_Subscribe(&mqtt, AIO_USERNAME "/cmnd/heater_1/status");
*/

// Bug workaround for Arduino 1.6.6, it seems to need a function declaration
// for some reason (only affects ESP8266, likely an arduino-builder bug).
void MQTT_connect();

int raw_sensor_1_reading = LOW;
int raw_sensor_2_reading = LOW;

float mean_val1 = 0;
float mean_val2 = 0;

void setup() {
Serial.begin(115200); // Start the Serial communication to send messages to the computer

  delay(10);
  Serial.println('\n');
  
  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Connecting to ");
  Serial.print(ssid); Serial.println(" ...");

  int i = 0;
  while (WiFi.status() != WL_CONNECTED) { // Wait for the Wi-Fi to connect
    delay(1000);
    Serial.print(++i); Serial.print(' ');
  }

  Serial.println('\n');
  Serial.println("Connection established!");  
  Serial.print("IP address:\t");
  Serial.println(WiFi.localIP());         // Send the IP address of the ESP8266 to the computer

/*
  // Setup MQTT subscription for onoff feed.
  mqtt.subscribe(&cmnd_target_temperature);
  mqtt.subscribe(&cmnd_status);
*/

// sensors calibration-ish

float val1 = 0;
float val2 = 0;
int j = 0;

for(j=0;j<50;j++){
  // set multiplexer to sensor 1 
pinMode(D1, OUTPUT); 
pinMode(D0, OUTPUT); //led
digitalWrite(D1, LOW);
digitalWrite(D0, LOW); //led
// read Analog value for sensor 1
raw_sensor_1_reading = analogRead(A0);

val1 += raw_sensor_1_reading;

delay(50);

// set multiplexer to sensor 2 
pinMode(D1, OUTPUT); 
pinMode(D0, OUTPUT); //led
digitalWrite(D1, HIGH);
digitalWrite(D0, HIGH); //led
// read Analog value for sensor 1
raw_sensor_2_reading = analogRead(A0);

val2 += raw_sensor_2_reading;

delay(50);
}
mean_val1 = val1/50; //value for 1atm
mean_val2 = val2/50; //value for 1atm


}


void loop() {

  // Ensure the connection to the MQTT server is alive (this will make the first
  // connection and automatically reconnect when disconnected).  See the MQTT_connect
  // function definition further below.
  MQTT_connect();

  // this is our 'wait for incoming subscription packets' busy subloop
  // try to spend your time here


// set multiplexer to sensor 1 
pinMode(D1, OUTPUT); 
pinMode(D0, OUTPUT); //led
digitalWrite(D1, LOW);
digitalWrite(D0, LOW); //led
// read Analog value for sensor 1
raw_sensor_1_reading = analogRead(A0);

delay(50);

// set multiplexer to sensor 2 
pinMode(D1, OUTPUT); 
pinMode(D0, OUTPUT); //led
digitalWrite(D1, HIGH);
digitalWrite(D0, HIGH); //led
// read Analog value for sensor 1
raw_sensor_2_reading = analogRead(A0);

delay(50);

// We calculate the pressure value for both sensors
float sensor_1_pressure_reading = (raw_sensor_1_reading * 101300 / mean_val1 );    // value set for 1atm = 1013 hPa
float sensor_2_pressure_reading = (raw_sensor_2_reading * 101300 / mean_val2 );    // value set for 1atm = 1013 hPa

String serial_out = "";

serial_out = "S1 raw = " + String(raw_sensor_1_reading) + " , " + "S1 pressure = " + String(sensor_1_pressure_reading) + " Pa, " + "S2 raw = " + String(raw_sensor_2_reading) + " , " + "S2 pressure = " + String(sensor_2_pressure_reading) + " Pa ";

Serial.println(serial_out);

delay(50);


// get MQTT susciptions (not used here)
/*
  Adafruit_MQTT_Subscribe *subscription;
  while ((subscription = mqtt.readSubscription(250))) {
    
    if (subscription == &cmnd_target_temperature) {
      Serial.print(F("Got: "));
      Serial.println((char *)cmnd_target_temperature.lastread);
      int temp_temperature_target = atof((char *)cmnd_target_temperature.lastread);
      if (temp_temperature_target >= 0 && temp_temperature_target < 40) {
      temperature_target = temp_temperature_target;
      Serial.print("Received New temperature Target : ");
      Serial.println(temperature_target);
      } else {
      Serial.print("Received UNVALID New temperature Target : ");
      Serial.println(temp_temperature_target);
      }
    }
    
    if (subscription == &cmnd_status) {
      Serial.print(F("Got: "));
      Serial.println((char *)cmnd_status.lastread);
      int temp_onoff_status = atof((char *)cmnd_status.lastread);
      if (temp_onoff_status == 0 || temp_onoff_status == 1) {
      onoff_status = temp_onoff_status;
      Serial.print("Received New onoff status : ");
      Serial.println(onoff_status);
      } else {
      Serial.print("Received UNVALID New onoff status : ");
      Serial.println(temp_onoff_status);
      }
      
    }

 }
 */  
  

  // Now we can publish stuff to MQTT!
  
  Serial.print(F("\nSending stat_raw_sensor_1_reading val "));
  Serial.print(raw_sensor_1_reading);
  Serial.print("...");
  if (! stat_raw_sensor_1_reading.publish(raw_sensor_1_reading)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  Serial.print(F("\nSending stat_raw_sensor_2_reading val "));
  Serial.print(raw_sensor_2_reading);
  Serial.print("...");
  if (! stat_raw_sensor_2_reading.publish(raw_sensor_2_reading)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }
  
  Serial.print(F("\nSending stat_sensor_1_pressure_reading val "));
  Serial.print(sensor_1_pressure_reading);
  Serial.print("...");
  if (! stat_sensor_1_pressure_reading.publish(sensor_1_pressure_reading)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  Serial.print(F("\nSending stat_sensor_2_pressure_reading val "));
  Serial.print(sensor_2_pressure_reading);
  Serial.print("...");
  if (! stat_sensor_2_pressure_reading.publish(sensor_2_pressure_reading)) {
    Serial.println(F("Failed"));
  } else {
    Serial.println(F("OK!"));
  }

  // Done publishing stuff to MQTT!


  // ping the server to keep the mqtt connection alive
  // NOT required if you are publishing once every KEEPALIVE seconds
  /*
  if(! mqtt.ping()) {
    mqtt.disconnect();
  }
  */

  delay(100); // we wait 100ms before looping back


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
       delay(250);  // wait 5 seconds
       retries--;
       if (retries == 0) {
         // basically die and wait for WDT to reset me
         while (1);
       }
  }
  Serial.println("MQTT Connected!");
}

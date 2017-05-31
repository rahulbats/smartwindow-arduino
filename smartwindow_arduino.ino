/*
 
 Arduino smart window
 
*/

#define LEDBLUE 13

#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <EEPROM.h>
#include "credentials.h"


//#define ssid          "wifi ssid"
//#define password      "wifi password"
//#define SERVER          "mqtt server"
//#define SERVERPORT      mqtt port
//#define MQTT_USERNAME   "mqtt username"
//#define MQTT_KEY        "mqtt password"
//#define USERNAME          "adafruit username"
//#define PREAMBLE          "feeds/"
//#define T_WINDOW_STATUS   "status key"
//#define T_WINDOW_COMMAND   "command id"



const char* weatherHost = "api.openweathermap.org";
const int weatherPort = 80;
unsigned long entry;
byte clientStatus, prevClientStatus = 99;
int inTemp=-100;
int outTemp=-100;
String prevString="";
//0-closed 1-opened 2-transitioning
int status = 0;
int smart = 0;
int desired = 0;

WiFiClient weatherClient;
WiFiClient WiFiClient;
// create MQTT object
PubSubClient client(WiFiClient);



DHT dht(D3, DHT11);

//
void setup() {

  //pinMode(D2, OUTPUT);
  //pinMode(D1, OUTPUT);
  Serial.begin(115200);
  delay(100);
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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  WiFi.printDiag(Serial);

  client.setServer(SERVER, SERVERPORT);
  client.setCallback(callback);
}

void getOutdoorTemp() {
  
      if (!weatherClient.connect(weatherHost, weatherPort)) {
        Serial.println("connection failed");
        return;
      }
     String url = "/data/2.5/weather?lat=32.7&lon=-97.3&units=metric&APPID=f2006d351c19c0b9e55e530ddaf1a909";
     Serial.print("Requesting URL: ");
     Serial.println(url);

     // This will send the request to the server
      weatherClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                   "Host: " + weatherHost + "\r\n" + 
                   "Connection: close\r\n\r\n");
      delay(10);
      // Read all the lines of the reply from server and print them to Serial
      //Serial.println("Respond:");
      while(weatherClient.available()){
        String line = weatherClient.readStringUntil('\r');
        int tempPos = line.indexOf("temp");
        if(tempPos>=0) {
          String tempLine =line.substring(tempPos+6);
          //Serial.println("tempLine:"+tempLine);
          String temperature = tempLine.substring(0,tempLine.indexOf(","));
          Serial.println("outside temperature:"+temperature);
          if(abs(temperature.toInt())<50)
            outTemp = temperature.toInt();
        }
        //Serial.print(line);
      }

}
//
void loop() {
  //Serial.println("loop");
  yield();
  if (!client.connected()) {
    Serial.println("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("rahulbats-bedroom", MQTT_USERNAME, MQTT_KEY)) {
      Serial.println("connected");
      Serial.print("subscribing to ");
      Serial.println(USERNAME PREAMBLE T_WINDOW_COMMAND);
      // ... and resubscribe
      client.subscribe(USERNAME PREAMBLE T_WINDOW_COMMAND, 1);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }

  if (millis() - entry > 60000) {
    Serial.println("Measure");
    entry = millis();
    int dhtReading = (int)dht.readTemperature();
    Serial.print("dht 11 reading");
    Serial.println(dhtReading);
    if(abs(dhtReading)<=50)
      inTemp = dhtReading;
    getOutdoorTemp();
    String valueStr;
    Serial.println(abs(inTemp));
    Serial.println(abs(outTemp));
    Serial.println(abs(desired));
    Serial.println(status);
    Serial.println(smart);
    
    if (client.connected() 
        && !isnan(inTemp) 
        && abs(inTemp)<=50 
        && abs(outTemp)<=50 
        && !isnan(outTemp)
        && abs(desired)<=50 
        && !isnan(desired)
        && abs(status)<=2
        && abs(smart)<=1
        ) {
     
      String totalStr = getTempString();
      //if(!totalStr.equals(prevString)) {
        Serial.println("Publishing total string ");
        Serial.println(totalStr);
        char valueStr[totalStr.length() + 1];
        totalStr.toCharArray(valueStr, sizeof(valueStr));
        client.publish(USERNAME PREAMBLE T_WINDOW_STATUS, valueStr);
        prevString=totalStr;
      //}
      
    }

  }

 
  client.loop();
}

String getTempString() {
  return ((String)inTemp)+'_'+((String)outTemp)+'_'+((String)desired)+'_'+((String)status)+'_'+((String)smart);
}

void openWindow() {
          Serial.println("Opening window");
          //switch status to transitioning
          status = 2;
          EEPROM.put(0,status);
          String statusStr = getTempString();
          char valueStr[statusStr.length() + 1];
          statusStr.toCharArray(valueStr, sizeof(valueStr));
          client.publish(USERNAME PREAMBLE T_WINDOW_STATUS, valueStr);  
          pinMode(D2, OUTPUT);
          pinMode(D1, OUTPUT);
          digitalWrite(D2, HIGH);
          digitalWrite(D1, LOW);
          delay(20000);
          stop();   
          status = 1;
          EEPROM.put(0,status);
          statusStr = getTempString();
          valueStr[statusStr.length() + 1];
          statusStr.toCharArray(valueStr, sizeof(valueStr));
          client.publish(USERNAME PREAMBLE T_WINDOW_STATUS, valueStr);   
          Serial.println("Window opened");       
}

void stop() {
          digitalWrite(D2, HIGH);
          digitalWrite(D1, HIGH);
}

void closeWindow() {
   
    Serial.println("Closing window");
    //switch status to transitioning
    status = 2;
    EEPROM.put(0,status);
    String statusStr = getTempString();
    char valueStr[statusStr.length() + 1];
    statusStr.toCharArray(valueStr, sizeof(valueStr));
    client.publish(USERNAME PREAMBLE T_WINDOW_STATUS, valueStr);
    pinMode(D2, OUTPUT);
    pinMode(D1, OUTPUT);
    digitalWrite(D2, LOW);
    digitalWrite(D1, HIGH);
    delay(22000);
    stop();
    status = 0;
    EEPROM.put(0,status);
    statusStr = getTempString();
    valueStr[statusStr.length() + 1];
    statusStr.toCharArray(valueStr, sizeof(valueStr));
    Serial.print("publishing after close ");
    Serial.println(valueStr);
    client.publish(USERNAME PREAMBLE T_WINDOW_STATUS, valueStr);  
    Serial.println("window closed");
}

//----------------------------------------------------------------------
void callback(char* topic, byte * data, unsigned int length) {
  // handle message arrived {
  String result="";
  Serial.print("inside callback with topic:");
  Serial.println(topic);
  Serial.println(length);
  
  if(data[0]=='O' && status==0) {
    openWindow();    
  } else if(data[0]=='C' && status==1) {
    closeWindow();
  } else if(data[0]=='S') {
    smart = 1;
  } else if(data[0]=='U') {
    smart = 0;
  } else {
    char dataArray[length];
    strncpy(dataArray, (char *)data, length);
    String dataString = String(dataArray);
    dataString.trim();
    Serial.print(dataString);
    desired = dataString.toInt();
  }
  
 
}


#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Adafruit_Sensor.h>
#include <Stepper.h>
#include "time.h"
#include "DHT.h"


#define STEPS 32
#define DHTPIN 13
#define DHTTYPE DHT11
#define uS_TO_S_FACTOR 1000000 
//30min sleep
#define TIME_TO_SLEEP  1200
#define OPEN_WINDOW 2038
#define CLOSE_WINDOW -2038 
DHT dht(DHTPIN, DHTTYPE);

Stepper stepper(STEPS, 26, 33, 25, 32);
int val = 0;

const char* ssid     = "ssid";     // your network SSID (name of wifi network)
const char* password = "password"; // your network password

const char*  server = "iotgroup5.ddenn.is";  // Server URL
const char*  url = "/push";
const char* ntpServer = "pool.ntp.org";

float average_rain = 0;
float average_temp = 0;


const char* test_root_ca = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDSjCCAjKgAwIBAgIQRK+wgNajJ7qJMDmGLvhAazANBgkqhkiG9w0BAQUFADA/\n" \
"MSQwIgYDVQQKExtEaWdpdGFsIFNpZ25hdHVyZSBUcnVzdCBDby4xFzAVBgNVBAMT\n" \
"DkRTVCBSb290IENBIFgzMB4XDTAwMDkzMDIxMTIxOVoXDTIxMDkzMDE0MDExNVow\n" \
"PzEkMCIGA1UEChMbRGlnaXRhbCBTaWduYXR1cmUgVHJ1c3QgQ28uMRcwFQYDVQQD\n" \
"Ew5EU1QgUm9vdCBDQSBYMzCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEB\n" \
"AN+v6ZdQCINXtMxiZfaQguzH0yxrMMpb7NnDfcdAwRgUi+DoM3ZJKuM/IUmTrE4O\n" \
"rz5Iy2Xu/NMhD2XSKtkyj4zl93ewEnu1lcCJo6m67XMuegwGMoOifooUMM0RoOEq\n" \
"OLl5CjH9UL2AZd+3UWODyOKIYepLYYHsUmu5ouJLGiifSKOeDNoJjj4XLh7dIN9b\n" \
"xiqKqy69cK3FCxolkHRyxXtqqzTWMIn/5WgTe1QLyNau7Fqckh49ZLOMxt+/yUFw\n" \
"7BZy1SbsOFU5Q9D8/RhcQPGX69Wam40dutolucbY38EVAjqr2m7xPi71XAicPNaD\n" \
"aeQQmxkqtilX4+U9m5/wAl0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNV\n" \
"HQ8BAf8EBAMCAQYwHQYDVR0OBBYEFMSnsaR7LHH62+FLkHX/xBVghYkQMA0GCSqG\n" \
"SIb3DQEBBQUAA4IBAQCjGiybFwBcqR7uKGY3Or+Dxz9LwwmglSBd49lZRNI+DT69\n" \
"ikugdB/OEIKcdBodfpga3csTS7MgROSR6cz8faXbauX+5v3gTt23ADq1cEmv8uXr\n" \
"AvHRAosZy5Q6XkjEGB5YGV8eAlrwDPGxrancWYaLbumR9YbK+rlmM6pZW87ipxZz\n" \
"R8srzJmwN0jP41ZL9c8PDHIyh8bwRLtTcm1D9SZImlJnt1ir/md2cXjbDaJWFBM5\n" \
"JDGFoqgCWjBH4d1QB7wCCZAA62RjYJsWvIjJEubSfZGL+T0yjWW06XyxV3bqxbYo\n" \
"Ob8VZRzI9neWagqNdwvYkQsEjgfbKbYK7p2CNTUQ\n" \
"-----END CERTIFICATE-----\n";




WiFiClientSecure client;

String printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return "";
  }

  char timeStringBuff[50]; 
  strftime(timeStringBuff, sizeof(timeStringBuff), "%Y:%m:%d:%H:%M:%S", &timeinfo);
  //print like "const char*"
  //Serial.println(timeStringBuff);
  String timestring;
  timestring = timeStringBuff;
  
  return timestring;
}

void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  delay(100);
  dht.begin();
  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  configTime(0, 0, ntpServer);
  printLocalTime();
  
  client.setCACert(test_root_ca);

}

void loop() {

  delay(1000);
  float t = dht.readTemperature();
  delay(1000);
  float h = dht.readHumidity();

  String incomingMessage = "";
  if(isnan(t)){
    Serial.println("Failed temp\n");
  }
  else if(isnan(h)){
    Serial.println("Failed hum\n");
  }
  else{

    StaticJsonBuffer<300> JSONbuffer;
    JsonObject& JSONencoder = JSONbuffer.createObject();
    JSONencoder["greenhouse_id"] = "42";
    JSONencoder["temp"] = String(t);
    JSONencoder["humidity"] = String(h);
    JSONencoder["time"] = printLocalTime();


    //JSONencoder.printTo(Serial);
    //Serial.println();

    
    String body;
    JSONencoder.printTo(body);
    String postRequest = 
    "POST " + String(url) + " HTTP/1.1\r\n" + 
    "Host: " + String(server) + "\r\n" + 
    "Content-Type: application/x-www-form-urlencoded\r\n" +
    "Content-Length: " + String(body.length()) + "\r\n" +
    "\r\n" + body;
    
    Serial.println("\n\nStarting connection to server...");
    if (!client.connect(server, 443))
      Serial.println("Connection failed!");
    else {
      Serial.println("Connected to server!");
      Serial.print(postRequest);
      client.print(postRequest);
      client.stop();
    }

    if (!client.connect(server, 443))
      Serial.println("\n\nConnection failed!");
    else {
      Serial.println("\n\nConnected to server!");
      // Make a HTTPS request for weather data:
      client.println("GET https://iotgroup5.ddenn.is/get_weather?latitude=40&longitude=-63 HTTP/1.0");
      client.println("Host: iotgroup5.ddenn.is");
      client.println("Connection: close");
      client.println();

      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          Serial.println("headers received");
          break;
        }
      }
      // if there are incoming bytes available
      // from the server, read them and print them:
      while (client.available()) {
        char c = client.read();
        incomingMessage += c;
        //Serial.write(c);
      }
      //necessary as it was reading an extra stray byte
      incomingMessage[incomingMessage.length()-1] = '\0';
      Serial.print(incomingMessage);
      client.stop();
    }
  }

  //get just the temperatures
  String rainmessage(incomingMessage.substring(incomingMessage.indexOf("rain")+7));
  String temperaturemessage(incomingMessage.substring(incomingMessage.indexOf("temperature")+14));
  Serial.print(temperaturemessage);
  
  average_temp = temperaturemessage.toFloat();
  temperaturemessage = temperaturemessage.substring(temperaturemessage.indexOf(',')+1);
  average_temp += temperaturemessage.toFloat();
  temperaturemessage = temperaturemessage.substring(temperaturemessage.indexOf(',')+1);
  average_temp += temperaturemessage.toFloat();
  average_temp = average_temp/3.0;
  //convert to c
  average_temp -= 273.1;
  Serial.print(average_temp);

  average_rain = rainmessage.toFloat();
  int x =0;
  while(x<8)
  {
    x++;
    rainmessage = rainmessage.substring(rainmessage.indexOf(',')+1);
    //instance of rain is when the chance of rain is 80%
    if(rainmessage.toFloat() > 0.8)
      average_rain += 1;
      
  }
  
  if(average_temp <= 25.0)
  {
    
    if(val != CLOSE_WINDOW)
    {
      val = CLOSE_WINDOW;
      stepper.setSpeed(500);
      stepper.step(val);
    }
     
  }
  else if (average_temp >= 25.0)
  {
    //if currently less than 25 close otherwise open
    if(t <= 25.0)
    {
      if(val != CLOSE_WINDOW)
      {
        val = CLOSE_WINDOW;
        stepper.setSpeed(500);
        stepper.step(val);
      }

    }
    else
    {  
      if(val != OPEN_WINDOW)
      {      
        val = OPEN_WINDOW;
        stepper.setSpeed(500);
        stepper.step(val);    
      }

    }

  }
  else if(average_rain >= 3)
  {
    //there will be three periods of rain close the window so as to not over water. this has the lowest priority temperature has a much greater effect than watering
    if(val != CLOSE_WINDOW)
    {
      val = CLOSE_WINDOW;
      stepper.setSpeed(500);
      stepper.step(val);
    }
  }
  
  //delay(10000);
  //deep sleep
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();




  
}

#include <SoftwareSerial.h>
#include <SPI.h>
#include <SD.h>
#include "WiFly.h"
#include <Wire.h>
#include <FaBoLCD_PCF8574.h>
#include <DHT.h>

#define toprak_nem_sensor A0
#define toprak_led 12
#define fan 11
#define isitici 13
#define DHTPIN A1
#define DHTTYPE DHT11
#define anahtar 7

DHT dht(DHTPIN, DHTTYPE);
FaBoLCD_PCF8574 lcd;
File myFile;

#define SSID      "poyraz"
#define KEY       "poyraz123"
// check your access point's security mode, mine was WPA20-PSK
// if yours is different you'll need to change the AUTH constant, see the file WiFly.h for avalable security codes
#define AUTH      WIFLY_AUTH_WPA2_PSK
char server[]="192.168.43.188";
int flag = 0;
float toprak_nem;
float temp;
float hum;
float fah;
float heat_index;
float heat_indexC;
   

// Pins' connection
// Arduino       WiFly
//  2    <---->    TX
//  10    <---->    RX

SoftwareSerial wiflyUart(10, 3); // create a WiFi shield serial object
WiFly wifly(&wiflyUart); // pass the wifi siheld serial object to the WiFly class
char ip[16];

void setup()
{
    pinMode(toprak_nem_sensor, INPUT);
    pinMode(anahtar,INPUT);
  
    pinMode(fan,OUTPUT);
    digitalWrite(fan,LOW);

    pinMode(toprak_led,OUTPUT);
    digitalWrite(toprak_led,LOW);

    pinMode(isitici,OUTPUT);
    digitalWrite(isitici,LOW);

  



    Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }


 Serial.print("Initializing SD card...");

  if (!SD.begin(4)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  
    lcd.begin(16,2);
    delay(1000);
    dht.begin();
    delay(1000);
    
    wiflyUart.begin(9600); // start wifi shield uart port

    Serial.begin(9600); // start the arduino serial port
    Serial.println("--------- WIFLY Webserver --------");

    // wait for initilization of wifly
    delay(1000);

    wifly.reset(); // reset the shield
    delay(1000);
    //set WiFly params

    wifly.sendCommand("set ip local 80\r"); // set the local comm port to 80
    delay(100);

    wifly.sendCommand("set comm remote 0\r"); // do not send a default string when a connection opens
    delay(100);

    wifly.sendCommand("set comm open *OPEN*\r"); // set the string that the wifi shield will output when a connection is opened
    delay(100);

    Serial.println("Join " SSID );
    if (wifly.join(SSID, KEY, AUTH)) {
        Serial.println("OK");
    } else {
        Serial.println("Failed");
    }

    delay(5000);

    wifly.sendCommand("get ip\r");

    wiflyUart.setTimeout(500);
    if(!wiflyUart.find("IP="))
    {
        Serial.println("can not get ip");
        while(1);;
    }else
    {
        Serial.print("IP:");
    }

    char c;
    int index = 0;
    while (wifly.receive((uint8_t *)&c, 1, 300) > 0) { // print the response from the get ip command
        if(c == ':')
        {
            ip[index] = 0;
            break;
        }
        ip[index++] = c;
        Serial.print((char)c);
        
    }
    Serial.println();
    while (wifly.receive((uint8_t *)&c, 1, 300) > 0);;
    Serial.println("Web server ready");

    

    
}

void loop()
{
    temp = dht.readTemperature();
    toprak_nem = analogRead(toprak_nem_sensor);
    toprak_nem = map(toprak_nem,0,1023,100,0);
  
  hum = dht.readHumidity();
  fah = dht.readTemperature(true); //reading the temperature in Fahrenheit
  heat_index = dht.computeHeatIndex(fah, hum); //Reading the heat index in Fahrenheit
  heat_indexC = dht.convertFtoC(heat_index); //Converting the heat index in Celsius
   

  
 if(temp <10){
    if(wifly.available())       // the wifi shield has data available
    {

        if(wiflyUart.find("*OPEN*")) // see if the data available is from an open connection by looking for the *OPEN* string
        {
            Serial.println("New Browser Request!");
            delay(1000); // delay enough time for the browser to complete sending its HTTP request string

            if(wiflyUart.find("pin=")) // look for the string "pin=" in the http request, if it's there then we want to control the LED
            {
                Serial.println("LED Control");
                // the user wants to toggle the LEDs
                int pinNumber = (wiflyUart.read()-48); // get first number i.e. if the pin 13 then the 1st number is 1
                int secondNumber = (wiflyUart.read()-48);
                if(secondNumber>=0 && secondNumber<=9)
                {
                    pinNumber*=10;
                    pinNumber +=secondNumber; // get second number, i.e. if the pin number is 13 then the 2nd number is 3, then add to the first number
                }
                digitalWrite(pinNumber, !digitalRead(pinNumber)); // toggle pin
                // Build pinstate string. The Arduino replies to the browser with this string.
                String pinState = "Pin ";
                pinState+=pinNumber;
                pinState+=" is ";
                if(digitalRead(pinNumber)) // check if the pin is ON or OFF
                {
                    pinState+="ON"; // the pin is on
                }
                else
                {
                    pinState+="OFF";  // the pin is off
                }

                // build HTTP header Content-Length string.
                String contentLength="Content-Length: ";
                contentLength+=pinState.length(); // the value of the length is the lenght of the string the Arduino is replying to the browser with.
                // send HTTP header
                wiflyUart.println("HTTP/1.1 200 OK");
                wiflyUart.println("Content-Type: text/html; charset=UTF-8");
                wiflyUart.println(contentLength); // length of HTML code
                wiflyUart.println("Connection: close");
                wiflyUart.println();
                // send response
                wiflyUart.print(pinState);
            }
            else
            {
                // send HTTP header
                wiflyUart.println("HTTP/1.1 200 OK");
                wiflyUart.println("Content-Type: text/html; charset=UTF-8");
                wiflyUart.println("Content-Length: 1435"); // length of HTML code
                wiflyUart.println("Connection: close");
                wiflyUart.println();

                // send webpage's HTML code
                wiflyUart.print("<html>");
                wiflyUart.print("<head>");
                wiflyUart.print("<meta http-equiv=\"Refresh\" content=\"60;\">");
                wiflyUart.print("<title>Sera Otomasyonu  </title>");
                wiflyUart.print("</head>");
                wiflyUart.print("<body>");
                wiflyUart.print("<table border=\"1\" align=\"center\">");
                wiflyUart.print("<tr>");
                wiflyUart.print("<td align=\"center\" height=\"100\" colspan=\"3\" bgcolor=\"#0a16b0\">");
                wiflyUart.print("<h1 style=\"color:white\">SERA OTOMASYONU</h1>");
                wiflyUart.print("</td>");
                wiflyUart.print("</tr>");

                wiflyUart.print("<tr>");
                wiflyUart.print("<td height=\"300\" width=\"150\" bgcolor=\"#0a16b0\">");
                wiflyUart.print("</td>");
                wiflyUart.print("<td align=\"center\" bgcolor=\"#2a62da\" height=\"300\" width=\"550\">");
                wiflyUart.print("<h1 style=\"color:white\">GUNCEL DEGERLER</h1>");
                wiflyUart.print("<p style=\"color:white\">Sıcaklık: </p>");
                wiflyUart.print(temp);
                wiflyUart.print(" C");
                wiflyUart.print("<p style=\"color:white\">Nem: </p>");
                wiflyUart.print("% ");
                wiflyUart.print( hum);               
                wiflyUart.print("<p style=\"color:white\">Toprak Nemi: </p>");
                wiflyUart.print("% ");
                wiflyUart.print( toprak_nem);
                wiflyUart.print("</td>");
                wiflyUart.print("<td height=\"300\" width=\"150\" bgcolor=\"#0a16b0\">");
                wiflyUart.print("</td>");
                wiflyUart.print("</tr>");
                              
                // In the <button> tags, the ID attribute is the value sent to the arduino via the "pin" GET parameter
                wiflyUart.print("<tr>");
                wiflyUart.print("<td align=\"center\" height=\"200\" colspan=\"3\" bgcolor=\"#0a16b0\">");
                wiflyUart.print("<h1 style=\"color:white\">UZAKTAN KONTROL</h1>");
                wiflyUart.print("<button id=\"11\" class=\"led\" style=\"border-radius:6px;font-size:20px;width:150px;padding:12px;background-color:#7ea2ee;\">FAN</button> "); // button for pin 11
                wiflyUart.print("<button id=\"12\" class=\"led\" style=\"border-radius:6px;font-size:20px;width:150px;padding:12px;background-color:#7ea2ee;\">SULAMA</button> "); // button for pin 12
                wiflyUart.print("<button id=\"13\" class=\"led\" style=\"border-radius:6px;font-size:20px;width:150px;padding:12px;background-color:#7ea2ee;\">ISITICI</button> "); // button for pin 13
                wiflyUart.print("</td>");
                wiflyUart.print("</tr>");
                wiflyUart.print("</table>");
                wiflyUart.print("<script src=\"http://ajax.googleapis.com/ajax/libs/jquery/2.1.3/jquery.min.js\"></script>");
                wiflyUart.print("<script type=\"text/javascript\">");
                wiflyUart.print("$(document).ready(function(){");
                wiflyUart.print("$(\".led\").click(function(){");
                wiflyUart.print("var p = $(this).attr('id');"); // get id value (i.e. pin13, pin12, or pin11)
                // send HTTP GET request to the IP address with the parameter "pin" and value "p", then execute the function
                // IMPORTANT: dont' forget to replace the IP address and port with YOUR shield's IP address and port
                wiflyUart.print("$.get(\"http://");
                wiflyUart.print(ip);
                wiflyUart.print(":80/a\", {pin:p},function(data){alert(data)});");// execute get request. Upon return execute the "function" (display an alert with the "data" send back to the browser.
                wiflyUart.print("});");
                wiflyUart.print("});");
                wiflyUart.print("</script>");
                wiflyUart.print("</body>");
                wiflyUart.print("</html>");
            }
            Serial.println("Data sent to browser");
        }
    }
 }
 else{
  
   if(wifly.connect(server,80)){
            Serial.println("connected!");
            
            wiflyUart.print("GET /ethernet/data.php?");
            wiflyUart.print("temperature=");
            wiflyUart.print(temp);
            wiflyUart.print("&humidity=");
            wiflyUart.print(hum);
            wiflyUart.print("&heat_index=");
            wiflyUart.println(heat_indexC);

            Serial.print("Temperature= ");
            Serial.println(temp);
            Serial.print("Humidity= ");
            Serial.println(hum);
            Serial.print("Heat Index= ");
            Serial.println(heat_indexC);
                                    
          } 
          else {
    // If Arduino can't connect to the server (your computer or web page)
    Serial.println("--> connection failed\n");
  }
 
            

  myFile = SD.open("THData.txt", FILE_WRITE);
  if(myFile)
    {
      Serial.println("Writing to THData.txt...");
      delay(1000);
      myFile.print(temp);
      myFile.print(",");
      myFile.println(hum);
      myFile.close();
    }
  else {
    Serial.println("error opening THData.txt");
    }

 if(temp>=25){
    digitalWrite(fan,HIGH);
  }
  else{
    digitalWrite(fan,LOW);
  }
  if(temp<=9){
    digitalWrite(isitici,HIGH);
  }
  else{
    digitalWrite(isitici,LOW);
  }
  if(toprak_nem<32){
    digitalWrite(toprak_led,HIGH);
  }
  else{
    digitalWrite(toprak_led,LOW);
  }
delay(20000);
 }
  lcd.clear();
  lcd.print("Sera Otomasyonu");
  delay(3000);
  lcd.clear();

  lcd.setCursor(0,0);
  lcd.print("Sicaklik:");
  lcd.print(temp);
  lcd.print(" C");

  lcd.setCursor(0,1);
  lcd.print("Nem: %");
  lcd.print(hum);
  delay(3000);
  
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Toprak Nem:%");
  lcd.print(toprak_nem);
  delay(3000);
 
 
}

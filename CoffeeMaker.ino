/*
    CoffeeMaker: Arduino Uno Wifi REST Server for the Jura S95 CoffeeMaker
    Acts as a backend for Homebridge / Siri
    Gitub Repository: https://github.com/thomaswitt/CoffeeMaker
    Written by Thomas Witt <https://github.com/thomaswitt>

    CoffeeMaker is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    CoffeeMaker is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with CoffeeMaker. If not, see <http://www.gnu.org/licenses/>.
*/

#include <Wire.h>
#include <ArduinoWiFi.h>
#include <SoftwareSerial.h>

String uriPrefix = "/arduino/custom/";
int ledPin = 13;

/*
  Jura S95 4-pin interface (from left to right):
  X*** pin 4 (+5V) -> Empty
  *X** pin 3 (RX)  -> Arduino Pin 3
  **X* pin 2 (GND) -> Arduino GND
  ***X pin 1 (TX)  -> Arduino Pin 2
  Use CONRAD Electronic, Part #1244004 (https://bit.ly/S95Connector)
*/
SoftwareSerial myCoffeemaker(2, 3); // RX (to S95 TX), TX (to S95 RX P3)

void blinkLED(int times = 1) {
  for (int i = 1; i <= times; i++) {
    digitalWrite(ledPin, HIGH);
    delay(150);
    digitalWrite(ledPin, LOW);
    delay(150);
  }
}

void setup() {
  pinMode(ledPin, OUTPUT);
  myCoffeemaker.begin(9600);
  delay(1000);
  myCoffeemaker.listen();
  delay(1000);
  Wifi.begin();
  blinkLED(5);
}

void loop() {
  while (Wifi.available()) {
    process(Wifi);
  }
  delay(50);
}

void httpResult(String &dest, String result = "", bool success = true) {
  if (success) {
    blinkLED();
    dest += F("HTTP/1.0 200 OK\n");
  } else {
    blinkLED(3);
    dest += F("HTTP/1.0 404 Not Found\n");
  }
  dest += F("\n");
  if (success) {
    dest += result;
  } else {
    dest += F("ERROR: ");
    dest += result;
  }
  dest += "\n";
  dest += EOL;
}

void process(WifiData client) {
  String httpReturn = "";
  String request = client.readStringUntil(EOL);
  if (request.startsWith(uriPrefix)) {
    request.replace(uriPrefix, "");
    if (request == F("turn_on")) {
      toCoffeemaker(F("AN:01\r\n"));
      httpResult(httpReturn, fromCoffeemaker());
    } else if (request == F("turn_off")) {
      toCoffeemaker(F("AN:02\r\n"));
      httpResult(httpReturn, fromCoffeemaker());
    } else if (request == F("flush")) {
      toCoffeemaker(F("FA:02\r\n"));
      httpResult(httpReturn, fromCoffeemaker());
    } else if (request == F("make_coffee")) {
      toCoffeemaker(F("FA:0C\r\n"));
      httpResult(httpReturn, fromCoffeemaker());
    /*
    } else if (request.startsWith("command")) {
      // Execute ANY command via web service - use with caution!
      request.replace("command/", "");
      toCoffeemaker(request + "\r\n");
      httpResult(httpReturn, fromCoffeemaker());
    */
    } else {
      httpResult(httpReturn, "Unknown command " + request, false);
    }
  } else {
    httpResult(httpReturn, "Invalid URI " + request, false);
  }
  client.println(httpReturn);
}

/*
  S95-Commands:
  AN:01 switches coffeemaker on
  AN:02 switches coffeemaker off
  FA:02 flush
  FA:04 small cup
  FA:05 two small cups
  FA:06 large cup
  FA:07 two large cups
  FA:08 Steam portion
  FA:09 Steam
  FA:0C XXL cup

  Thanks to:
  - https://github.com/psct/sharespresso
  - https://github.com/oliverk71/Coffeemaker-Payment-System
*/

void toCoffeemaker(String outputString)
{
  for (byte a = 0; a < outputString.length(); a++) {
    byte d0 = 255;
    byte d1 = 255;
    byte d2 = 255;
    byte d3 = 255;
    bitWrite(d0, 2, bitRead(outputString.charAt(a), 0));
    bitWrite(d0, 5, bitRead(outputString.charAt(a), 1));
    bitWrite(d1, 2, bitRead(outputString.charAt(a), 2));
    bitWrite(d1, 5, bitRead(outputString.charAt(a), 3));
    bitWrite(d2, 2, bitRead(outputString.charAt(a), 4));
    bitWrite(d2, 5, bitRead(outputString.charAt(a), 5));
    bitWrite(d3, 2, bitRead(outputString.charAt(a), 6));
    bitWrite(d3, 5, bitRead(outputString.charAt(a), 7));
    myCoffeemaker.write(d0);
    delay(1);
    myCoffeemaker.write(d1);
    delay(1);
    myCoffeemaker.write(d2);
    delay(1);
    myCoffeemaker.write(d3);
    delay(7);
  }
}

String fromCoffeemaker() {
  delay(50);
  String inputString = "";
  byte d0, d1, d2, d3;
  char d4 = 255;
  while (myCoffeemaker.available() > 0) {
    d0 = myCoffeemaker.read();
    delay (1);
    d1 = myCoffeemaker.read();
    delay (1);
    d2 = myCoffeemaker.read();
    delay (1);
    d3 = myCoffeemaker.read();
    delay (7);
    bitWrite(d4, 0, bitRead(d0, 2));
    bitWrite(d4, 1, bitRead(d0, 5));
    bitWrite(d4, 2, bitRead(d1, 2));
    bitWrite(d4, 3, bitRead(d1, 5));
    bitWrite(d4, 4, bitRead(d2, 2));
    bitWrite(d4, 5, bitRead(d2, 5));
    bitWrite(d4, 6, bitRead(d3, 2));
    bitWrite(d4, 7, bitRead(d3, 5));
    inputString += d4;
  }
  inputString.trim();
  if (inputString != "") {
    return (inputString);
  }
}


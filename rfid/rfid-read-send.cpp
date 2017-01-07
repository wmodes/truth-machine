#include <SoftwareSerial.h>
SoftwareSerial RFID(2, 3); // RX and TX

int digit;
int led = 13;
int rfidLen = 41;
int rfidByteCount = 14;
int rfidCount = 3;

void setup()
{
    RFID.begin(9600);    // start serial to RFID reader
    Serial.begin(9600);  // start serial to PC 
    pinMode(led, OUTPUT);
}

void loop()
{
  // create a string array to put our full RFID
  char fullId[rfidLen];
  fullId[0] = '\0';
  // do we have a digit waiting?
  if (RFID.available() > 0) 
  {
    // Yes! Great, let's get 14 of them
    for(int i = 0; i < rfidByteCount; i++) {
      // read new digit
      int digit = RFID.read();
      // convert digit to 2 digit string with colon
      char numStr[2]; 
      sprintf(numStr, "%02d", digit);
      // append 2 digit string to fullId
      fullId[i*3] = numStr[0];
      fullId[(i*3)+1] = numStr[1];
      if (i < rfidByteCount) {
          fullId[(i*3)+2] = ':';
      }
      // check to make sure we have another digit waiting
      // if not, bounce!
      if (RFID.available() == 0) {
        break;
      }
    }
    // We send three times to guard against serial data loss
    for(int i = 0; i < rfidCount; i++) {
      Serial.println(fullId);
    }
    digitalWrite(led, HIGH);
    delay(250);
    digitalWrite(led, LOW);
    delay(250);
  }
}

#include <RCSwitch.h>
#include <SPI.h>
#include <SD.h>

File myFile;
RCSwitch mySwitch = RCSwitch();
volatile unsigned long priv_decimal = 0;
String incomingString;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  // Serial.println();
  Serial.println("booting");

  Serial.print("Initializing SD card...");
  if (!SD.begin(D8)) {
    Serial.println("initialization failed!");
    return;
  }
  myFile = SD.open("received.txt", FILE_WRITE);
  Serial.println("initialization done.");

  mySwitch.enableReceive(D2);
  mySwitch.enableTransmit(D1);

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  Serial.println("ready");
}

void loop() {
  // put your main code here, to run repeatedly:
  if (mySwitch.available()) {
    unsigned int protocol = mySwitch.getReceivedProtocol();
    unsigned long decimal = mySwitch.getReceivedValue();
    unsigned int length = mySwitch.getReceivedBitlength();
    unsigned int delay = mySwitch.getReceivedDelay();
    char* bits = dec2binWzerofill(decimal, length);
    char button = convertToLetter(decimal);
    unsigned long remote = convertToRemote(decimal);

    if (length <= 4)
      goto after_receiver;
    
    if (priv_decimal != decimal)
        priv_decimal = decimal;
    
    char output[164]; // Adjust the size as needed
    
    sprintf(
      output,
      "%lu -> %s / Protocol: %d / Remote: %lu / %dbit %s / Delay: %d / Button: %c\n", 
      millis(), (priv_decimal == decimal) ? "Repeated" : "Received", 
      protocol, remote, length, bits, delay, button
    );

    Serial.print(output);
    myFile.print(output);
    myFile.flush();

    mySwitch.resetAvailable();
  }
  after_receiver:

  if (Serial.available() > 0) {
    Serial.print("sending: ");
    
    incomingString = Serial.readString();
    
    // converting String to char array
    // TODO: use a faster method, this is too slow
    int str_len = incomingString.length() + 1; 
    char char_array[str_len];
    incomingString.toCharArray(char_array, str_len);

    Serial.print(incomingString);
    mySwitch.send(char_array);
    Serial.println(" done");
  }
}

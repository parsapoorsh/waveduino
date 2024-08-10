#include <RCSwitch.h>
#include <SPI.h>
#include <SD.h>
#include <AsyncTimer.h>

#define LED_PIN LED_BUILTIN
#define SDCARD_CS_PIN D8
#define TRANSMITER_PIN D1
#define RECEIVE_PIN D2

#define SERIAL_SPEED 115200
#define NOISE_LENGTH 4

AsyncTimer t;
File myFile;
RCSwitch mySwitch = RCSwitch();
volatile unsigned long priv_decimal = 0;
String incomingString;
bool led_state = false;
unsigned int led_interval = 1000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_SPEED);
  // Serial.println();
  Serial.println("booting");

  Serial.print("SDcard: Initializing...");
  if (!SD.begin(SDCARD_CS_PIN)) {
    Serial.println("SDcard: initialization failed!");
    led_interval = 100; // blink 10Hz to indicate a problem with SDcard
  } else {
    myFile = SD.open("received.txt", FILE_WRITE);
    Serial.println("SDcard: initialization done.");
  }
  
  mySwitch.enableReceive(RECEIVE_PIN);
  mySwitch.enableTransmit(TRANSMITER_PIN);

  pinMode(LED_PIN, OUTPUT);
  t.setInterval(
    [](){
      digitalWrite(LED_PIN, led_state);
      led_state = !led_state;
    }, 
    led_interval
  );

  Serial.println("ready");
}

void loop() {
  // put your main code here, to run repeatedly:
  t.handle();

  // 1ms sleep to avoid 100% cpu utilization, good for cpu temp and longer batter life
  delay(1);

  if (mySwitch.available()) {
    unsigned int protocol = mySwitch.getReceivedProtocol();
    unsigned long decimal = mySwitch.getReceivedValue();
    unsigned int length = mySwitch.getReceivedBitlength();
    unsigned int delay = mySwitch.getReceivedDelay();
    mySwitch.resetAvailable();
    
    char* bits = dec2binWzerofill(decimal, length);
    char button = convertToLetter(decimal);
    unsigned long remote = convertToRemote(decimal);

    // dont care about noise
    if (length <= NOISE_LENGTH)
      goto after_receiver;
    
    char output[164]; // adjust the size as needed
    
    sprintf(
      output,
      "%lu -> %s / Protocol: %d / Remote: %lu / %dbit %s / Delay: %d / Button: %c\n", 
      millis(), 
      (priv_decimal == decimal) ? "Repeated" : "Received", 
      protocol, remote, length, bits, delay, button
    );

    if (priv_decimal != decimal)
        priv_decimal = decimal;

    Serial.print(output);

    if (myFile.availableForWrite()){
      // write to SD card
      myFile.print(output);
      myFile.flush();
    }
  }
  after_receiver:

  if (Serial.available() > 0) {
    Serial.print("sending: ");
    
    incomingString = Serial.readStringUntil('\n');
    
    // converting String to char array
    int str_len = incomingString.length() + 1; 
    char char_array[str_len];
    incomingString.toCharArray(char_array, str_len);

    Serial.print(incomingString);
    mySwitch.send(char_array); // send the signal
    Serial.println(" done");
  }
}

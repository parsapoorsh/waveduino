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
#define SDCARD_MAX_RETRY 3
#define SDCARD_RETRY_DELAY 500

AsyncTimer t;
File myFile;
RCSwitch mySwitch = RCSwitch();

volatile unsigned long priv_decimal = 0;
unsigned int led_interval = 1000;
unsigned int sdcard_retry = SDCARD_MAX_RETRY;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(SERIAL_SPEED);
  Serial.println(F("booting"));

  retry_sdcard_init:
  Serial.println(F("SDcard: Initializing..."));
  if (SD.begin(SDCARD_CS_PIN)) {
    myFile = SD.open(F("received.txt"), FILE_WRITE);
    Serial.println(F("SDcard: initialization done."));
  } else {
    Serial.printf_P(PSTR("SDcard: initialization failed! (%d)\n"), SDCARD_MAX_RETRY - sdcard_retry + 1);
    if (sdcard_retry > 1){
      sdcard_retry -= 1;
      SD.end();
      delay(SDCARD_RETRY_DELAY);
      goto retry_sdcard_init;
    }
    led_interval = 100; // blink 10Hz to indicate a problem with SDcard
  }

  mySwitch.enableReceive(RECEIVE_PIN);
  mySwitch.enableTransmit(TRANSMITER_PIN);

  pinMode(LED_PIN, OUTPUT);
  t.setInterval(blinkLED, led_interval);

  Serial.println(F("ready"));
}

void loop() {
  // put your main code here, to run repeatedly:
  t.handle();

  // 1ms sleep to avoid 100% cpu utilization, good for cpu temp and longer batter life
  delay(1);

  ignore_noise:
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
      goto ignore_noise;

    char output[164]; // adjust the size as needed

    printf_P(
      output,
      PSTR("%lu -> %s / Protocol: %d / Remote: %lu / %dbit %s / Delay: %d / Button: %c\n"), 
      millis(), 
      (priv_decimal == decimal) ? PSTR("Repeated") : PSTR("Received"), 
      protocol, remote, length, bits, delay, button
    );

    if (priv_decimal != decimal)
      priv_decimal = decimal;

    if (myFile.availableForWrite()){
      // write to SD card
      myFile.print(output);
      myFile.flush();
    }

    Serial.print(output);
  }

  if (Serial.available() > 0) {
    String incoming_string = Serial.readStringUntil('\n');

    // converting String to char array
    unsigned int str_len = incoming_string.length() + 1;
    char char_array[str_len];
    incoming_string.toCharArray(char_array, str_len);

    Serial.printf_P(PSTR("sending: %s "), char_array);
    mySwitch.send(char_array); // send the signal

    if (myFile.availableForWrite()){
      // write to SD card
      myFile.printf_P(PSTR("sent %s signal\n"), char_array);
      myFile.flush();
    }

    Serial.println(F("done"));
  }
}

void blinkLED(){
  bool led_state = digitalRead(LED_PIN);
  digitalWrite(LED_PIN, !led_state);
}

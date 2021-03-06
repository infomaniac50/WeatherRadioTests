// WeatherRadio Copyright (C) 2014 Derek Chafin

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.
// #include <Streaming.h>
// #include <Flash.h>
#include <Arduino.h>

#include <Wire.h>
#include <SI4707.h>
#include "Logging.h"
#include "Message.h"
#include "Runtime.h"

#define ERROR_PIN 7
#define STATUS_PIN 8

Logging logging;
PrintEx serial = Serial;

struct prop_t {
    uint16_t address;
    uint16_t value;
};

class Control {
  Runtime runtime;

  public:
  //
  //  The End.
  //

  void errorLoop()
  {
    // Blink forever
    while (true)
    {
      delay(1000);
      blink(ERROR_PIN, 250);
      blink(ERROR_PIN, 250);
    }
  }

   void blink(int pin, int blinkDelay) {
    digitalWrite(pin, HIGH);
    delay(blinkDelay);
    digitalWrite(pin, LOW);
    delay(blinkDelay);
  }

   void toggleMute(void) {
    if (Radio.mute)
    {
      Radio.setMute(OFF);
      serial.printf("%p\n", MUTE_OFF_LABEL);
    }
    else
    {
      Radio.setMute(ON);
      serial.printf("%p\n", MUTE_ON_LABEL);
    }
  }
   void togglePower(void) {
    if (Radio.power)
    {
      Radio.off();
      serial.printf("%p\n", RADIO_OFF_LABEL);
      logging.end();
    }
    else
    {
      logging.begin();
      Radio.patch();          //  Use this one to to include the 1050 Hz patch.
      //Radio.on();           //  Use this one if not using the patch.
      logging.printRadioVersionTo(Serial);
      //
      //  All useful interrupts are enabled here.
      //
      // Disable CTSIEN as it was getting to be a pain.
      // CTSI assert -> getStatus() -> Si CMD -> CTSI assert -> forever and ever and ever ..........
      // I believe the Si4707 library already delays a proper amount of time after each command.
      // Radio.setProperty(GPO_IEN, (CTSIEN | ERRIEN | RSQIEN | SAMEIEN | ASQIEN | STCIEN));
      Radio.setProperty(GPO_IEN, (ERRIEN | RSQIEN | SAMEIEN | ASQIEN | STCIEN));
      //
      //  RSQ Interrupt Sources.
      //
      Radio.setProperty(WB_RSQ_SNR_HIGH_THRESHOLD, 0x007F);   // 127 dBuV for testing..want it high
      Radio.setProperty(WB_RSQ_SNR_LOW_THRESHOLD, 0x0001);    // 1 dBuV for testing
      Radio.setProperty(WB_RSQ_RSSI_HIGH_THRESHOLD, 0x004D);  // -30 dBm for testing
      Radio.setProperty(WB_RSQ_RSSI_LOW_THRESHOLD, 0x0007);   // -100 dBm for testing
      //Radio.setProperty(WB_RSQ_INT_SOURCE, (SNRHIEN | SNRLIEN | RSSIHIEN | RSSILIEN));
      //
      //  SAME Interrupt Sources.
      //
      Radio.setProperty(WB_SAME_INTERRUPT_SOURCE, (EOMDETIEN | HDRRDYIEN));
      //
      //  ASQ Interrupt Sources.
      //
      Radio.setProperty(WB_ASQ_INT_SOURCE, (ALERTOFIEN | ALERTONIEN));

      //
      //  Tune to the desired frequency.
      //
      Radio.scan();  //  6 digits only.

      serial.printf("%p\n", RADIO_ON_LABEL);
    }
  }

  bool tuneUp() {
    if (Radio.tuneStatus.channel >= WB_MAX_FREQUENCY)
      return false;

    Radio.tuneStatus.channel += WB_CHANNEL_SPACING;
    Radio.tune();
    return true;
  }

  bool tuneDown() {
    if (Radio.tuneStatus.channel <= WB_MIN_FREQUENCY)
      return false;

    Radio.tuneStatus.channel -= WB_CHANNEL_SPACING;
    Radio.tune();
    return true;
  }

  unsigned long int prompt() {
      char cmdBuffer[10];
      unsigned long int loopValue;

      for (int i = 0; i < 10; i++) {
        cmdBuffer[i] = '\0';
      }

      // 0x04x
      Serial.print("> ");
      Serial.setTimeout(30000);
      Serial.readBytesUntil(';', cmdBuffer, 10);
      Serial.println(cmdBuffer);

      loopValue = strtoul(cmdBuffer, 0, 16);

      return loopValue;
  }

  //
  //  Functions are performed here.
  //
  void getFunction()
  {
    char function = Serial.read();

    switch (function)
    {
      case 'h':
      case '?':
        serial.printf("%p\n", MENU);
        break;

      case 'd':
        if (tuneDown()) serial.printf("%p\n", CHANNEL_DOWN_LABEL);
        break;

      case 'u':
        if (tuneUp()) serial.printf("%p\n", CHANNEL_UP_LABEL);
        break;

      case 's':
        serial.printf("%p\n", SCANNING_LABEL);
        Radio.scan();
        break;

      case '-':
        Radio.setVolume(Radio.volume - 1);
        break;

      case '+':
        Radio.setVolume(Radio.volume + 1);
        break;

      case 'm':
        toggleMute();
        break;

      case 'o':
        togglePower();
        break;

      case 'r':
        Radio.interruptStatus.rsq = 1;
        break;

      case 'a':
        Radio.interruptStatus.asq = 1;
        break;

      case 'I':
        Radio.getIntStatus();
      case 'i':
        serial.printf("available: %d\n", Radio.available); // Global interrupt status
        serial.printf("clearToSend: %d\n", Radio.interruptStatus.clearToSend); // D7
        serial.printf("error: %d\n", Radio.interruptStatus.error); // D6
        // Reserved D5
        // Reserved D4
        serial.printf("rsq: %d\n", Radio.interruptStatus.rsq); // D3
        serial.printf("same: %d\n", Radio.interruptStatus.same); // D2
        serial.printf("asq: %d\n", Radio.interruptStatus.asq); // D1
        serial.printf("tuneComplete: %d\n", Radio.interruptStatus.tuneComplete); // D0
        break;

      case 'c':
        runtime.processInterrupts();
        break;

      case 'P':
      case 'p':
        prop_t property;

        property.address = prompt();

        Serial.print("Address: 0x");
        Serial.print(property.address, HEX);
        Serial.write(" 0b");
        Serial.print(property.address, BIN);
        Serial.write(' ');
        Serial.println(property.address);

        if (function == 'p') {
            property.value = Radio.getProperty(property.address);
        } else {
            property.value = prompt();
            Radio.setProperty(property.address, property.value);
        }

        Serial.print("Value: 0x");
        Serial.print(property.value, HEX);
        Serial.write(" 0b");
        Serial.print(property.value, BIN);
        Serial.write(' ');
        Serial.println(property.value);
        break;

      default:
        blink(ERROR_PIN, 25);
        break;
    }

    blink(STATUS_PIN, 25);
  }

  int dump(uint8_t * var, size_t size) {
    serial.printf("Printing %d bytes\n", size);

    unsigned int i;
    for (i = 0; i < size; i++) {
      serial.printf("0x%x ", var[i]);
    }
    Serial.println();
    return i;
  }
};

Control control;

void setup()
{
  // On recent versions of Arduino the LED pin likes to turn on for no apparent reason
  // pinMode(13, OUTPUT);
  // but don't let the SD card clock be pushed or pulled by this pin
  // we are driving SCK with the Mega hardware SPI so just go high-z
  // no status pin though :(
  pinMode(13, INPUT);
  //but thats ok ;)
  pinMode(STATUS_PIN, OUTPUT);
  pinMode(ERROR_PIN, OUTPUT);

  Serial.begin(9600);

  delay(100);

  // Setup Radio
  Radio.begin(22);
}

void loop()
{
  if (Serial.available()) {
    control.blink(STATUS_PIN, 25);
    control.getFunction();
  }
}

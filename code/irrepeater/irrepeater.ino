/*
    PT-BR
    
    Este código é uma simples adaptação do exemplo ReceiveAndSend da biblioteca IRremote 
    desenvolvida por Ken Shirriff http://www.righto.com, disponível em 
    https://github.com/Arduino-IRremote/Arduino-IRremote. 
    Agradeço a ele por tornar público um trabalho tão bom! 
    
    No exemplo original era necessário pressionar um botão para enviar os códigos infravermelho, 
    então adaptei o código para que isso não fosse mais necessário e o disponibilizei em
    https://github.com/themakester/irrepeater.

    EN-US
    
    This code is an adaptation from ReceiveAndSend example packed into IRremote library 
    made by Ken Shirriff http://www.righto.com, available at
    https://github.com/Arduino-IRremote/Arduino-IRremote. 
    Thanks for share their good job Mr. Shirriff!
    
    In the original code was needed to push a button to trigger IR codes, so I changed this, 
    and now it's an automatic process. The modified code could be found at 
    https://github.com/themakester/irrepeater.


 ************************************************************************************
   MIT License

   Copyright (c) 2009-2021 Ken Shirriff, Armin Joachimsmeyer

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is furnished
   to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
   INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
   PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
   CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
   OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 ************************************************************************************
*/
#include <Arduino.h>

/*
   Define macros for input and output pin etc.
*/
#include "PinDefinitionsAndMore.h"

//#define EXCLUDE_EXOTIC_PROTOCOLS // saves around 900 bytes program space

#include <IRremote.h>

// remove int SEND_BUTTON_PIN = APPLICATION_PIN;
int STATUS_PIN = LED_BUILTIN;

int DELAY_BETWEEN_REPEAT = 50;

// On the Zero and others we switch explicitly to SerialUSB
#if defined(ARDUINO_ARCH_SAMD)
#define Serial SerialUSB
#endif

// Storage for the recorded code
struct storedIRDataStruct {
  IRData receivedIRData;
  // extensions for sendRaw
  uint8_t rawCode[RAW_BUFFER_LENGTH]; // The durations if raw
  uint8_t rawCodeLength; // The length of the code
} sStoredIRData;

// remove int lastButtonState;

void storeCode(IRData *aIRReceivedData);
void sendCode(storedIRDataStruct *aIRDataToSend);

void setup() {
  IrReceiver.start(); // themakester: moved receiver start sintax from loop to here
  Serial.begin(115200);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL)  || defined(ARDUINO_attiny3217)
  delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
  // Just to know which program is running on my Arduino
  Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));

  IrReceiver.begin(IR_RECEIVE_PIN, ENABLE_LED_FEEDBACK); // Start the receiver, enable feedback LED, take LED feedback pin from the internal boards definition

  IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin

  pinMode(STATUS_PIN, OUTPUT);

  Serial.print(F("Ready to receive IR signals at pin "));
#if defined(ARDUINO_ARCH_STM32) || defined(ESP8266)
  Serial.println(IR_RECEIVE_PIN_STRING);
#else
  Serial.println(IR_RECEIVE_PIN);
#endif

  Serial.print(F("Ready to send IR signals at pin "));
#if defined(ARDUINO_ARCH_STM32) || defined(ESP8266)
  Serial.println(IR_SEND_PIN_STRING);
#else
  Serial.print(IR_SEND_PIN);
#endif


}

void loop() {

  // themakester: this is the code that receives and then sends de IR codes with no button press need.
  if (IrReceiver.available()) {
    storeCode(IrReceiver.read());
    Serial.println(F("Code received, now sending"));
    digitalWrite(STATUS_PIN, HIGH); // flashes built-in led to show IR activity
    sendCode(&sStoredIRData); //send IR code
    digitalWrite(STATUS_PIN, LOW); // flashes built-in led to show IR activity
    delay(DELAY_BETWEEN_REPEAT); // wait a bit between retransmissions
    IrReceiver.resume(); // resume receiver
  }

}

// Stores the code for later playback in sStoredIRData
// Most of this code is just logging
void storeCode(IRData *aIRReceivedData) {
  if (aIRReceivedData->flags & IRDATA_FLAGS_IS_REPEAT) {
    Serial.println(F("Ignore repeat"));
    return;
  }
  if (aIRReceivedData->flags & IRDATA_FLAGS_IS_AUTO_REPEAT) {
    Serial.println(F("Ignore autorepeat"));
    return;
  }
  if (aIRReceivedData->flags & IRDATA_FLAGS_PARITY_FAILED) {
    Serial.println(F("Ignore parity error"));
    return;
  }
  /*
     Copy decoded data
  */
  sStoredIRData.receivedIRData = *aIRReceivedData;

  if (sStoredIRData.receivedIRData.protocol == UNKNOWN) {
    Serial.print(F("Received unknown code and store "));
    Serial.print(IrReceiver.decodedIRData.rawDataPtr->rawlen - 1);
    Serial.println(F(" timing entries as raw "));
    IrReceiver.printIRResultRawFormatted(&Serial, true); // Output the results in RAW format
    sStoredIRData.rawCodeLength = IrReceiver.decodedIRData.rawDataPtr->rawlen - 1;
    /*
       Store the current raw data in a dedicated array for later usage
    */
    IrReceiver.compensateAndStoreIRResultInArray(sStoredIRData.rawCode);
  } else {
    IrReceiver.printIRResultShort(&Serial);
    sStoredIRData.receivedIRData.flags = 0; // clear flags -esp. repeat- for later sending
    Serial.println();
  }
}

void sendCode(storedIRDataStruct *aIRDataToSend) {
  if (aIRDataToSend->receivedIRData.protocol == UNKNOWN /* i.e. raw */) {
    // Assume 38 KHz
    IrSender.sendRaw(aIRDataToSend->rawCode, aIRDataToSend->rawCodeLength, 38);

    Serial.print(F("Sent raw "));
    Serial.print(aIRDataToSend->rawCodeLength);
    Serial.println(F(" marks or spaces"));
  } else {

    /*
       Use the write function, which does the switch for different protocols
    */
    IrSender.write(&aIRDataToSend->receivedIRData, NO_REPEATS);

    Serial.print(F("Sent: "));
    printIRResultShort(&Serial, &aIRDataToSend->receivedIRData);
  }
}

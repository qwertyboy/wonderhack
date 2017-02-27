/**************************************************************************/
/*!
    @file     readMifare.pde
    @author   Adafruit Industries
	@license  BSD (see license.txt)

    This example will wait for any ISO14443A card or tag, and
    depending on the size of the UID will attempt to read from it.

    If the card has a 4-byte UID it is probably a Mifare
    Classic card, and the following steps are taken:

    - Authenticate block 4 (the first block of Sector 1) using
      the default KEYA of 0XFF 0XFF 0XFF 0XFF 0XFF 0XFF
    - If authentication succeeds, we can then read any of the
      4 blocks in that sector (though only block 4 is read here)

    If the card has a 7-byte UID it is probably a Mifare
    Ultralight card, and the 4 byte pages can be read directly.
    Page 4 is read by default since this is the first 'general-
    purpose' page on the tags.


This is an example sketch for the Adafruit PN532 NFC/RFID breakout boards
This library works with the Adafruit NFC breakout
  ----> https://www.adafruit.com/products/364

Check out the links above for our tutorials and wiring diagrams
These chips use SPI or I2C to communicate.

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

*/
/**************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// If using the breakout or shield with I2C, define just the pins connected
// to the IRQ and reset lines.  Use the values below (2, 3) for the shield!
#define PN532_IRQ   (2)
#define PN532_RESET (3)  // Not connected by default on the NFC Shield

// DAC output defines
#define ANALOG_RES 8

// specific sample rates for the different melodies
// values are microsecond delays
#define RATE_44K 22
#define RATE_22K 45
#define RATE_11K 90 // or 91
#define RATE_8K 125

// Or use this line for a breakout or shield with an I2C connection:
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// array of function pointers
typedef uint32_t (* formulaArrayType) (uint32_t t);

// sound functions! theyre numbered weird because eric
uint32_t form1(uint32_t t) {return (t&t>>6)|(t>>2)%256;}
uint32_t form2(uint32_t t) {return 2*t&((t>>4)|(t>>14))%128;}
uint32_t form5(uint32_t t) {return t * ((t>>3|t>>9)&64&t>>2);}
uint32_t form6(uint32_t t) {return 7*(t%((t>>13)&(t>>6)));}
uint32_t form7(uint32_t t) {return (t*5&t>>7)|(t*3&t>>10);}
uint32_t form8(uint32_t t) {return (t*t/256)&( t>>((t/1024)%16));}
uint32_t form9(uint32_t t) {return (t*t/128)&(t>>((t/256)%16));}
uint32_t form11(uint32_t t) {return 2*((t&2*t>>16)|(t&2*t>>8))%128;}
uint32_t form12(uint32_t t) {return 2*(t*(t>>10|t>>5)&16);}
uint32_t form13(uint32_t t) {return 2*(2*t*(t>>14|t>>12)&16);}
uint32_t form14(uint32_t t) {return 2*(t*(t>>9|t>>13)&16);}
uint32_t form15(uint32_t t) {return 2*(t*(t>>12|t>>6)&32);}
uint32_t form16(uint32_t t) {return 2*(t*(3*t>>4|2*t>>10)&32);}
uint32_t form17(uint32_t t) {return 8*(t*(t>>14|t>>10)&8);}

void setup(void){
    analogWriteResolution(ANALOG_RES);

    while(!Serial);
    Serial.begin(115200);
    Serial.println("Hello!");

    nfc.begin();

    uint32_t versiondata = nfc.getFirmwareVersion();
    if (! versiondata) {
        Serial.print("Didn't find PN53x board");
        while (1); // halt
    }
    // Got ok data, print it out!
    Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX);
    Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC);
    Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);

    // configure board to read RFID tags
    nfc.SAMConfig();
}


void loop(void) {
    // create arrays of sound functions
    formulaArrayType formArray1[] = {
        form1, form2, form5, form7,
        form11, form15, form16
    };

    formulaArrayType formArray2[] = {
        form6, form8, form9, form12,
        form13, form14, form17
    };

    while(1){
        uint8_t success;
        uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
        uint8_t uidNibbles[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

        // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
        // 'uid' will be populated with the UID, and uidLength will indicate
        // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
        Serial.println("Waiting for an ISO14443A Card ...");
        success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);

        if (success) {
            // Display some basic information about the card
            Serial.println("Found an ISO14443A card");
            Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
            Serial.print("  UID Value: ");
            nfc.PrintHex(uid, uidLength);
            Serial.println("");

            // extract the nibbles from the id number
            uint8_t nibbleIndex = 0;
            for(uint8_t i = 0; i < uidLength; i++){
                uint8_t byte = uid[i];
                uidNibbles[nibbleIndex++] = (byte & 0xF0) >> 4;
                uidNibbles[nibbleIndex++] = byte & 0x0F;
            }

            // get sums for the top and bottom half of nibbles
            // used for picking two formulas to play
            uint8_t evenNibbleSum = 0;
            uint8_t oddNibbleSum = 0;
            for(uint8_t i = 0; i < uidLength * 2; i+=2){
                evenNibbleSum += uidNibbles[i];
                oddNibbleSum += uidNibbles[i + 1];
            }

            // calculate sound array indicies
            uint8_t array1ID = evenNibbleSum % 7;
            uint8_t array2ID = oddNibbleSum % 7;

            Serial.print("Playing something using values of ");
            Serial.print(array1ID);
            Serial.print(" and ");
            Serial.println(array2ID);

            for(uint32_t t = 0; t < 30 * 11025; t++){
                uint32_t val = 0;

                // fetch our formulas
                val = formArray1[array1ID](t);
                val += formArray2[array2ID](t);

                // send to dac and wait for next cycle
                analogWrite(A14, val);
                delayMicroseconds(RATE_11K);
            }
        }
    }
}

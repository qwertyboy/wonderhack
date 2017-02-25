#include <Arduino.h>

#define ANALOG_RES 8

// specific sample rates for the different melodies
// values are microsecond delays
#define RATE_44K 22
#define RATE_22K 45
#define RATE_11K 90 // or 91
#define RATE_8K 125

uint32_t melody1(uint32_t t);

void setup(){
    analogWriteResolution(ANALOG_RES);
}

void loop(){
    uint32_t t = 0;
    for(;;t++){
        // play melody1 forever
        analogWrite(A14, melody1(t));
        delayMicroseconds(RATE_44K);
    }
}

uint32_t melody1(uint32_t t){
    return ((t*("36364689"[t>>13&7]&15))/12&128)+(((((t>>12)^(t>>12)-2)%11*t)/4|t>>13)&127);
}

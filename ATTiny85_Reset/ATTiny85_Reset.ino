#include "Arduino.h"
// AVR High-voltage Serial Programmer
// Originally created by Paul Willoughby 03/20/2010
// www.rickety.us slash 2010/03/arduino-avr-high-voltage-serial-programmer/
// Inspired by Jeff Keyzer mightyohm.com
// Serial Programming routines from ATtiny25/45/85 datasheet

// Desired fuse configuration
#define  HFUSE            0xDF   // Defaults for ATtiny25/45/85
#define  HFUSE_RSTDISBL   0x5F   // Default, but with external reset disabled
#define  LFUSE            0x62   // Defaults for ATtiny25/45/85
#define  LFUSE_NO_CKDIV8  0xE2   // Defaults, but with CKDIV8 not set (i.e. run at 8MHz instead of 1MHz)

#define  RST     13    // Output to level shifter for !RESET from transistor to Pin 1
#define  CLKOUT  12    // Connect to Serial Clock Input (SCI) Pin 2
#define  DATAIN  11    // Connect to Serial Data Output (SDO) Pin 7
#define  INSTOUT 10    // Connect to Serial Instruction Input (SII) Pin 6
#define  DATAOUT  9    // Connect to Serial Data Input (SDI) Pin 5
#define  VCC      8    // Connect to VCC Pin 8

int inData = 0;         // incoming serial byte AVR


void setup() {
  // Set up control lines for HV parallel programming
  pinMode(VCC, OUTPUT);
  pinMode(RST, OUTPUT);
  pinMode(DATAOUT, OUTPUT);
  pinMode(INSTOUT, OUTPUT);
  pinMode(CLKOUT, OUTPUT);
  pinMode(DATAIN, OUTPUT);  // configured as input when in programming mode

  // Initialize output pins as needed
  digitalWrite(RST, HIGH);  // Level shifter is inverting, this shuts off 12V

  // start serial port at 9600 bps:
  Serial.begin(9600);
}

void enterProgMode()
{
  // Initialize pins to enter programming mode
  pinMode(DATAIN, OUTPUT);  //Temporary
  digitalWrite(DATAOUT, LOW);
  digitalWrite(INSTOUT, LOW);
  digitalWrite(DATAIN, LOW);
  digitalWrite(RST, HIGH);  // Level shifter is inverting, this shuts off 12V

  // Enter High-voltage Serial programming mode
  digitalWrite(VCC, HIGH);  // Apply VCC to start programming process
  delayMicroseconds(20);
  digitalWrite(RST, LOW);   //Turn on 12v
  delayMicroseconds(10);
  pinMode(DATAIN, INPUT);   //Release DATAIN
  delayMicroseconds(300);
}

void cleanup()
{
  digitalWrite(CLKOUT, LOW);
  digitalWrite(VCC, LOW);
  digitalWrite(RST, HIGH);   //Turn off 12v  
}

void setFuses(int targetLFuse, int targetHFuse)
{
  Serial.println("Entering programming Mode");
  Serial.print("Target L and H are");
  Serial.print(targetLFuse, HEX);
  Serial.print(" and ");
  Serial.print(targetHFuse, HEX);
  Serial.println(".");
  enterProgMode();

  //Programming mode
  int lFuse = readLFuse();
  //Write lfuse if not already the value we want
  if (lFuse != targetLFuse) 
  {
    delay(1000);
    Serial.print("Writing lfuse");
    Serial.println(targetLFuse, HEX);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x40, 0x4C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, targetLFuse, 0x2C);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x64);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6C);
      
  }

  
  int hFuse = readHFuse();

  //Write hfuse if not already the value we want
  if (hFuse != targetHFuse) 
  {
    delay(1000);
    Serial.print("Writing hfuse as ");
    Serial.println(targetHFuse, HEX);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x40, 0x4C);

    // User selected option
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, targetHFuse, 0x2C);

    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x74);
    shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x7C);
  }


  // Confirm new state of play
  readFuses();

  cleanup();

  // Let user know we're done
  Serial.println("Programming complete");
}

void loop() {

  switch (establishContact()) {
    case 48+1:
      Serial.println("Choice 1");
      setFuses(LFUSE, HFUSE);
      break;
    case 48+2:
      Serial.println("Choice 2");
      setFuses(LFUSE, HFUSE_RSTDISBL);
      break;
    case 48+3:
      Serial.println("Choice 3");
      setFuses(LFUSE_NO_CKDIV8, HFUSE);
      break;
    case 48+4:
      Serial.println("Choice 4");
      setFuses(LFUSE_NO_CKDIV8, HFUSE_RSTDISBL);
      break;
    case 48+5:
      Serial.println("Choice 5");
      enterProgMode();
      readFuses();
      cleanup();
      break;
    default:
      Serial.println("Unexpected choice");
  }

  Serial.println("\nProgramming complete. Press RESET to run again.");
  while (1==1){};
}

int establishContact() {
  Serial.println("Turn on the 12 volt power/\n\nYou can ENABLE the RST pin (as RST) "
      "to allow programming\nor DISABLE it to turn it into a (weak) GPIO pin.\n");

  // We must get a 1 or 2 to proceed
  int reply;

  do {
    Serial.println("Enter 1 to ENABLE the RST pin - run at 1MHz");
    Serial.println("Enter 2 to DISABLE the RST pin - run at 1MHz");
    Serial.println("Enter 3 to ENABLE the RST pin - run at 8MHz");
    Serial.println("Enter 4 to DISABLE the RST pin - run at 8MHz");
    Serial.println("Enter 5 to read fuses");
    while (!Serial.available()) {
      // Wait for user input
    }
    reply = Serial.read();
  }
  while (reply-48 < 1 || 5 < reply-48);
  return reply;
}

int shiftOut2(uint8_t dataPin, uint8_t dataPin1, uint8_t clockPin, uint8_t bitOrder, byte val, byte val1) {
  int i;
  int inBits = 0;
  //Wait until DATAIN goes high
  while (!digitalRead(DATAIN)) {
    // Nothing to do here
  }

  //Start bit
  digitalWrite(DATAOUT, LOW);
  digitalWrite(INSTOUT, LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);

  for (i = 0; i < 8; i++) {

    if (bitOrder == LSBFIRST) {
      digitalWrite(dataPin, !!(val & (1 << i)));
      digitalWrite(dataPin1, !!(val1 & (1 << i)));
    }
    else {
      digitalWrite(dataPin, !!(val & (1 << (7 - i))));
      digitalWrite(dataPin1, !!(val1 & (1 << (7 - i))));
    }
    inBits <<= 1;
    inBits |= digitalRead(DATAIN);
    digitalWrite(clockPin, HIGH);
    digitalWrite(clockPin, LOW);

  }

  //End bits
  digitalWrite(DATAOUT, LOW);
  digitalWrite(INSTOUT, LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);
  digitalWrite(clockPin, HIGH);
  digitalWrite(clockPin, LOW);

  return inBits;
}

int readLFuse()
{
  shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x04, 0x4C);
  shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x68);
  int fuse = shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6C);
  Serial.print("lfuse reads as ");
  Serial.println(fuse, HEX);
  return fuse;
}

int readHFuse()
{
  shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x04, 0x4C);
  shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x7A);
  int fuse = shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x7E);
  Serial.print("hfuse reads as ");
  Serial.println(fuse, HEX);  
  return fuse;
}

int readEFuse()
{
  shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x04, 0x4C);
  shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6A);
  int fuse = shiftOut2(DATAOUT, INSTOUT, CLKOUT, MSBFIRST, 0x00, 0x6E);
  Serial.print("efuse reads as ");
  Serial.println(fuse, HEX);  
}

// Returns the value of the HFUSE
void readFuses() {
  Serial.println("Reading fuses");
  readLFuse();
  readHFuse();
  readEFuse();
}

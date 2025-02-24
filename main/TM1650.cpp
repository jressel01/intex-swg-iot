#include "driver/gpio.h"
#include "esp_event.h"
#include "utils.h"

#include "TM1650.h"

TM1650::TM1650(gpio_num_t dataPin, gpio_num_t clockPin, uint8_t numDigits, bool activateDisplay, uint8_t intensity,  uint8_t displaymode) {
    this->dataPin = dataPin;
    this->clockPin = clockPin;
    this->strobePin = dataPin;
    this->_maxDisplays = TM1650_MAX_POS;
    this->digits = numDigits;

    gpio_set_direction(dataPin, OUTPUT);
    gpio_set_direction(clockPin, OUTPUT);
    gpio_set_direction(strobePin, OUTPUT);

    digitalWrite(strobePin, HIGH);
    digitalWrite(clockPin, HIGH);

	//clearDisplay();

	// set the display mode, actual setting of chip is done in setupDisplay() which also sets intensity and on/off state
  _maxSegments=(displaymode==TM1650_DISPMODE_4x8 ? 8 : 7); // default TM1650_DISPMODE_4x8: display mode 4 Grid x 8 Segment

	//setupDisplay(activateDisplay, intensity);

}

#if defined(__AVR_ATtiny85__) ||  defined(__AVR_ATtiny13__) ||  defined(__AVR_ATtiny44__)
	// On slow processors we may not need this bitDelay, so save some flash
	#define bitDelay(x) 
#else
void TM1650::bitDelay()
{
	delayMicroseconds(5);
	// NOTE: on TM1637 reading keys should be slower than 250Khz (see datasheet p3)
	// for that reason the delay between reading bits should be more than 4us
	// When using a fast clock (such as ESP8266) a delay is needed to read bits correctly
	
	// TODO: the TM1650 datasheet (p.7) specifies edge times of minimal 0.1us and an average data rate of 4M bps.
	// Perhaps the bitDelay can be much lower than 5us. Button response time is 40ms with a scan period of 7ms.
	// Still need to test on ESP8266 to see which speed it can handle.
}
#endif


void TM1650::start()
{	// if needed derived classes can use different patterns to start a command (eg. for TM1637)
	// Datasheet p.3: "Start signal: keep SCL at "1" level, SDA jumps from "1" to "0", which is considered to be the start signal."
	// TM1650 expects start and stop like I2C: at start data is low, then clock changes from high to low.
  digitalWrite(dataPin, HIGH);
  digitalWrite(clockPin, HIGH);
  bitDelay();
  digitalWrite(dataPin, LOW);
  digitalWrite(clockPin, LOW);
	bitDelay();

}

void TM1650::stop()
{ // to stop TM1650 expects the clock to go high, when strobing DIO high
	// Datasheet p.3: "End signal: keep SCL at "1" level, SDA jumps from "0" to "1", which is considered to be the end signal."
	// TM1650 expects start and stop like I2C: at stop clock is high, then data changes from low to high.
  digitalWrite(clockPin, LOW);
  digitalWrite(dataPin, LOW);
	bitDelay();
  digitalWrite(clockPin, HIGH);
  digitalWrite(dataPin, HIGH);
	bitDelay();
}

void TM1650::setSegments(uint8_t segments, uint8_t position)
{	// set 8 leds on common grd as specified
	// TODO: support 10-14 segments on chips like TM1638/TM1668
	if(position<_maxDisplays)
		sendData(position, segments);
		//sendData(TM16XX_CMD_ADDRESS | position, segments);
}

void TM1650::send(uint8_t data)
{	// send a byte to the chip the way the TM1650 likes it (MSB-first)
	// For the TM1650 the bit-order is MSB-first requiring different implementation than in base class.

  for (int i = 0; i < 8; i++)
  {
    digitalWrite(clockPin, LOW);
    bitDelay();

    digitalWrite(dataPin, data & 0x80 ? HIGH : LOW);		// in contrast to other TM16xx chips, the TM1650 expects MSB first
    data <<= 1;

    digitalWrite(clockPin, HIGH);
    bitDelay();
  }
  bitDelay();
  bitDelay();

	// unlike TM1638/TM1668 and TM1640, the TM1650 and TM1637 uses an ACK to confirm reception of command/data
  // read the acknowledgement
  // TODO? return the ack?
	// (method derived from https://github.com/avishorp/TM1637 but using pins in standard output mode when writing)
  digitalWrite(clockPin, LOW);
  gpio_set_direction(dataPin, INPUT);
  bitDelay();
  digitalWrite(clockPin, HIGH);
  bitDelay();
  uint8_t ack = digitalRead(dataPin);
  if (ack == 0)
	  digitalWrite(dataPin, LOW);
  gpio_set_direction(dataPin, OUTPUT);
}

void TM1650::sendData(uint8_t address, uint8_t data)
{	// TM1650 uses different commands than other TM16XX chips
	start();
  send(TM1650_CMD_ADDRESS | address<<1);						// address command + address (68,6A,6C,6E)
  send(data);
  stop();
}

void TM1650::clearDisplay()
{	// Clear all data registers. The number of registers depends on the chip.
	// TM1638 (10x8): 10 segments per grid, stored in two bytes. The first byte contains the first 8 display segments, second byte has seg9+seg10  => 16 bytes
	// TM1640 (8x16): one byte per grid => 16 bytes
	// TM1637 (8x6): one byte per grid => 6 bytes
	// TM1650 (8x4): two bytes per grid => 8 bytes
	// TM1668 (10x7 - 14x3): two bytes per grid => 14 bytes
  for (int i = 0; i < _maxDisplays; i++) {
  	sendData(i, 0);
  }
}

void TM1650::setupDisplay(bool active, uint8_t intensity)
{	// For the TM1650 level 0 is maximum brightness, 1-7 is low to high.
	// To align with other TM16XX chips we translate this to the same levels (0-7)
	intensity=intensity;
	intensity+=1;
	if(intensity==8) intensity=0;
	start();
  send(TM1650_CMD_MODE);
  send( (intensity<<4) | (_maxSegments==7? 0x08:0x00) | (active?0x01:0x00));
  stop();
}

uint8_t TM1650::receive()
{	// For the TM1650 the bit-order is MSB-first requiring different implementation than in base class.
  uint8_t temp = 0;

  // Pull-up on
  digitalWrite(clockPin, LOW);
  gpio_set_direction(dataPin, INPUT);
  digitalWrite(dataPin, HIGH);

  for (int i = 0; i < 8; i++)
  {
    temp <<= 1;  // MSB first on TM1650, so shift left

    digitalWrite(clockPin, HIGH);
    bitDelay();		// NOTE: on TM1637 reading keys should be slower than 250Khz (see datasheet p3)

    if (digitalRead(dataPin)) {
      temp |= 0x01;	 // MSB first on TM1650, so set lowest bit
    }

    digitalWrite(clockPin, LOW);
    bitDelay();
  }

	// receive Ack
	// TODO: currently the logical analyzer reports a NAK for the received value.
	// I'd like to see an ACK instead as all data transfer seems okay.
  digitalWrite(clockPin, LOW);
  //digitalWrite(dataPin, HIGH);
  gpio_set_direction(dataPin, INPUT);
  digitalWrite(dataPin, HIGH);
  bitDelay();
  digitalWrite(clockPin, HIGH);
  bitDelay();
  uint8_t ack = digitalRead(dataPin);
  digitalWrite(clockPin, LOW);
  if (ack == 0)
	  digitalWrite(dataPin, LOW);
  gpio_set_direction(dataPin, OUTPUT);

  return temp;
}

uint8_t TM1650::getButtonPressedCode()
{ // Keyscan data on the TM1650/TM1637 is one byte, with index of the button that is pressed.
	// TM1650 supports 7x4 buttons (A-G via 2K resistors to DIG1-DIG4). Simultaneous presses are not supported.
	// Received value is below 0x0F when no buttons are pressed
	// For TM1650 button return values are:
	//		0x44-0x47 for DIG1 to DIG4 on KeyInput1 (A)
	//		0x4C-0x4F for DIG1 to DIG4 on KeyInput2 (B)
	//		0x54-0x57 for DIG1 to DIG4 on KeyInput3 (C)
	//		0x5C-0x5F for DIG1 to DIG4 on KeyInput4 (D)
	//		0x64-0x67 for DIG1 to DIG4 on KeyInput5 (E)
	//		0x6C-0x6F for DIG1 to DIG4 on KeyInput6 (F)
	//		0x74-0x77 for DIG1 to DIG4 on KeyInput7 (G)
	// Button state is reset when another command is issued
	// For compatibility with the rest of the library the buttonstate is returned as a 32-bit value
  start();
  send(TM1650_CMD_DATA_READ);		// send read buttons command
	uint8_t received=receive();
	stop();

	// Only accept values >= 0x44 as pressed keys (datasheet shows 0x44-0x77 as valid keypresses)
	// Testing shows return value is below 0x44 when no button is pressed.
	// The return value is 0x04 after button 0x44 was released or 0x0C after button 0x4C was released.
	// Testing also shows return value is 0x2E after first powerup (when no buttons were pressed). It changes to 0x0C after button 0x4C was released.
	//if(received<0x44)
	//	return(0);

  return received;
}

/*
	Demonstrates how to use a subset of the levels.

	In this example, we set the lowest volume to 90 and the highest to 190.  Then we cycle through all the levels and display the volumes and
	volume levels.  The output is expected to be similar to the example below.

	Output:
	Current volume: 90
	Current volume level: 0

	Current volume: 100
	Current volume level: 1

	...

	Current volume: 180
	Current volume level: 9

	Current volume: 190
	Current volume level: 10

*/

#include <SoftwareSerial.h>
#include "VS1000UART.h"

// Arduino pins that can be used with SoftwareSerial.
#define ARDUINO_PIN_RX_FROM_AUDIO_TX	5
#define ARDUINO_PIN_TX_TO_AUDIO_RX		6

// Connect to the RST pin on the Sound Board.
#define ARDUINO_PIN_FOR_AUDIO_RESET		4

// We'll be using software serial.
SoftwareSerial	_softwareSerial				= SoftwareSerial(ARDUINO_PIN_RX_FROM_AUDIO_TX, ARDUINO_PIN_TX_TO_AUDIO_RX);

// Pass the software serial to the audio class, the second argument is the debug port (not used really) and the third arg is the reset pin.
VS1000UART 		_vsUart 					= VS1000UART(&_softwareSerial, ARDUINO_PIN_FOR_AUDIO_RESET);

void setup()
{
	// You don't seem to be able to run the serial monitor at the same speed as the communication with the chip so make sure the two baud
	// rates are different.
	// Must call "begin" on serial stream before VS1000UART.
	Serial.begin(115200);
	_softwareSerial.begin(9600);
	_vsUart.begin();

	// Set the minimum volume used.
	_vsUart.setMinimumVolume(90);
	
	// Set the maximum volume used.
	_vsUart.setMaximumVolume(190);

	// Call begin on the VSUART1000 to run its start up.
	_vsUart.begin();

	// Restart the aduio to ensure it is in a known state.
	_vsUart.reset();
	Serial.println("Audio ready.");

	// The audio chip normally starts at the highest volume, let's start at the bottom and go up.
	_vsUart.setVolumeLevel(VS1000UART::VOLUME0);
}

void loop()
{
	// Loop through all the levels and display the values.
	for (int i = 0; i < 11; i++)
	{
		// Blank line output on serial monitor for readability.
		Serial.println();
		// Get the current volume and volume level and display them.
		Serial.print("Current volume: ");
		Serial.println(_vsUart.getVolume());
		Serial.print("Current volume level: ");
		Serial.println((uint8_t)(_vsUart.getVolumeLevel()));

		// This will increment the volume level.  If the maximum level is reached, it will restart at the lowest.
		_vsUart.cycleVolumeLevel();
	}

	Serial.println();
	Serial.println("Press enter to start the loop over.");
	while (!Serial.available())
	{
		delay(10);
	}
	while (Serial.available())
	{
		// Read all remaining data on the serial.  Use a slight delay to give the data time to transfer.
		Serial.read();
		delay(10);
	}
}
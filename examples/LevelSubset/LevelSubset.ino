/*
	Demonstrates how to use a subset of the levels.

	In this example, we set the lowest level to 1 and the highest to 5.  Then we cycle through all the levels and display the volumes and
	volume levels.  The expected output is that the lowest volume (0) is level 1 and the highest volume is level 5.  It should appear something
	like the example output below.

	Output:
	Current volume: 0
	Current volume level: 1

	Current volume: 50
	Current volume level: 2

	Current volume: 100
	Current volume level: 3

	Current volume: 150
	Current volume level: 4

	Current volume: 200
	Current volume level: 5
*/

#include <SoftwareSerial.h>
#include "VS1000UART.h"

// Choose any two pins that can be used with SoftwareSerial to RX & TX.
#define SFX_TX 5
#define SFX_RX 6

// Connect to the RST pin on the Sound Board.
#define SFX_RST 4

// We'll be using software serial.
SoftwareSerial	_softwareSerial				= SoftwareSerial(SFX_TX, SFX_RX);

// Pass the software serial to the audio class and the second argument is the reset pin number.
VS1000UART 		_vsUart 					= VS1000UART(&_softwareSerial, SFX_RST);

void setup()
{
	Serial.begin(9600);

	// Software serial at 9600 baud.  Must call "begin" on serial stream before VS1000UART.
	_softwareSerial.begin(9600);

	// Set up the levels we want to use.
	// This sets the lowest level to 1 instead of 0.
	_vsUart.useLowerLevelOne(true);

	// Set the maximum level as 5.
	_vsUart.setMaximumLevel(VS1000UART::VOLUME5);

	// Call begin on the VSUART1000 to run its start up.
	_vsUart.begin();

	// Restart the aduio to ensure it is in a known state.
	_vsUart.reset();
	Serial.println("Audio ready.");

	// The audio chip normally starts at the highest volume, let's start at the bottom and go up.
	_vsUart.setVolumeLevel(VS1000UART::VOLUME1);
}

void loop()
{
	// Loop through all the levels and display the values.
	for (int i = 0; i < 5; i++)
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
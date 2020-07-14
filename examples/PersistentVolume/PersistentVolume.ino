/* 
	Demonstrates how to make a volume setting persist after an Arduino has been restarted or powered off.

	The volume level can be stored on the Arduino's memory (EEPROM).  When the VS1000UART class is created after the restart/power off
	it reads the value from memory and restores the volume setting on the audio chip.

	Usage:
	When the sketch starts, the last volume is read from the flash memory on the Arduino and displayed.
	When prompted, pressing enter will increment the volume level and save it to the flash memory.

	To demonstrate the volume is persistent, change the volume, then restart the Arduino.  The restart can be done in one of several ways:
		- For a hard restart, remove power, then re-power the Arduino.
		- The serial monitor can be closed and re-opened.
		- When prompted, enter 'r' from the serial monitor.
*/

#include <SoftwareSerial.h>
#include "VS1000UART.h"

// Choose any two pins that can be used with SoftwareSerial to RX & TX.
#define SFX_TX 5
#define SFX_RX 6

// Connect to the RST pin on the Sound Board.
#define SFX_RST 4

// Uncomment for additional output messages.
#define DEBUGOUTPUT

// You can also monitor the ACT pin for when audio is playing.

// We'll be using software serial.
SoftwareSerial	_softwareSerial				= SoftwareSerial(SFX_TX, SFX_RX);

// Pass the software serial to the audio class, the second argument is the reset pin number, and the third number is the memory address
// used to store the volume.  If you are not using any other memory, it is recommended to use 0.
VS1000UART 		_vsUart 					= VS1000UART(&_softwareSerial, SFX_RST, 0);

// A 'function' to reset the Arduino.  It is a function pointer to the first memory address.  Causing execution to start from there is a restart.
void(*reset)() = 0;

void setup()
{
	Serial.begin(9600);

	// Software serial at 9600 baud.  Must call "begin" on serial stream before VS1000UART.
	_softwareSerial.begin(9600);
	_vsUart.begin();

	if (!_vsUart.reset())
	{
		Serial.println("VS1000 failed to reset.");

		// Something went wrong, so we freeze.
		while (1)
		{
		}
	}

	Serial.println("Audio ready.");
}

void loop()
{
	// Blank line output on serial monitor for readability.
	Serial.println();

	// Get the current volume and volume level and display them.
	Serial.print("Current volume: ");
	Serial.println(_vsUart.getVolume());
	Serial.print("Current volume level: ");
	Serial.println(_vsUart.getVolumeLevel());

	// Show a message to the user asking them what to do, then wait for a response.
	Serial.println("Press enter to increment volume level or enter 'r' to reset the Arduino.");
	while (!Serial.available())
	{
		delay(10);
	}

	// Read the response, then clear any extra data sent.
	char characterRead = Serial.read();
	while (Serial.available())
	{
		// Read all remaining data on the serial.  Use a slight delay to give the data time to transfer.
		Serial.read();
		delay(10);
	}
	if (characterRead == 'r')
	{
		reset();
	}

	// Increment volume level and display it.  If we get to the maximum level, we will roll over to the start.
	// This will also save the volume to memory.
	Serial.println();
	Serial.println("Setting new volume level...");
	_vsUart.cycleVolumeLevel();
	Serial.print("New volume: ");
	Serial.println(_vsUart.getVolume());
	Serial.print("New volume level: ");
	Serial.println(_vsUart.getVolumeLevel());
}
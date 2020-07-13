/* 
	Menu driven control of a sound board over UART.
	Commands for playing by number or by name (full 11-char name).
	Hard reset.
	List files (when not playing audio).
	Volume up and down (only when not playing audio).
	Pause, unpause, quit playing (when playing audio).
	Current play time, and bytes remaining & total bytes (when playing audio).

	For the Adafruit Sound
	Connect UG to ground to have the sound board boot into UART mode.

	This is not considered to be a good example for several reasons:
	- It tries to demonstrate everything at once.  Better examples demonstrate a single topic and only use as much code as necessary
		to show how that topic works.
	- It is unlikely that the audio class with ever be used in this manner (with a user interface).  It is more likely the class be
		called as necessary based on events or states determined by the main microprocessor.
	- The most imporant parts of the code (calls to the audio class interface) are buried inside a bunch of user interface code.

	Enhancements:
	- Added better feedback to the user.
	- Cleaned up code in some places.
	- Added comments to explain what is happening.
	- Used "atoi" function call instead of the "x-'0'" that was previously used.  The function call is more explicit, it doesn't require
		interpreting how characters as stored as integers and implicit conversions.
	- Better code reuse.

	Bug fixes:
	- Menu printed twice after entering a command.
		- Wasn't waiting for the end of line character to hit the buffer before trying to read it.  On the next loop it was seen as input.
	- Play track by name or number did not work as the original author intended.  The intent was to enter the menu command, then
		user would be prompted to enter the name or number.  This did not work.  Entering "#" or "P" would lock the interface.  Instead,
		the command and name/number had to be entered on a single line, e.g. "#7".
		- Likely related to error in reading line return character.
	- Fixed inconsistencies in user interface output and input.
	- Added "F()" macro to all constant strings in "println" functions to reduce amount of dynamic memory used.
*/

/*
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

// Pass the software serial to the audio class, the second argument is the debug port (not used really) and the third arg is the reset pin.
VS1000UART 		_vsUart 					= VS1000UART(&_softwareSerial, SFX_RST, 0);

// A 'function' to reset the Arduino.  It is a function pointer to the first memory address.  Causing execution to start from there is a restart.
void(* resetFunc)(void) = 0;

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

	// Get the current volume level and display it.
	VS1000UART::VOLUMELEVEL volumeLevel = _vsUart.getVolumeLevel();
	Serial.print("Current volume level: ");
	Serial.println(volumeLevel);

	Serial.println("Press enter to continue or enter 'r' to reset the Arduino.");
	while (!Serial.available())
	{
		delay(10);
	}
	char characterRead = Serial.read();
	while (Serial.available())
	{
		// Read all remaining data on the serial.  Use a slight delay to give the data time to transfer.
		Serial.read();
		delay(10);
	}
	if (characterRead == 'r')
	{
		resetFunc();
	}

	// Increment volume level and display it.
	volumeLevel = (VS1000UART::VOLUMELEVEL)(volumeLevel + 1);
	if (volumeLevel >= VS1000UART::VOLUME10)
	{
		volumeLevel = VS1000UART::VOLUME0;
	}

	// Set the new volume level.  This will also save it to memory.
	Serial.println("Setting new volume level...");
	Serial.print("Level: ");
	Serial.println(volumeLevel);
	_vsUart.setVolume(volumeLevel);
	Serial.print("New volume level: ");
	Serial.println(_vsUart.getVolumeLevel());
}
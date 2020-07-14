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
VS1000UART 		_vsUart 					= VS1000UART(&_softwareSerial, SFX_RST);

// Variable to store text sent by user.
#define BUFFERSIZE 20
char 			_lineBuffer[BUFFERSIZE];

void setup()
{
	Serial.begin(9600);

	// Software serial at 9600 baud.  Must call "begin" on serial stream before VS1000UART.
	_softwareSerial.begin(9600);
	_vsUart.begin();

	// if (!_vsUart.reset())
	// {
	// 	Serial.println("VS1000 failed to reset.");

	// 	// Something went wrong, so we freeze.
	// 	while (1)
	// 	{
	// 	}
	// }

	Serial.println("Audio ready.");
}

void loop()
{
	// Print the options as a list and read the input.
	char command = getCommand();

	// Now run the command.
	runCommand(command);
}

char getCommand()
{
	Serial.println();
	Serial.println(F("What would you like to do?"));
	Serial.println(F("[R] - Reset"));
	Serial.println(F("[+] - Volume up"));
	Serial.println(F("[-] - Volume down"));
	Serial.println(F("[V] - Set volume level"));
	Serial.println(F("[L] - List files"));
	Serial.println(F("[P] - Play by file name"));
	Serial.println(F("[#] - Play by file number"));
	Serial.println(F("[=] - Pause playing"));
	Serial.println(F("[>] - Resume playing"));
	Serial.println(F("[Q] - Stop playing"));
	Serial.println(F("[T] - Play time status"));
	Serial.println(F("> "));

	// Read the response.  We will only use the first letter, read the line to clear everything sent (like the line return).
	readLine();

	return _lineBuffer[0];
}

void runCommand(char command)
{
	switch (command)
	{
		case 'R':
		{
			Serial.println(F(""));
			Serial.println(F("Restarting..."));
			if (_vsUart.reset())
			{
				Serial.println(F("Audio restarted."));
			}
			else
			{
				Serial.println(F("Reset failed."));
			}
			break;
		}

		case 'L':
		{
			uint8_t numberOfFiles = _vsUart.listFiles();

			if (numberOfFiles > 0)
			{
				Serial.println();
				Serial.println(F("File Listing"));
				Serial.println(F("========================"));
				Serial.print(F("Found "));
				Serial.print(numberOfFiles);
				Serial.println(F(" files"));
				for (uint8_t i = 0; i < numberOfFiles; i++)
				{
					Serial.print(i);
					Serial.print(F("\tname: "));
					Serial.print(_vsUart.fileName(i));
					Serial.print(F("\tsize: "));
					Serial.println(_vsUart.fileSize(i));
				}
				Serial.println(F("========================"));
			}
			else
			{
				Serial.println(F("No files found."));
				Serial.println(F("  - Stop play before listing files."));
				Serial.println(F("  - Ensure files have been loaded."));
			}
			
			break;
		}

		case '#':
		{
			Serial.println(F("Enter track #:"));
			Serial.println(F("> "));

			uint8_t trackNumber = readNumber();

			Serial.print(F("Playing track #"));
			Serial.println(trackNumber);
			if (!_vsUart.playTrack((uint8_t)trackNumber))
			{
				Serial.println(F("Failed to play track."));
			}
			break;
		}

		case 'P':
		{
			Serial.println(F("Enter track name (full 12 character name):"));
			Serial.println(F("> "));
			readLine();

			Serial.print(F("Playing track \""));
			Serial.print(_lineBuffer);
			Serial.println("\"");
			if (!_vsUart.playTrack(_lineBuffer))
			{
				Serial.println(F("Failed to play track."));
			}
			break;
		}

		case '+':
		{
			Serial.println(F("Volume up."));
			uint16_t volumeReturned = _vsUart.volumeUp();
			reportVolumeResult(volumeReturned);
			break;
		}

		case '-':
		{
			Serial.println(F("Volume down."));
			uint16_t volumeReturned = _vsUart.volumeDown();
			reportVolumeResult(volumeReturned);
			break;
		}

		case 'V':
		{
			Serial.println(F("Enter volume level (0-10)."));
			Serial.println(F("> "));
			uint8_t volumeLevel = readNumber();

			// Make level is valid.  Must be between 0 and 10.  Variable is unsigned so it cannot be negative.
			if (volumeLevel > 10)
			{
				Serial.println(F("Invalid entry."));
			}
			else
			{
				uint8_t newVolume = _vsUart.setVolumeLevel((VS1000UART::VOLUMELEVEL)volumeLevel);
				if (newVolume)
				{
					Serial.print(F("Volume level: "));
					Serial.println(volumeLevel);
					Serial.print(F("Volume setting: "));
					Serial.println(newVolume);
				}
				else
				{
					Serial.println(F("Failed to set volume."));
				}
			}
			
			break;
		}

		case '=':
		{
			if (_vsUart.pausePlay())
			{
				Serial.println(F("Paused."));
			}
			else
			{
				Serial.println(F("Failed to pause."));
			}
			break;
		}

		case '>':
		{
			if (_vsUart.resumePlay())
			{
				Serial.println(F("Play resumed."));
			}
			else
			{
				Serial.println(F("Failed to resume."));
			}
			break;
		}

		case 'Q':
		{
			if (_vsUart.stopPlay())
			{
				Serial.println(F("Play stopped."));
			}
			else
			{
				Serial.println(F("Failed to stop play."));
			}
			break;
		}

		case 'T':
		{
			uint32_t current	= 0;
			uint32_t total		= 0;
			if (_vsUart.trackTime(&current, &total))
			{
				Serial.print(F("Track time: "));
				Serial.print(current);
				Serial.println(F(" seconds"));
			}
			else
			{
				Serial.println(F("Track time query failed."));
			}
			break;
		}

		case 'S':
		{
			Serial.print(F("Track size (bytes remaining/total): "));
			uint32_t remain = 0;
			uint32_t total = 0;
			if (!_vsUart.trackSize(&remain, &total))
			{
				Serial.println(F("Failed to query"));
			}
			Serial.print(remain);
			Serial.print("/");
			Serial.println(total);
			break;
		}

		default:
		{
			Serial.println(F("Invalid entry, try again."));
			break;
		}
	}
}

char readBlocking()
{
	// Waits until the serial says there is something in the buffer to read.
	while (!Serial.available())
	{
	}
	
	// Read one character and return it.
	return Serial.read();
}

uint16_t readNumber()
{
	// This is a two step function.  First read until we get a digit, then read so long as there are digits entered.
	
	// Read until we get a digit.
	char characterRead = readBlocking();
	while (!isdigit(characterRead))
	{
		characterRead = readBlocking();
	}

	// Convert the character to a number.
	uint16_t x = atoi(&characterRead);

	// Keep reading so long as there is a number.
	characterRead = readBlocking();
	while (isdigit(characterRead))
	{
		// Another digit was read, so move the decimal point and add in the new digit.
		x				= x * 10 + atoi(&characterRead);
		characterRead	= readBlocking();
	}

	return x;
}

uint8_t readLine()
{
	// Index into the buffer.
	uint16_t index = 0;

	while (true)
	{
		// Ensure we don't over run the size of the buffer.
		if (index > BUFFERSIZE)
		{
			// If we fill up the buffer, break the loop and allow the function to exit.
			break;
		}

		if (Serial.available())
		{
			char characterRead = Serial.read();

			// Ignores a carraige return character.  This is not a newline character.
			if (characterRead == '\r')
			{
				continue;
			}

			if (characterRead == 0xA)
			{
				if (index == 0)
				{
					// The first 0x0A is ignored.
					// Why is this?
					continue;
				}

				// Done so exit loop.
				break;
			}

			// Insert character at the end and advance the index.
			_lineBuffer[index] = characterRead;
			index++;
		}
	}

	// Null term at the end of the string.  The index was already advanced in loop.
	_lineBuffer[index] = 0;
	return index;
}

void reportVolumeResult(uint8_t volumeReturned)
{
	// This function reports the result of a volume change so it is not duplicated for an up change and a down
	// change.  This also helps shorten main user interface switch statement and make it more readable.
	if (volumeReturned == 0)
	{
		Serial.println(F("Failed to change volume."));
	}
	else
	{
		Serial.print(F("Volume: "));
		Serial.println(volumeReturned);
	}
}
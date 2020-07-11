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
SoftwareSerial softwareSerial = SoftwareSerial(SFX_TX, SFX_RX);

// Pass the software serial to Adafruit_soundboard, the second argument is the debug port
// (not used really) and the third arg is the reset pin.
VS1000UART vsUart = VS1000UART(&softwareSerial, NULL, SFX_RST);

void setup()
{
	Serial.begin(115200);

	// Software serial at 9600 baud.
	softwareSerial.begin(9600);

	if (!vsUart.reset())
	{
		Serial.println("VS1000 failed to reset.");

		// Something went wrong, so we freeze.
		while (1)
		{
		}
	}

	Serial.println("SFX board found");
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
	flushInput();

	Serial.println("");
	Serial.println("What would you like to do?");
	Serial.println("[R] - Reset");
	Serial.println("[+] - Volume up");
	Serial.println("[-] - Volume down");
	Serial.println("[L] - List files");
	Serial.println("[P] - Play by file name");
	Serial.println("[#] - Play by file number");
	Serial.println("[=] - Pause playing");
	Serial.println("[>] - Unpause playing");
	Serial.println("[Q] - Stop playing");
	Serial.println("[T] - Play time status");
	Serial.println("> ");

	// Get one character.
	char command = readBlocking();

	// Eat the return character.
	readBlocking();

	#ifdef DEBUGOUTPUT
		Serial.print("Input read: ");
		if (command == '\n')
		{
			Serial.println("Line return");
		}
		else
		{
			Serial.println(command);
		}
	#endif

	return command;
}

void runCommand(char command)
{
	switch (command)
	{
		case 'R':
		{
			Serial.println("");
			Serial.println("Restarting...");
			if (vsUart.reset())
			{
				Serial.println("Audio restarted.");
			}
			else
			{
				Serial.println("Reset failed.");
			}
			break;
		}

		case 'L':
		{
			uint8_t files = vsUart.listFiles();

			Serial.println();
			Serial.println("File Listing");
			Serial.println("========================");
			Serial.print("Found ");
			Serial.print(files);
			Serial.println(" files");
			for (uint8_t f = 0; f < files; f++)
			{
				Serial.print(f);
				Serial.print("\tname: ");
				Serial.print(vsUart.fileName(f));
				Serial.print("\tsize: ");
				Serial.println(vsUart.fileSize(f));
			}
			Serial.println("========================");
			break;
		}

		case '#':
		{
			Serial.print("Enter track #");
			uint8_t n = readNumber();

			Serial.print("\nPlaying track #");
			Serial.println(n);
			if (!vsUart.playTrack((uint8_t)n))
			{
				Serial.println("Failed to play track.");
			}
			break;
		}

		case 'P':
		{
			Serial.print("Enter track name (full 12 character name) >");
			char name[20];
			readLine(name, 20);

			Serial.print("\nPlaying track \"");
			Serial.print(name);
			Serial.print("\"");
			if (!vsUart.playTrack(name))
			{
				Serial.println("Failed to play track.");
			}
			break;
		}

		case '+':
		{
			Serial.println("Volume up...");
			uint16_t volumeReturned = vsUart.volumeUp();
			reportVolumeResult(volumeReturned);
			break;
		}

		case '-':
		{
			Serial.println("Volume down...");
			uint16_t volumeReturned = vsUart.volumeDown();
			reportVolumeResult(volumeReturned);
			break;
		}

		case '=':
		{
			Serial.println("Pausing...");
			if (!vsUart.pause())
			{
				Serial.println("Failed to pause");
			}
			break;
		}

		case '>':
		{
			Serial.println("Unpausing...");
			if (!vsUart.unpause())
			{
				Serial.println("Failed to unpause");
			}
			break;
		}

		case '1':
		{
			Serial.println("Stopping...");
			if (!vsUart.stop())
			{
				Serial.println("Failed to stop");
			}
			break;
		}

		case 'T':
		{
			Serial.print("Track time: ");
			uint32_t current;
			uint32_t total;
			if (!vsUart.trackTime(&current, &total))
			{
				Serial.println("Failed to query");
			}
			Serial.print(current);
			Serial.println(" seconds");
			break;
		}

		case 'S':
		{
			Serial.print("Track size (bytes remaining/total): ");
			uint32_t remain;
			uint32_t total;
			if (!vsUart.trackSize(&remain, &total))
			{
				Serial.println("Failed to query");
			}
			Serial.print(remain);
			Serial.print("/");
			Serial.println(total);
			break;
		}

		default:
		{
			Serial.println("Invalid entry, try again.");
			break;
		}
	}
}

void flushInput()
{
	// Read all available serial input to flush pending data.
	uint16_t timeoutloop = 0;
	while (timeoutloop++ < 40)
	{
		while (softwareSerial.available())
		{
			softwareSerial.read();

			// If char was received reset the timer.
			timeoutloop = 0;
		}
		delay(1);
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
		x = x * 10 + atoi(&characterRead);
	}

	return x;
}

uint8_t readLine(char* buffer, uint8_t bufferSize)
{
	// Index into the buffer.
	uint16_t index = 0;

	while (true)
	{
		// Ensure we don't over run the size of the buffer.
		if (index > bufferSize)
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
			buffer[index] = characterRead;
			index++;
		}
	}

	// Null term at the end of the string.  The index was already advanced in loop.
	buffer[index] = 0;
	return index;
}

void reportVolumeResult(uint8_t volumeReturned)
{
	// This function reports the result of a volume change so it is not duplicated for an up change and a down
	// change.  This also helps shorten main user interface switch statement and make it more readable.
	if (volumeReturned == 0)
	{
		Serial.println("Failed to change volume.");
	}
	else
	{
		Serial.print("Volume: ");
		Serial.println(volumeReturned);
	}
}
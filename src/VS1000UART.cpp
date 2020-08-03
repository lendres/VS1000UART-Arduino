/*! \file VS1000UART.cpp  */

/*!
	VS1000UART
	Based on the "Adafruit_Soundboard" class written by Limor Fried/Ladyada for Adafruit Industries.

	Updated by Lance A. Endres to  to fix compiler warnings, cleanup inconsistancies, moderize, improve readability, add comments, fix incorrect comments,
	add functionality, and generalize it for the VS1000 chip used on other boards.

	To a casual observer or inexperienced programmer some changes may appear arbitrary, but they are founded in logic and experience.

	Adafruit_Soundboard
	This is a library for the Adafruit Sound Boards in UART mode

	----> http://www.adafruit.com/products/2342
	----> http://www.adafruit.com/products/2341
	----> http://www.adafruit.com/products/2217
	----> http://www.adafruit.com/products/2210
	----> http://www.adafruit.com/products/2133
	----> http://www.adafruit.com/products/2220

	Check out the links above for our tutorials and wiring diagrams.  This sound fx driver uses TTL Serial to communicate.

	Adafruit invests time and resources providing this open source code, please support Adafruit and open-source hardware by
	purchasing products from Adafruit!
 
  	Written by Limor Fried/Ladyada for Adafruit Industries.

  	MIT license, all text above must be included in any redistribution.
*/

#include "VS1000UART.h"

// Initialize static members.
const uint8_t		VS1000UART::_lineBufferSize 			= 80;
const uint8_t		VS1000UART::_chipMinVolume				= 0;
const uint8_t		VS1000UART::_chipMaxVolume				= 204;

VS1000UART::VS1000UART(Stream* chipStream, int8_t resetPin) :
	_chipStream(chipStream),
	_resetPin(resetPin),
	_minimumVolume(_chipMinVolume),
	_maximumVolume(_chipMaxVolume),
	_volumeIncrement((_maximumVolume - _minimumVolume)/10.0),
	_minimumLevel(VOLUME0),
	_maximumLevel(VOLUME10),
	_persistentVolume(false),
	_memoryAddress(0)
{
	_lineBuffer = new char[_lineBufferSize];
}

VS1000UART::VS1000UART(Stream* chipStream, int8_t resetPin, int memoryAddress) :
	_chipStream(chipStream),
	_resetPin(resetPin),
	_minimumVolume(_chipMinVolume),
	_maximumVolume(_chipMaxVolume),
	_volumeIncrement((_maximumVolume - _minimumVolume)/10.0),
	_minimumLevel(VOLUME0),
	_maximumLevel(VOLUME10),
	_persistentVolume(true),
	_memoryAddress(memoryAddress)
{
	_lineBuffer = new char[_lineBufferSize];
}

VS1000UART::~VS1000UART()
{
	if (_lineBuffer)
	{
		delete[] _lineBuffer;
	}
}

void VS1000UART::setMinimumVolume(uint8_t minimumVolume)
{
	_minimumVolume = minimumVolume;
}

void VS1000UART::setMaximumVolume(uint8_t maximumVolume)
{
	_maximumVolume = maximumVolume;
}

void VS1000UART::useLowerLevelOne(bool useLowerLevelOne)
{
	if (useLowerLevelOne)
	{
		_minimumLevel = VOLUME1;
	}
	else
	{
		_minimumLevel = VOLUME0;
	}
}

void VS1000UART::setMaximumLevel(VOLUMELEVEL volumeLevel)
{
	_maximumLevel = volumeLevel;
}

void VS1000UART::begin()
{
	_chipStream->setTimeout(500);

	// The reset pin is connected to Vcc.  By switching to input, we will let the reset be pulled to Vcc.
	pinMode(_resetPin, INPUT);

	// Calculate volume increment based on volume and level settings.
	_volumeIncrement = ((float)_maximumVolume - _minimumVolume) / (_maximumLevel - _minimumLevel);
	synchVolumes();
}

bool VS1000UART::reset()
{
	// Reset by bringing the reset pin low.  First we have to switch the pin to output.  Then pull it low.
	pinMode(_resetPin, OUTPUT);
	digitalWrite(_resetPin, LOW);
	delay(15);

	// Swith the pin back to input.  The reset pin is connected to Vcc.  By switching to input, we will let the
	// reset be pulled back to Vcc.  The chip may not run at the same voltage as the Arduino so we don't control it
	// with the Arduino.
	pinMode(_resetPin, INPUT);

	// Give a bit of time to boot up.
	delay(1000);

	// Eat new line.
	readLine();

	#if VS1000DEBUGLEVEL > 0
		Serial.println();
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	// "Adafruit FX Sound Board 9/10/14"
	readLine();

	#if VS1000DEBUGLEVEL > 0
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	if (!strstr(_lineBuffer, "Adafruit FX Sound Board"))
	{
		//return false;
	}

	delay(250);

	readLine();
	#if VS1000DEBUGLEVEL > 0
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	readLine();
	#if VS1000DEBUGLEVEL > 0
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	// After restarting the chip, the volumes need to by synchronized.
	synchVolumes();

	return true;
}

uint8_t VS1000UART::listFiles(char fileNames[][12], uint32_t fileSizes[], uint8_t arrayLength)
{
	sendCommand(F("L\n"));

	// Reset number of files.  The "_numberOfFiles" is used in the function to index in to the "_fileNames"
	// and "_fileSizes" array.  Therefore, the number of filex must be incremented after all the other work is done
	// since the number of files is one less than the index of the file in the array (arrays are zero based).
	uint8_t numberOfFiles = 0;

	// Loop so long as were are getting new lines back and we haven't exceded our file limit.  The readLine function will return
	// a zero when it does not read anything.
	while (readLine() && numberOfFiles < arrayLength)
	{
		// File names are 8.3 without the separating dot.
		// Line returned is composed of file name, tab, zero padded right justified file size.
		// Example: 04LATCHWAV	0000051892
		// [0]	- Name start.
		// [10]	- Last name character.
		// [11]	- Tab.
		// [12]	- First size digit.
		// [21]	- Last size digit.
		// Copy over the file name.  Then terminate the string in the twelveth character (position [11]).
		memcpy(fileNames[numberOfFiles], _lineBuffer, 11);
		fileNames[numberOfFiles][11] = 0;

		// Parse out the file size after the name + tab (tab is 12th character ).
		fileSizes[numberOfFiles] = 0;
		for (uint8_t i = 12; i < 22; i++)
		{
			char character = _lineBuffer[i];

			// This shouldn't be needed unless garabage got transmitted.
			if (!isdigit(character))
			{
				break;
			}

			// Essentially, this moves the decimal place and adds in the new digit read.  The file size is right justified, but the
			// left is padded with zeros, so the file size will just be zero until we hit the first significant digit.
			fileSizes[numberOfFiles] *= 10;
			fileSizes[numberOfFiles] += atoi(&character);
		}

		// Now that we are done with the work, we increment the counter.  Don't do this sooner as it is used to index the other arrays.
		numberOfFiles++;
	}

	return numberOfFiles;
}

bool VS1000UART::playFile(uint8_t fileNumber)
{
	sendCommand(F("#"));
	_chipStream->println(fileNumber);

	// Eat return.
	readLine();
	readLine();

	// Check we got "play" back.
	if (strstr(_lineBuffer, "play") == 0)
	{
		return false;
	}

	// Check the # is correct.
	int playing = atoi(_lineBuffer + 5);

	if (fileNumber != playing)
	{
		return false;
	}

	return true;
}

bool VS1000UART::playFile(char* fileName)
{
	sendCommand(F("P"));
	_chipStream->println(fileName);

	// Eat return.
	readLine();
	readLine();

	// Check we got "play" back.
	if (strstr(_lineBuffer, "play") == 0)
	{
		return false;
	}

	return true;
}

uint8_t VS1000UART::volumeUp()
{
	volumeUpWithoutSaving();
	saveVolumeToMemory();
	return _volume;
}

uint8_t VS1000UART::volumeDown()
{
	volumeDownWithoutSaving();
	saveVolumeToMemory();
	return _volume;
}

uint8_t VS1000UART::setVolume(uint8_t volume)
{
	// If we need to turn volume down.
	if (_volume > volume)
	{
		while (_volume > volume)
		{
			volumeDownWithoutSaving();
		}
	}

	// If we need to turn volume up.
	if (_volume < volume)
	{
		while (_volume < volume)
		{
			volumeUpWithoutSaving();
		}
	}

	saveVolumeToMemory();
	return _volume;
}

uint8_t VS1000UART::getVolume()
{
	return _volume;
}

VS1000UART::VOLUMELEVEL VS1000UART::volumeLevelUp()
{
	return setVolumeLevel((VOLUMELEVEL)(getVolumeLevel() + 1));
}

VS1000UART::VOLUMELEVEL VS1000UART::volumeLevelDown()
{
	return setVolumeLevel((VOLUMELEVEL)(getVolumeLevel() - 1));
}

VS1000UART::VOLUMELEVEL VS1000UART::setVolumeLevel(VOLUMELEVEL level)
{
	// Never go above the maximum level.
	if (level > _maximumLevel)
	{
		level = _maximumLevel;
	}

	// Never go below the minimum level.
	if (level < _minimumLevel)
	{
		level = _minimumLevel;
	}


	// Calculate new volume from level and size of increment per level.
	uint8_t volume = round((level - _minimumLevel) * _volumeIncrement + _minimumVolume);

	setVolume(volume);

	return level;
}

VS1000UART::VOLUMELEVEL VS1000UART::cycleVolumeLevel()
{
	VOLUMELEVEL level = getVolumeLevel();

	if (level == _maximumLevel)
	{
		level = _minimumLevel;
	}
	else
	{
		level = (VOLUMELEVEL)(level + 1);
	}
	

	return setVolumeLevel(level);
}

VS1000UART::VOLUMELEVEL VS1000UART::getVolumeLevel()
{
	return calculateLevelFromVolume(_volume);
}

bool VS1000UART::pausePlay()
{
	sendCommand(F("=\n"));

	return checkCommandResult('=');
}

bool VS1000UART::resumePlay()
{
	sendCommand(F(">\n"));

	return checkCommandResult('>');
}

bool VS1000UART::stopPlay()
{
	sendCommand(F("q\n"));

	return checkCommandResult('q');
}

bool VS1000UART::playTime(uint32_t* current, uint32_t* total)
{
	sendCommand(F("t"));

	readLine();

	if (strlen(_lineBuffer) != 12)
	{
		// There seems to be a bug in the firmware.  If you call to playTime when a track is not playing, then call to list files the list files command fails.
		// It's not known if the bug is from Adafruit or VSI.  This command is not in the VSI1000 data sheet, so it's either undocumented or added by Adafruit.
		// As a work around, we can send a new line character and clear the buffer.
		sendCommand(F("\n"));
		readLine();
		return false;
	}

	*current	= atoi(_lineBuffer);
	*total		= atoi(_lineBuffer + 6);

	return true;
}

bool VS1000UART::fileSize(uint32_t* remain, uint32_t* total)
{
	sendCommand(F("s"));

	if (strlen(_lineBuffer) != 22)
	{
		return false;
	}

	*remain	= atol(_lineBuffer);
	*total	= atol(_lineBuffer + 11);

	return true;
}

void VS1000UART::continuousPlayMode()
{

}

void VS1000UART::sendCommand(const __FlashStringHelper* command)
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(command);
}

bool VS1000UART::checkCommandResult(char command)
{
	// Read the response with "readLine" which returns the number of bytes read.  No bytes are read, then return false.
	// This uses short-circuit logic, if the first part true, the second part doesn't execute, it goes straight to the return.
	if (!readLine() || _lineBuffer[0] != command)
	{
		return false;
	}
	
	return true;
}

VS1000UART::VOLUMELEVEL VS1000UART::calculateLevelFromVolume(uint8_t volume)
{
	return (VOLUMELEVEL)(round((volume - _minimumVolume) / _volumeIncrement + _minimumLevel));
}

void VS1000UART::synchVolumes()
{
	// We need to initialize the "_volume" variable before a call to "setVolume."  To do that, we will make a call to volume up
	// and that will set the variable as part of the call.  Do not save the volume or we overwrite the value we are trying to restore.
	volumeUpWithoutSaving();

	if (_persistentVolume)
	{
		// Read the volume from memory.  Then calculate and set a new volume level.
		uint8_t volume = EEPROM.readInt(_memoryAddress);
		setVolumeLevel(calculateLevelFromVolume(volume));
	}
}

int VS1000UART::readLine()
{
	int x 			= _chipStream->readBytesUntil('\n', _lineBuffer, _lineBufferSize);
	_lineBuffer[x]	= 0;

	// Check for when the new line is followed by a carriage return.
	if (_chipStream->peek() == '\r')
	{
		_chipStream->read();
	}

	#if VS1000DEBUGLEVEL > 1
		Serial.print(F("Line buffer\tbits: "));
		Serial.print(x);
		Serial.print(F("\tvalue: "));
		Serial.println(_lineBuffer);
	#endif

	// The number of characters placed in the buffer (0 means no valid data found).
	return x;
}

void VS1000UART::volumeUpWithoutSaving()
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	// Send the volume change command to the chip.
	_chipStream->println(F("+"));

	// Reads the returned value from the chip and saves it.
	readVolumeFromChip();
}

void VS1000UART::volumeDownWithoutSaving()
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	// Send the volume change command to the chip.
	_chipStream->println(F("-"));

	// Reads the returned value from the chip and saves it.
	readVolumeFromChip();
}

void VS1000UART::readVolumeFromChip()
{
	// Read the serial stream from the chip.
	readLine();

	// Convert the text to a numerical volume.
	_volume = atoi(_lineBuffer);
}

void VS1000UART::saveVolumeToMemory()
{
	// When we have volume saving enabled, we save it to the flash memory on the Arduino.
	if (_persistentVolume)
	{
		EEPROM.writeInt(_memoryAddress, _volume);
	}
}
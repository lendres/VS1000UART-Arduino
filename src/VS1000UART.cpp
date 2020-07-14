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

VS1000UART::VS1000UART(Stream* chipStream, int8_t resetPin) :
	_chipStream(chipStream),
	_resetPin(resetPin),
	_numberOfFiles(0),
	_minimumVolume(MINVOLUME),
	_maximumVolume(MAXVOLUME),
	_volumeIncrement(VOLUMEINCREMENT),
	_minimumLevel(VOLUME0),
	_maximumLevel(VOLUME10),
	_persistentVolume(false),
	_memoryAddress(0)
{
	_chipStream->setTimeout(500);
}

VS1000UART::VS1000UART(Stream* chipStream, int8_t resetPin, int memoryAddress) :
	_chipStream(chipStream),
	_resetPin(resetPin),
	_numberOfFiles(0),
	_minimumVolume(MINVOLUME),
	_maximumVolume(MAXVOLUME),
	_volumeIncrement(VOLUMEINCREMENT),
	_minimumLevel(VOLUME0),
	_maximumLevel(VOLUME10),
	_persistentVolume(true),
	_memoryAddress(memoryAddress)
{
	_chipStream->setTimeout(500);
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
	// Calculate volume increment based on volume and level settings.
	_volumeIncrement = (_maximumVolume - _minimumVolume) / (_maximumLevel - _minimumLevel);
	synchVolumes();
}

bool VS1000UART::reset(void)
{
	// Do a hard reset by bringing the reset pin low then read out the output lines.
	digitalWrite(_resetPin, LOW);
	pinMode(_resetPin, OUTPUT);
	delay(10);
	pinMode(_resetPin, INPUT);

	// Give a bit of time to boot up.
	delay(1000);

	// Eat new line.
	readLine();

	#ifdef DEBUG
		Serial.println();
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	// "Adafruit FX Sound Board 9/10/14"
	readLine();

	#ifdef DEBUG
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	if (!strstr(_lineBuffer, "Adafruit FX Sound Board"))
	{
		//return false;
	}

	delay(250);

	readLine();
	#ifdef DEBUG
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	readLine();
	#ifdef DEBUG
		Serial.print(F("Audio chip: "));
		Serial.println(_lineBuffer);
	#endif

	// After restarting the chip, the volumes need to by synchronized.
	synchVolumes();

	return true;
}

uint8_t VS1000UART::listFiles()
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	// 'L' for list.
	_chipStream->println(F("L"));

	// Reset number of files.  The "_numberOfFiles" is used in the function to index in to the "_fileNames"
	// and "_fileSizes" array.  Therefore, the number of filex must be incremented after all the other work is done
	// since the number of files is one less than the index of the file in the array (arrays are zero based).
	_numberOfFiles = 0;

	while (_chipStream->readBytesUntil('\n', _lineBuffer, LINE_BUFFER_SIZE))
	{
		// Copy over the file name.
		memcpy(_fileNames[_numberOfFiles], _lineBuffer, 12);
		_fileNames[_numberOfFiles][11] = 0;

		// Parse out the file size after the name + tab.
		_fileSizes[_numberOfFiles] = 0;
		for (uint8_t i = 0; i < 16; i++)
		{
			char character = _lineBuffer[12 + i];

			if (!isdigit(character))
			{
				break;
			}

			_fileSizes[_numberOfFiles] *= 10;
			_fileSizes[_numberOfFiles] += atoi(&character);
		}

		// Now that we are done with the work, we increment the counter.  Don't do this sooner as it is used to index the other arrays.
		_numberOfFiles++;

		// Make sure we don't over run the ends of our arrays.
		if (_numberOfFiles >= MAXFILES)
		{
			break;
		}
	}

	return _numberOfFiles;
}

char* VS1000UART::fileName(uint8_t fileNumber)
{
	// Make sure the parameter is within bounds.
	if (fileNumber >= _numberOfFiles)
	{
		return NULL;
	}

	return _fileNames[fileNumber];
}

uint32_t VS1000UART::fileSize(uint8_t fileNumber)
{
	// Make sure the parameter is within bounds.
	if (fileNumber >= _numberOfFiles)
	{
		return 0;
	}

	return _fileSizes[fileNumber];
}

bool VS1000UART::playTrack(uint8_t trackNumber)
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(F("#"));
	_chipStream->println(trackNumber);

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

	if (trackNumber != playing)
	{
		return false;
	}

	return true;
}

bool VS1000UART::playTrack(char* name)
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(F("P"));
	_chipStream->println(name);

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
	uint8_t volume = (level - _minimumLevel) * _volumeIncrement + _minimumVolume;

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
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(F("="));

	if (!_chipStream->readBytes(_lineBuffer, 1))
	{
		return false;
	}

	if (_lineBuffer[0] != '=')
	{
		return false;
	}

	return true;
}

bool VS1000UART::resumePlay()
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(F(">"));
	if (!_chipStream->readBytes(_lineBuffer, 1))
	{
		return false;
	}

	if (_lineBuffer[0] != '>')
	{
		return false;
	}
	
	return true;
}

bool VS1000UART::stopPlay()
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(F("q"));
	readLine();

	if (_lineBuffer[0] != 'q')
	{
		return false;
	}

	return true;
}

bool VS1000UART::trackTime(uint32_t* current, uint32_t* total)
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(F("t"));
	readLine();
	
	if (strlen(_lineBuffer) != 12)
	{
		return false;
	}

	*current	= atoi(_lineBuffer);
	*total		= atoi(_lineBuffer + 6);

	return true;
}

bool VS1000UART::trackSize(uint32_t* remain, uint32_t* total)
{
	while (_chipStream->available())
	{
		_chipStream->read();
	}

	_chipStream->print(F("s"));
	readLine();

	if (strlen(_lineBuffer) != 22)
	{
		return false;
	}

	*remain	= atol(_lineBuffer);
	*total	= atol(_lineBuffer + 11);

	return true;
}

VS1000UART::VOLUMELEVEL VS1000UART::calculateLevelFromVolume(uint8_t volume)
{
Serial.println();
Serial.println("CALCULATE LEVEL");
Serial.print("Volume: ");
Serial.println(_volume);
Serial.print("max-min: ");
Serial.println((float)_maximumLevel - _minimumLevel);
Serial.print("max-min*vol: ");
Serial.println(((float)_maximumLevel - _minimumLevel) * volume);
Serial.print("vol max-min: ");
Serial.println(_maximumVolume - _minimumVolume);
Serial.print("Total: ");
Serial.println(((float)_maximumLevel - _minimumLevel) * volume / (_maximumVolume - _minimumVolume));
	return (VOLUMELEVEL)(round(((float)_maximumLevel - _minimumLevel) * volume / (_maximumVolume - _minimumVolume) + _minimumLevel));
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
	int x 			= _chipStream->readBytesUntil('\n', _lineBuffer, LINE_BUFFER_SIZE);
	_lineBuffer[x]	= 0;

	// Check for when the new line is followed by a carriage return.
	if (_chipStream->peek() == '\r')
	{
		_chipStream->read();
	}

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
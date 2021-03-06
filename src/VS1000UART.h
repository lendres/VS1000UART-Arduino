/*! @file VS1000UART.h */

/*!
 	\mainpage VS1000 Audio Chip UART Interface

	\section intro_sec Introduction

 	\section author Author
	Based on the "Adafruit_Soundboard" class written by Limor Fried/Ladyada for Adafruit Industries.

	Updated by Lance A. Endres to  to fix compiler warnings, cleanup inconsistancies, moderize, improve readability, add comments, fix incorrect comments,
	add functionality, and generalize it for the VS1000 chip used on other boards.

	To a casual observer or inexperienced programmer some changes may appear arbitrary, but they are founded in logic and experience.

	New features:
	- Added "setVolume"
		Automatically goes to a specified volume level.  This allows setting the volume explicitly to a specified level.
	- Added persistent volume.
		Allows saving the set volume level to the Arduinos EEPROM memory and having it restored on start up.
	- Added debugging levels for printing output.

	Bug fixes and other improvements:
	- More comments to explain code and document behavior.
	- Consistent use of F() macro to reduce RAM used.
	- Getting track/play time while not playing broke the list files command.  Implemented a work around, but problem is in firmware.
	- Sets the reset pin to input in the begin function and rearrange calls in the reset.  This *seems* to stabilize behavior somewhat.
	- Much, much better code resuse.
	- Elimination of unused code.
	- Removed storage of file names, file sizes, and number of files.  This was only used to return names and sizes for the example created by Adafruit.  By
		having the storage arrays passed into the list files function, they can be removed from the class and use less memory when not in use.
	- Code style far more consistent and readable.
*/

/*!
 	\mainpage Adafruit Soundboard Library

	\section intro_sec Introduction

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
 
 	\section author Author
  	Written by Limor Fried/Ladyada for Adafruit Industries.
 
  	\section license License
  	MIT license, all text above must be included in any redistribution.
*/

#ifndef VS1000UART_H
#define VS1000UART_H

#include <Arduino.h>
#include <EEPROMex.h>

// Debugging output to the serial monitor.  Set the value to specify the amount of messages.
// 0 - No messages.
// 1 - Basic messages (boot up).
// 2 - Additional messages (line buffer).
#define VS1000DEBUGLEVEL	0

/// \brief Class that stores the state and functions of the soundboard object.
class VS1000UART
{
	public:
		/// \brief Enumeration for setting absolute volume level.  Level 0 is completely off, level 10 is maximum volume.
		enum VOLUMELEVEL : uint8_t
		{
			VOLUME0,
			VOLUME1,
			VOLUME2,
			VOLUME3,
			VOLUME4,
			VOLUME5,
			VOLUME6,
			VOLUME7,
			VOLUME8,
			VOLUME9,
			VOLUME10
		};

	// Constructors.
	public:
		/// \brief Constructor that lets you provide your own serial communicate stream.
		/// \param chipStream Pointer to the serial stream used to communicate with the chip.
		/// \param resetPin Reset pin.
		VS1000UART(Stream* chipStream, int8_t resetPin);

		/// \brief Same as previous, but can save and restore the volume.
		/// \param chipStream Pointer to the serial stream used to communicate with the chip.
		/// \param resetPin Reset pin.
		/// \param memoryAddress Memory address to save volume level.
		VS1000UART(Stream* chipStream, int8_t resetPin, int memoryAddress);

		/// \brief Destructor.
		~VS1000UART();

	// Functions to use in setup.
	public:
		/// \brief Sets a lower limit on the volume.  Useful to adjust the volume for a particular setup.
		/// \parm minimumVolume The minimum volume level.
		void setMinimumVolume(uint8_t minimumVolume);

		/// \brief Sets an upper limit on the volume.  Useful to adjust to volume for a particular setup.
		/// \parm maximumVolume The maximum volume level.
		void setMaximumVolume(uint8_t maximumVolume);

		/// \brief Sets which volume level is the minimum.
		/// \param useLowerLevelOne If true, VOLUME1 is the lowest level.  If false, VOLUME0 is the lowest level.
		void useLowerLevelOne(bool useLowerLevelOne);

		/// \brief Sets the maximum level.  For example, if only 5 increments of volume are required, it can be adjusted here.
		void setMaximumLevel(VOLUMELEVEL volumeLevel);

		/// \brief Last call to this class for use in the "Setup" function.
		void begin();

	// Functions for controlling/interacting with the audio chip.
	public:
		/// \brief Hard reset of the chip.
		/// \return Returns the output lines.
		bool reset();

		/// \brief Query the board for the # of files and names/sizes.
		/// \return Returns Number of files.
		// uint8_t listFiles();
		uint8_t listFiles(char fileNames[][12], uint32_t fileSizes[], uint8_t arrayLength);

		/// \brief Raises the volume.
		/// \return Returns the current volume.
		uint8_t volumeUp();

		/// \brief Lowers the volume.
		/// \return Returns the current volume.
		uint8_t volumeDown();

		/// \brief Sets the volume to a specified value.
		/// \param volume The volume to set.
		/// \return The new volume level.
		uint8_t setVolume(uint8_t volume);

		/// \brief Get the volume.
		/// \returns Returns the current volume setting.
		uint8_t getVolume();

		/// \brief Raises the volume one level.
		/// \return Returns the current volume.
		VOLUMELEVEL volumeLevelUp();

		/// \brief Lowers the volume one level
		/// \return Returns the current volume.
		VOLUMELEVEL volumeLevelDown();

		/// \brief Set the volume to a specific level.
		/// \return Returns the current volume.
		VOLUMELEVEL setVolumeLevel(VOLUMELEVEL level);

		/// \brief Increases the volume level by one.  If the current level is the maximum, it resets to the lowest level.
		VOLUMELEVEL cycleVolumeLevel();

		/// \brief Gets the volume as a VOLUMELEVEL.
		/// \return Returns the volume as the closest VOLUMELEVEL.
		VOLUMELEVEL getVolumeLevel();

		/// \brief Play the specified track.
		/// \param n track id.
		/// \return Returns true if the track was played.
		bool playFile(uint8_t fileNumber);

		/// \brief Play the specified track.
		/// \param name track name.
		/// \return Returns true if the track was played.
		bool playFile(char* fileName);

		/// \brief Pauses track.
		/// \return Returns if pausing was successful.
		bool pausePlay();

		/// \brief Unpauses track.
		/// \return Returns if unpausing was successful.
		bool resumePlay();

		/// \brief Stops track.
		/// \return Returns if stopping was successful.
		bool stopPlay();

		/// \brief Returns the track time.
		/// \param current Buffer with the current track time.
		/// \param total Buffer with the total track time.
		/// \return Returns the current track time.
		bool playTime(uint32_t* current, uint32_t* total);

		/// \brief Returns the track size.
		/// \param current Buffer with the current track size.
		/// \param total Buffer with the total track size.
		/// \return Returns how many bytes are remaining over the total track size.
		bool fileSize(uint32_t* current, uint32_t* total);

		void continuousPlayMode();

	// Support functions.
	private:
		/// \brief Send a command to the audio chip.
		void sendCommand(const __FlashStringHelper* command);

		/// \brief Read the response back from the audio chip and check it against the expected command.
		bool checkCommandResult(char command);

		/// \brief Converts a volume to a volume level.
		VOLUMELEVEL calculateLevelFromVolume(uint8_t volume);

		/// \brief Synchs the volume on the chip and the class's stored volume.
		void synchVolumes();

		/// \brief Reads a line from the stream.
		/// \return The number of characters placed in the buffer (0 means no valid data found).
		int readLine();

		/// \brief Raises the volume without saving the value.
		/// \return Returns the current volume.
		void volumeUpWithoutSaving();

		/// \brief Lowers the volume without saving the value.
		/// \return Returns the current volume.
		void volumeDownWithoutSaving();

		/// \brief Read the volume level from the chip.
		void readVolumeFromChip();

		/// \brief Stores the volume.
		void saveVolumeToMemory();

	private:
		// Constant parameters for configuration.  Encapsulate variables to prevent name conflict.
		static const uint8_t		_lineBufferSize;
		static const uint8_t		_chipMinVolume;
		static const uint8_t		_chipMaxVolume;

		// Stream for the chip/board, e.g. SoftwareSerial or Serial1.
		Stream*						_chipStream;

		int8_t						_resetPin;
		char*						_lineBuffer;

		// Volume.
		uint8_t						_minimumVolume;
		uint8_t						_maximumVolume;
		float						_volumeIncrement;
		VOLUMELEVEL					_minimumLevel;
		VOLUMELEVEL					_maximumLevel;
		bool						_persistentVolume;
		int							_memoryAddress;
		uint8_t						_volume;
};

#endif
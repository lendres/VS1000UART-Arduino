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

#define DEBUG 				1

#define LINE_BUFFER_SIZE 	80		//!< Size of the line buffer.
#define MAXFILES 			25		//!< Max number of files.

#define MINVOLUME			0
#define MAXVOLUME			202
#define VOLUMEINCREMENT		MAXVOLUME / 10

/// \brief Class that stores the state and functions of the soundboard object.
class VS1000UART
{
	public:
		/// \brief Enumeration for setting absolute volume level.
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

	public:
		/// \brief Constructor.
		/// \param chipStream Pointer to the Serial stream.
		/// \param resetPin Reset pin.
		VS1000UART(Stream* chipStream, int8_t resetPin);

		/// \brief Constructor.
		/// \param chipStream Pointer to the Serial stream.
		/// \param resetPin Reset pin.
		/// \param memoryAddress Memory address to save volume level.
		VS1000UART(Stream* chipStream, int8_t resetPin, int memoryAddress);

		/// \brief Run in the "Setup" function.
		void begin();

		/// \brief Hard reset of the chip.
		/// \return Returns the output lines.
		bool reset();

		/// \brief Query the board for the # of files and names/sizes.
		/// \return Returns the information about the files.
		uint8_t listFiles();

		/// \brief Returns the file name.
		/// \param n Id of the file.
		/// \return Returns the file name.
		char* fileName(uint8_t n);

		/// \brief Returns the size of the file.
		/// \param n id of the file.
		/// \return Returns the file size.
		uint32_t fileSize(uint8_t n);

		/// \brief Raises the volume.
		/// \return Returns the current volume.
		uint8_t volumeUp();

		/// \brief Lowers the volume.
		/// \return Returns the current volume.
		uint8_t volumeDown();

		/// \brief Set the volume to a specific level.
		/// \return Returns the current volume.
		uint8_t setVolume(VOLUMELEVEL level);

		/// \brief Gets the volume as a VOLUMELEVEL.
		/// \return Returns the volume as the closest VOLUMELEVEL.
		VOLUMELEVEL getVolumeLevel();

		/// \brief Play the specified track.
		/// \param n track id.
		/// \return Returns true if the track was played.
		bool playTrack(uint8_t n);

		/// \brief Play the specified track.
		/// \param name track name.
		/// \return Returns true if the track was played.
		bool playTrack(char* name);

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
		bool trackTime(uint32_t* current, uint32_t* total);

		/// \brief Returns the track size.
		/// \param current Buffer with the current track size.
		/// \param total Buffer with the total track size.
		/// \return Returns how many bytes are remaining over the total track size.
		bool trackSize(uint32_t* current, uint32_t* total);

	private:
		/// \brief Synchs the volume on the chip and the class's stored volume.
		void synchVolumes();
		
		/// \brief Reads a line from the stream.
		/// \return The number of characters placed in the buffer (0 means no valid data found).
		int readLine();

		/// \brief Raises the volume without saving the value.
		/// \return Returns the current volume.
		uint8_t volumeUpWithoutSaving();

		/// \brief Lowers the volume without saving the value.
		/// \return Returns the current volume.
		uint8_t volumeDownWithoutSaving();

		/// \brief Read the volume level from the chip.
		void readVolumeFromChip();

		/// \brief Stores the volume.
		void saveVolumeToMemory();

	private:
		// Stream for the chip/board, e.g. SoftwareSerial or Serial1.
		Stream*		_chipStream;

		int8_t		_resetPin;

		char		_lineBuffer[LINE_BUFFER_SIZE];
		bool		_writing;

		// File name & size caching.
		uint8_t		_numberOfFiles;
		char		_fileNames[MAXFILES][12];
		uint32_t	_fileSizes[MAXFILES];

		// Volume.
		bool		_persistentVolume;
		int			_memoryAddress;
		uint8_t		_volume;
};

#endif
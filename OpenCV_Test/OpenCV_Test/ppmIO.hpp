#pragma once
#include <string>
#include <stdlib.h>
#include <sstream>
#include <fstream>
#include <iostream>

namespace PPM
{
	struct PPMFile final
	{
		PPMFile()
			: name("")
			, magicNumber("")
			, width(0)
			, height(0)
			, maxValue(0)
			, data(nullptr)
			, wasSuccessful(false)
		{ }
		PPMFile(const PPMFile& other) = delete;
		PPMFile(PPMFile&& other) noexcept
			: name(std::move(other.name))
			, magicNumber(std::move(other.magicNumber))
			, width(std::move(other.width))
			, height(std::move(other.height))
			, maxValue(std::move(other.maxValue))
			, data(std::move(other.data))
			, wasSuccessful(std::move(other.wasSuccessful))
		{
			other.data = nullptr;
		}

		~PPMFile()
		{
			delete[] data;
		}

		PPMFile& operator=(const PPMFile& other) = delete;
		PPMFile& operator=(PPMFile&& other) noexcept
		{
			delete[] data;
			
			name = std::move(other.name);
			magicNumber = std::move(other.magicNumber);
			width = std::move(other.width);
			height = std::move(other.height);
			maxValue = std::move(other.maxValue);
			data = std::move(other.data);
			wasSuccessful = std::move(other.wasSuccessful);

			other.data = nullptr;

			return *this;
		}

		size_t headerOffset() const
		{
			return 
				2 +									// Magic number
				1 +									// Whitespace
				std::to_string(width).size() +		// Width
				1 +									// Whitespace
				std::to_string(height).size() +		// Height
				1 +									// Whitespace
				std::to_string(maxValue).size() +	// Max value
				1;									// Whitespace
		}
		size_t elementSize() const
		{
			return (maxValue < 256 ? 1 : 2);
		}

		std::string name;
		std::string magicNumber;
		size_t width;
		size_t height;
		size_t maxValue;
		unsigned char* data;
		bool wasSuccessful;

		static constexpr size_t CHANNELS = 3;
	};

	// Called by read(), do not call manually
	void readBinary(PPMFile& ppmFile)
	{
		FILE* file;
		
		// Open file
		errno_t error = fopen_s(&file, ppmFile.name.c_str(), "rb");
		if (error || !file)
			return;

		// Move file pointer to data after header
		fseek(file, ppmFile.headerOffset(), SEEK_SET);
		
		// Figure out element size
		const size_t ELEMENT_SIZE = ppmFile.elementSize();
		
		// Read pixel data
		const size_t N_VALUES = ppmFile.width * ppmFile.height * PPMFile::CHANNELS;
		ppmFile.data = new unsigned char[N_VALUES * ELEMENT_SIZE];
		const size_t BYTES_READ = fread(ppmFile.data, ELEMENT_SIZE, N_VALUES, file);
		fclose(file);

		if (BYTES_READ != N_VALUES * ELEMENT_SIZE)
		{
			std::cout << "Could not read file";
			return;
		}

		ppmFile.wasSuccessful = true;
		return;
	}

	// Called by read(), do not call manually
	void readPlain(PPMFile& ppmFile)
	{
		std::ifstream infile;
		std::string str;

		// Open file
		infile.open(ppmFile.name, std::ifstream::in);
		if (!infile.is_open())
			return;
		
		// Move file pointer to data after header
		infile >> str >> str >> str >> str;
		//infile.seekg(ppmFile.headerOffset() - 1, std::ios::cur);
		
		// Figure out element size
		const size_t ELEMENT_SIZE = ppmFile.elementSize();

		// Allocate memory
		const size_t N_VALUES = ppmFile.width * ppmFile.height * PPMFile::CHANNELS;
		ppmFile.data = new unsigned char[N_VALUES * ELEMENT_SIZE];

		if (ELEMENT_SIZE == 1)
		{
			// Read all the pixel data
			for (size_t i = 0; i < N_VALUES; i++)
			{
				short value;
				infile >> value;
				ppmFile.data[i] = value;
			}
		}
		else
		{
			// Read all the pixel data
			for (size_t i = 0; i < N_VALUES; i++)
			{
				short value;
				infile >> value;
				ppmFile.data[i * 2 + 0] = value >> 8;	// First byte
				ppmFile.data[i * 2 + 1] = value & 0xff;	// Second byte
			}
		}
		

		infile.close();

		ppmFile.wasSuccessful = true;
		return;
	}
	
	// Calls either readBinary() or readPlain() depending on the file header
	[[no_discard]] PPMFile read(std::string fileName)
	{
		PPMFile ppmFile;
		ppmFile.name = fileName;

		// Open file
		std::ifstream infile;
		infile.open(fileName, std::ifstream::in);
		if (!infile.is_open())
			return ppmFile;

		// Read only the file header
		infile >> ppmFile.magicNumber >> ppmFile.width >> ppmFile.height >> ppmFile.maxValue;
		infile.close();

		if (ppmFile.magicNumber == "P3")
		{
			readPlain(ppmFile);
		}
		else if (ppmFile.magicNumber == "P6")
		{
			readBinary(ppmFile);
		}

		return ppmFile;
	}

	bool writeBinary(unsigned char* data, size_t width, size_t height, size_t maxValue, const char* fileName)
	{
		FILE* file;
		errno_t error;

		// Open file
		error = fopen_s(&file, fileName, "wb");
		if (error || !file) return false;

		// Print header (magic number, width, height, max value)
		fprintf(file, "P6\n%d %d\n%d\n", static_cast<int>(width), static_cast<int>(height), static_cast<int>(maxValue));

		// Print data
		size_t bytesWritten = fwrite(data, 1ULL, width * height * 3ULL, file);
		size_t bytesToWrite = width * height * 3ULL;
		fclose(file);

		return bytesWritten == bytesToWrite;
	}
	bool writePlain(unsigned char* data, size_t width, size_t height, size_t maxValue, const char* fileName)
	{
		std::ofstream outFile;
		outFile.open(fileName);
		if (!outFile.is_open())
			return false;

		outFile << "P3\n" << width << "\n" << height << "\n" << maxValue << "\n";

		const size_t N_VALUES = width * height * 3ULL;
		for (size_t i = 0; i < N_VALUES; i++)
		{
			outFile << static_cast<int>(data[i]) << " ";

			// Add a new line every 5 pixels
			// A line in the plain ppm format can't be longer than 70 characters (Found at: http://netpbm.sourceforge.net/doc/ppm.html)
			// 5 pixels * 3 channels * (1 to 3 value characters + 1 space) = 30 to 60 characters per line
			if (i % 15 == 14)
			{
				outFile << "\n";
			}
		}

		outFile.close();

		return true;
	}
}
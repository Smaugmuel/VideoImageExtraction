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

		std::string name;
		std::string magicNumber;
		size_t width;
		size_t height;
		size_t maxValue;
		unsigned char* data;
		bool wasSuccessful;
	};

	// Called by read(), do not call manually
	void readBinary(PPMFile& ppmFile)
	{
		size_t nValues;
		size_t bytesRead;
		size_t headerOffset;
		FILE* file;
		char* buffer;
		errno_t error;

		
		// Open file
		error = fopen_s(&file, ppmFile.name.c_str(), "rb");
		if (error || !file)
			return;

		// Calculate the number of characters in the header
		headerOffset =
			2 +											// Magic number
			1 +											// Whitespace
			std::to_string(ppmFile.width).size() +		// Width
			1 +											// Whitespace
			std::to_string(ppmFile.height).size() +		// Height
			1 +											// Whitespace
			std::to_string(ppmFile.maxValue).size() +	// Max value
			1;											// Whitespace

		// Read away the header to move the file pointer
		buffer = new char[headerOffset];
		fread(buffer, 1, headerOffset, file);
		delete[] buffer;

		// Read all the pixel data
		nValues = ppmFile.width * ppmFile.height * 3;
		ppmFile.data = new unsigned char[nValues];
		bytesRead = fread(ppmFile.data, 1, nValues, file);
		fclose(file);

		if (bytesRead != nValues)
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
		infile.open(ppmFile.name, std::ifstream::in);
		if (!infile.is_open())
			return;

		// Read the magic number, width, height, and max value
		infile >> str >> str >> str >> str;

		// Each pixel has three values (RGB)
		const size_t N_VALUES = ppmFile.width * ppmFile.height * 3;
		ppmFile.data = new unsigned char[N_VALUES];

		// Read all the pixel data
		for (size_t i = 0; i < N_VALUES; i++)
		{
			short value;
			infile >> value;
			ppmFile.data[i] = static_cast<unsigned char>(value);
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
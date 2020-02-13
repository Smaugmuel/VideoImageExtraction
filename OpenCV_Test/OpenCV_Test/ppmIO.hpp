#pragma once
#include <string>
#include <stdlib.h>
#include <sstream>
#include <fstream>

namespace PPM
{
	struct PPMFile
	{
		std::string name;
		std::string magicNumber;
		size_t width;
		size_t height;
		size_t maxValue;
		unsigned char* data;
	};

	bool ReadBinary(PPMFile& ppmFile)
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
		{
			return false;
		}

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
			return false;
		}

		return true;
	}
	bool ReadPlain(PPMFile& ppmFile)
	{
		std::ifstream infile;
		std::string str;
		infile.open(ppmFile.name, std::ifstream::in);
		if (!infile.is_open())
			return false;

		// Read the magic number, width, height, and max value
		infile >> str >> str >> str >> str;

		// Each pixel has three values
		size_t nValues = ppmFile.width * ppmFile.height * 3;
		ppmFile.data = new unsigned char[nValues];

		// Read all the pixel data
		for (size_t i = 0; i < nValues; i++)
		{
			int value;
			infile >> value;
			ppmFile.data[i] = static_cast<unsigned char>(value);
		}

		infile.close();

		return true;
	}
	bool Read(PPMFile& ppmFile, const char* fileName)
	{
		ppmFile.name = fileName;

		// Read the file header
		std::ifstream infile;
		infile.open(fileName, std::ifstream::in);
		if (!infile.is_open())
			return false;
		infile >> ppmFile.magicNumber >> ppmFile.width >> ppmFile.height >> ppmFile.maxValue;
		infile.close();

		if (ppmFile.magicNumber == "P3")
		{
			return ReadPlain(ppmFile);
		}
		else if (ppmFile.magicNumber == "P6")
		{
			return ReadBinary(ppmFile);
		}

		return false;
	}

	bool WriteBinary(unsigned char* data, size_t width, size_t height, size_t maxValue, const char* fileName)
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
	void WritePlain(unsigned char* data, size_t width, size_t height, size_t maxValue, const char* fileName)
	{
		std::ofstream outFile;
		outFile.open(fileName);

		outFile << "P3\n" << width << "\n" << height << "\n" << maxValue << "\n";

		size_t nValues = width * height * 3ULL;
		for (size_t i = 0; i < nValues; i++)
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
	}
}
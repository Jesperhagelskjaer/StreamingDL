/*********************************************
** Digital Lynx SX, UDP Record format handler
** Created: 3/4 2017 by Kim Bjerge, AU
** Modified:
** 4/4 2017 Added creation of header file
**********************************************/
#pragma once
#include <cstdint>
#include <string>
#include <fstream>
using namespace std;
#include "Semaphore.h"

// Defines for Digital Lynx SX system, number of input boards and channels
#define NUM_BOARDS      1  // Number of input boards in Digital Lynx SX
#define NUM_CHANNELS   32  // Number of channels in input board
#define RESERVED_SIZE  10  // Reserved space in Lynx Record header

typedef struct LHEADER 
{
	uint32_t start;
	uint32_t packetId;
	uint32_t size;
	uint32_t timestampHigh;
	uint32_t timestampLow;
	uint32_t systemStatus;
	uint32_t ttlIO;
	uint32_t reserved[RESERVED_SIZE];
} LxHeader;

typedef struct LBOARD_DATA 
{
	int32_t data[NUM_CHANNELS];
} LxBoardData;

typedef struct LBOARDS_DATA
{
	LxBoardData board[NUM_BOARDS];
} LxBoardsData;

typedef struct LRECORD 
{
	LxHeader header;
	LxBoardData board[NUM_BOARDS];
	uint32_t checksum;
} LxRecord;

class LynxRecord 
{

public:
	LynxRecord () : semaWaitForData(1, 0)
	{
		// Clear lxRecord
		memset(&lxRecord, 0, sizeof(LxRecord));
		MemPoolSize = 0;
		pMemPoolStart = 0;
		pMemPoolReadPtr = 0;
		pMemPoolWritePtr = 0;
		RecordSize = sizeof(LxBoardsData)/sizeof(int32_t);
	};

	~LynxRecord ()
	{
		CloseFiles();
		ClearMemPool();
	};

	// -------------------------------------------------------------------------------------
	// Methods to handle lynx record data and storing records in memory pool
	// -------------------------------------------------------------------------------------

	// Returns char pointer to lxRecord and size of buffer
	int GetBuffer(char **pBuffer) 
	{
		*pBuffer = (char *)&lxRecord;
		return sizeof(LxRecord);
	}

	// Handling of memory pool
	// Allocates memory pool in size of samples (int)
	bool CreateMemPool(uint32_t size)
	{
		pMemPoolStart = (int32_t *)calloc(size, sizeof(int));
		MemPoolSize = size;
		ResetMemPtr();
		return (pMemPoolStart != 0);
	}

	void ClearMemPool(void)
	{
		if (pMemPoolStart != 0)
			free(pMemPoolStart);
	}

	// Set writing of samples to start of memory pool
	void ResetMemPtr(void)
	{
		pMemPoolWritePtr = pMemPoolStart;
		pMemPoolReadPtr = pMemPoolStart;
		cout << "Reset memory read/write pointers" << endl;
	}

	bool AppendDataToMemPool()
	{
		if (pMemPoolWritePtr == 0) return false;

		if ( pMemPoolWritePtr < (pMemPoolStart+MemPoolSize-RecordSize) )
		{
			//std::cout << "Timestamp low : " << lxRecord.header.timestampLow << endl;
			/*
			for (int j = 0; j < NUM_BOARDS; j++)
				for (int i = 0; i < NUM_CHANNELS; i++)
				{
					*pMemPoolWritePtr = lxRecord.board[j].data[i];
					pMemPoolWritePtr++;
				}
			*/
			memcpy(pMemPoolWritePtr, lxRecord.board, sizeof(LxBoardsData));
			pMemPoolWritePtr += RecordSize;

		    // Signal new data record in memory pool
			semaWaitForData.signal();
			return true;
		}

		if (isMemPoolEmpty()) ResetMemPtr(); // Reset write and read pointers if all records written to files
		return false;
	}

	// Computes and verifies checksum of record
	bool CheckSumOK(void) 
	{
		bool result = false;
		uint32_t *pStart = (uint32_t *)&lxRecord;
		uint32_t checksum = 0;
		for (int i = 0; i < sizeof(LxRecord)/sizeof(uint32_t); i++)
			checksum = checksum ^ pStart[i];
		if (checksum == 0) 
			result = true;
		return result;
	}

	void CreatTestData(int start)
	{
		lxRecord.header.packetId = 1;
		lxRecord.header.timestampHigh = 6789;
		lxRecord.header.timestampLow = 12345;
		lxRecord.header.ttlIO = 0x5A5A5A5A;
		lxRecord.header.systemStatus = 2222;

		for (int j = 0; j < NUM_BOARDS; j++)
			for (int i = 0; i < NUM_CHANNELS; i ++)
				lxRecord.board[j].data[i] = 10000*(j+1)+(start+i);
	}

	bool isMemPoolEmpty(bool print = false) 
	{
		if (print) 
			printf("Read 0x%08X - Write 0x%08X\r\n", pMemPoolReadPtr, pMemPoolWritePtr);
		return (pMemPoolReadPtr == pMemPoolWritePtr);
	}

	// -------------------------------------------------------------------------------------
	// Methods to handle writing lynx record data to files
	// -------------------------------------------------------------------------------------

	bool OpenChannelDataFiles(char *fileName)
	{
		bool result = true;
		char chFileName[255];
		CloseChannelDataFiles();
		for (int ch = 0; ch < NUM_CHANNELS; ch++)
		{
			sprintf(chFileName, "%s%02d.txt", fileName, ch);
			dataChStreams[ch].open(chFileName, ios::out | ios::trunc); // Truncate file 
			if (dataChStreams[ch].is_open() == false) {
				std::cout << "Unable to open file: " << chFileName << endl;
				result = false;
			}
		}
		return result;
	}

	bool OpenDataFile(char *fileName)
	{
		bool result = true;
		CloseDataFile(); // Close file if already opened
		//dataStream.open(fileName, ios::out | ios::app); // Append to end of file
		dataStream.open(fileName, ios::out | ios::trunc); // Truncate file 
		if (dataStream.is_open() == false) {
			std::cout << "Unable to open file: " << fileName << endl;
			result = false;
		}
		return result;
	}

	void AppendDataIntToFile()
	{
		if (dataStream.is_open()) 
		{
			//std::cout << "Timestamp low : " << lxRecord.header.timestampLow << endl;
			for (int j = 0; j < NUM_BOARDS; j++)
				for (int i = 0; i < NUM_CHANNELS; i++) {
					dataStream << lxRecord.board[j].data[i] << ", ";
				}
			dataStream << endl;
		}
	}

	void AppendMemPoolIntToFile()
	{
		if (dataStream.is_open())
		{
			semaWaitForData.wait();
			LxBoardsData *pBoardsData = (LxBoardsData *) pMemPoolReadPtr;
			LxBoardsData *pMemPoolEndPtr = (LxBoardsData *)(pMemPoolWritePtr-RecordSize);
			//std::cout << "Timestamp low : " << lxRecord.header.timestampLow << endl;
			while (pBoardsData <= pMemPoolEndPtr) {
				for (int j = 0; j < NUM_BOARDS; j++)
					for (int i = 0; i < NUM_CHANNELS; i++)
					{
						dataStream << pBoardsData->board[j].data[i] << ", ";
					}
				dataStream << endl;
				pBoardsData++;
				pMemPoolReadPtr = (int32_t *)pBoardsData;
			}
		}
	}

	void AppendMemPoolIntToChFiles()
	{
		if (dataChStreams[0].is_open())
		{
			semaWaitForData.wait();
			LxBoardsData *pBoardsData = (LxBoardsData *) pMemPoolReadPtr;
			LxBoardsData *pMemPoolEndPtr = (LxBoardsData *)(pMemPoolWritePtr-RecordSize);
			while (pBoardsData <= pMemPoolEndPtr) {
				for (int j = 0; j < NUM_BOARDS; j++)
					for (int i = 0; i < NUM_CHANNELS; i++)
					{
						dataChStreams[i] << pBoardsData->board[j].data[i] << endl;
					}
				pBoardsData++;
				pMemPoolReadPtr = (int32_t *)pBoardsData;
			}
		}
	}

	void AppendDataFloatToFile()
	{
		if (dataStream.is_open())
		{
			//std::cout << "Timestamp low : " << lxRecord.header.timestampLow << endl;
			for (int j = 0; j < NUM_BOARDS; j++)
				for (int i = 0; i < NUM_CHANNELS; i++) 
				{
					float data = (float)(lxRecord.board[j].data[i] / pow(2, 31));
					dataStream << data << ", ";
				}
			dataStream << endl;
		}
	}

	bool OpenHeaderFile(char *fileName)
	{
		bool result = true;
		CloseHeaderFile(); // Close file if already opened
		//headerStream.open(fileName, ios::out | ios::app); // Append to end of file
		headerStream.open(fileName, ios::out | ios::trunc); // Truncate file 
		if (headerStream.is_open() == false) 
		{
			std::cout << "Unable to open file: " << fileName << endl;
			result = false;
		}
		return result;
	}

	// Append record to header file with: timestampHigh, timestampHigh, ttlIO, systemStatus, 
	void AppendHeaderToFile()
	{
		if (headerStream.is_open())
		{
			//std::cout << "Timestamp high: " << lxRecord.header.timestampHigh << endl;
			headerStream << lxRecord.header.timestampHigh << ", ";
			headerStream << lxRecord.header.timestampLow << ", ";
			headerStream << lxRecord.header.ttlIO << ", ";
			headerStream << lxRecord.header.systemStatus << ", ";
			headerStream << endl;
		}
	}

	void CloseDataFile(void) 
	{
		if (dataStream.is_open() == true)
			dataStream.close();
	}

	void CloseChannelDataFiles(void) 
	{
		for (int ch = 0; ch < NUM_CHANNELS; ch++)
		{
			if (dataChStreams[ch].is_open() == true)
				dataChStreams[ch].close();
		}
	}

	void CloseHeaderFile(void) 
	{
		if (headerStream.is_open() == true)
			headerStream.close();
	}

	void CloseFiles(void) 
	{
		CloseDataFile();
		CloseHeaderFile();
		CloseChannelDataFiles();
	}

private:
	Semaphore semaWaitForData;
	ofstream dataStream;
	ofstream dataChStreams[NUM_CHANNELS];
	ofstream headerStream;
	LxRecord lxRecord;
	char dummy[10]; // Just for safety, could be removed
	uint32_t RecordSize;
	uint32_t MemPoolSize;
	int32_t *pMemPoolReadPtr;
	int32_t *pMemPoolWritePtr;
	int32_t *pMemPoolStart;
};

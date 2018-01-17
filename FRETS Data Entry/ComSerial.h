/*
	Serial Communication Handler
*/

#include <stdint.h>

#pragma once
class ComSerial
{

public:
	enum eCommand
	{
		cmdBeginMatch	= 'B',
		cmdEndMatch		= 'E',
		cmdDumpResults	= 'D'
	};

private:
	static uint16_t crc_16_table[16];

	const char START_CHAR = ':';

	enum eState
	{
		RM_Idle			= 0,
		RM_WaitOnID		= 1,
		RM_WaitOnCMD	= 2,
		//RM_WaitOnLength = 3,
		//RM_WaitOnData	= 4,
		//RM_WaitOnCRC0	= 5,
		//RM_WaitOnCRC1	= 6,
		//RM_Execute		= 7,
		//RM_WaitOnReturn = 8
	};

	eState	m_state;

	int		m_boardIdChar;

	// binary protocol control
	// 
	uint8_t bpMode;
	uint8_t bpLength;
	char bpCommand;
	char bpData[255];
	int  bpIndex;
	unsigned int bpCRC;

public:
	ComSerial(int boardID);
	~ComSerial();

	eCommand chkMsg();
	bool chkCRC(void);
	void sendCRC(void);
	void addCRC(uint8_t btVal);
	void addCRC2(uint8_t btVal);

};


#include "ComSerial.h"

uint16_t ComSerial::crc_16_table[16] =
{
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};



ComSerial::ComSerial(int boardID)
{
	if (boardID < 1 || boardID > 9)
		return;

	m_boardIdChar = '0' + boardID;
}


ComSerial::~ComSerial()
{
}



ComSerial::eCommand ComSerial::chkMsg()
{
	//while (Serial.available())
	//{
	//	switch (m_state)
	//	{
	//	case RM_Idle:
	//		break;
	//	case RM_WaitOnID:
	//		break;
	//	case RM_WaitOnCMD:
	//		break;

	//	default:
	//		break;
	//	}
	//}
}

bool ComSerial::chkCRC(void)
{
	bpCRC = 0xffff;       // start value of CRC
	int i = 2;              // skip command and length
	while (i < bpIndex)   // CRC the rest
		addCRC(bpData[i]);

	return bpCRC == 0;    // CRC including the CRC bytes should be zero
}

void ComSerial::sendCRC(void) 
{
//	Serial.write(lowByte(bpCRC));
//	Serial.write(highByte(bpCRC));
}

void ComSerial::addCRC(uint8_t btVal)
{
	bpCRC ^= btVal;
	for (int i = 0; i<8; i++)
	{
		uint8_t lsb = bpCRC & 0x0001;
		bpCRC >>= 1;
		if (lsb == 1)
			bpCRC ^= 0xa001;
	}
}

// good article: http://www.lammertbies.nl/comm/info/crc-calculation.html
// this implementation: http://darkridge.com/~jpr5/archive/alg/node191.html
//
void ComSerial::addCRC2(uint8_t btVal)
{
	unsigned int r;
	/* compute checksum of lower four bits of bVal */
	r = crc_16_table[bpCRC & 0xF];
	bpCRC = (bpCRC >> 4) & 0x0FFF;
	bpCRC = bpCRC ^ r ^ crc_16_table[btVal & 0xF];

	/* now compute checksum of upper four bits of *p */
	r = crc_16_table[bpCRC & 0xF];
	bpCRC = (bpCRC >> 4) & 0x0FFF;
	bpCRC = bpCRC ^ r ^ crc_16_table[(btVal >> 4) & 0xF];
}




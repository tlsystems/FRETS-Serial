//
// FRETS-Serial 
//	First Robotics Evaluation & Tracking System
//	Serial interface to host computer
//
//

#define DEBUG
//#define DEBUG_DEBOUNCE

#include "ComSerial.h"


// array of pin number for data entry buttons
const int pinButtons[] = { 9,8,7,6,5,4,3,2 };
// array of pin number for data entry board ID
const int pinID[] = { 12,11,10 };

const int pinBUZZER = A4;
const int pinLED = A5;

const int DEBOUNCE_TIME = 10;

byte  g_btBoardID;			// board ID

// button state
byte  g_btRawBTNS;			// actual state of buttons
byte  g_btDebounceTimer[8];	// debounce timer for buttons
byte  g_btBTNS;				// debounced state of buttons
byte  g_btLastBTNS;			// state of buttons after last scan

typedef struct {
	uint8_t		btnID;		// button identifier (0-7)
	uint8_t		eventID;	// event (0-button release, 1-button press)
	uint16_t	time;		// centiseconds (1/100) from start of match
} sEvent;

sEvent  g_eventArray[1000];
int		g_eventIndex;
bool	g_bGameInProcess;

ComSerial* g_pSerialPort;

HardwareSerial* g_pDebugPort;
HardwareSerial* g_pComPort;


//**********************************************************************
//  Initialize Hardware
//
void setup()
{
	// data entry buttons and board ID are all inpuits with pullup enabled
	for (int i = 0; i<8; i++)
		pinMode(pinButtons[i], INPUT_PULLUP);
	for (int i = 0; i<3; i++)
		pinMode(pinID[i], INPUT_PULLUP);

	pinMode(pinLED, OUTPUT);
	pinMode(pinBUZZER, OUTPUT);
	digitalWrite(pinLED, HIGH);

	// read board ID
	g_btBoardID = digitalRead(pinID[0]) << 0 |
				  digitalRead(pinID[1]) << 1 |
				  digitalRead(pinID[2]) << 2;

	g_pSerialPort = new ComSerial(g_btBoardID);

	g_pDebugPort = &Serial1;
	g_pComPort = &Serial;

	// enable serial port
	#if defined(DEBUG)
	g_pDebugPort->begin(115200);
	g_pDebugPort->println("Hello World");
	#endif
}

//**********************************************************************
//  Main Processing Loop
//
void loop()
{
	ProcessSerialCommands();

//	if (g_bGameInProcess)
	{
		ScanIO();			// read IO
		DebounceButtons();	// debounce buttons

		DebugButtons();
		//ProcessButtons();
	}


}



//**********************************************************************
//  Read buttons 
//
//	The actual raw state of the each button is inverted and consolidated
//	into one byte. The raw state is inverted becase the buttons are
//	wired in a low-true manner i.e. they pull the input to ground. 
//	The internal pullup resistor on each input is enabled to pull
//	the input high when the button is not pressed.
//
void ScanIO(void)
{
	g_btRawBTNS = 0;
	//
	for (int i = 0; i<8; i++)
	{
		// OR inverted & shifted input state into RawBTNS
		g_btRawBTNS |= !digitalRead(pinButtons[i]) << i;
	}
}


//********************************************************************
// DebounceButtons
//
//  This routine eliminates spurious changes on a button input if the 
//  contact inside the button bounces before settling. A timer is started
//  when a button input first changes and any subsequent changes will be
//  ignored until the timer has timed out. Each button has its own timer.
//  The timeout value can be adjusted for noisy buttons, but most button
//  switches will settle within 10 mSec.
//
void DebounceButtons(void)
{
	static uint32_t lastMillis;
	static byte btLastRawBTNS;

	// determine how many mSecs have passed since last call
	uint32_t currentMillis = millis();
	uint32_t deltaMillis = currentMillis - lastMillis;

	// return if delta < 1 mSec
	if (deltaMillis == 0)
		return;

	// determine the bits that have changed
	byte btChanged = g_btRawBTNS ^ g_btBTNS;

	// update debounce timeouts
	for (byte i = 0; i<8; i++)
	{
		byte bitMask = 0x01 << i;
		#if defined(DEBUG_DEBOUNCE)
		g_pDebugPort->print(bitMask, HEX);
		#endif

		// check next bit's debounceTimer
		if (g_btDebounceTimer[i] > deltaMillis)
		{
			// still active, update and continue
			g_btDebounceTimer[i] -= deltaMillis;
			#if defined(DEBUG_DEBOUNCE)
			g_pDebugPort->print(" debouncing ");
			g_pDebugPort->println(g_btDebounceTimer[i]);
			#endif
			continue;
		}
		else if (g_btDebounceTimer[i] > 0)
		{
			// debounce timed out
			g_btDebounceTimer[i] = 0;
			#if defined(DEBUG_DEBOUNCE)
			g_pDebugPort->print(" zero timer");
			#endif
			if (btChanged & bitMask)
			{
				if ((g_btRawBTNS & bitMask) == 0)
				{
					#if defined(DEBUG_DEBOUNCE)
					g_pDebugPort->print(" clear bit ");
					#endif
					g_btBTNS &= ~bitMask;
				}
				else
				{
					#if defined(DEBUG_DEBOUNCE)
					g_pDebugPort->print(" set bit ");
					#endif
					g_btBTNS |= bitMask;
				}
			}
		}
		else
		{
			// start debounce timer
			if (btChanged & bitMask)
				g_btDebounceTimer[i] = DEBOUNCE_TIME;
		}
	}
	btLastRawBTNS = g_btBTNS;
}

void DebugButtons()
{
	static byte btLastBTNS = 0;
	static byte btLastRAW = 0;
	uint32_t curMillis = millis();

	if (g_btBTNS != btLastBTNS)
	{
		g_pDebugPort->print("BTN ");
		g_pDebugPort->print(g_btBTNS, HEX);
		g_pDebugPort->print(" @ ");
		g_pDebugPort->println(curMillis);
	}

	if (g_btRawBTNS != btLastRAW)
	{
		g_pDebugPort->print("RAW ");
		g_pDebugPort->print(g_btRawBTNS, HEX);
		g_pDebugPort->print(" @ ");
		g_pDebugPort->println(curMillis);
	}

	btLastBTNS = g_btBTNS;
	btLastRAW  = g_btRawBTNS;
}

//**********************************************************************
//  Process button presses
//
void ProcessButtons()
{
	static byte btLastBTNS = 0;
	byte btChanged = btLastBTNS ^ g_btBTNS;
	uint32_t time = (millis() - g_matchStart)) / 10;

	byte bitMask = 0x01;
	for (int i = 0; i < 8; i++)
	{
		if ((bitMask & btChanged) != 0)
		{
			g_eventArray[g_eventIndex].btnID = i;
			g_eventArray[g_eventIndex].eventID = ((g_btBTNS & bitMask) != 0) ? 'P' : 'R';
			g_eventArray[g_eventIndex].time = i;
		}
		
	}
	return;
}

//**********************************************************************
//  Process Serial Commands
//
void ProcessSerialCommands()
{
	g_pSerialPort->chkMsg();
}


void printButtonData()
{
	g_pDebugPort->print(" ID:");
	g_pDebugPort->print(g_btBoardID, HEX);
	g_pDebugPort->print(" 0:");
	g_pDebugPort->print(g_btBTNS & 0x01 ? 1 : 0);
	g_pDebugPort->print(" 1-");
	g_pDebugPort->print(g_btBTNS & 0x02 ? 1 : 0);
	g_pDebugPort->print(" 2-");
	g_pDebugPort->print(g_btBTNS & 0x04 ? 1 : 0);
	g_pDebugPort->print(" 3-");
	g_pDebugPort->print(g_btBTNS & 0x08 ? 1 : 0);
	g_pDebugPort->print(" 4-");
	g_pDebugPort->print(g_btBTNS & 0x10 ? 1 : 0);
	g_pDebugPort->print(" 5-");
	g_pDebugPort->print(g_btBTNS & 0x20 ? 1 : 0);
	g_pDebugPort->print(" 6-");
	g_pDebugPort->print(g_btBTNS & 0x40 ? 1 : 0);
	g_pDebugPort->print(" 7-");
	g_pDebugPort->println(g_btBTNS & 0x80 ? 1 : 0);
}

void flashLED(int count)
{
	for (int i = 0; i < count; i++)
	{
		digitalWrite(pinLED, HIGH);
		delay(50);
		digitalWrite(pinLED, LOW);
		delay(100);

	}
}
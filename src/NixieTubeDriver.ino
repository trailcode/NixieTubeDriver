#include <Arduino.h>
#include <Wire.h>
#include <digitalWriteFast.h>  // library for high performance reads and writes by jrraines
                               // see http://www.arduino.cc/cgi-bin/yabb2/YaBB.pl?num=1267553811/0
                               // and http://code.google.com/p/digitalwritefast/

int myLatchPin = 8;  // Pin connected to ST_CP of 74HC595
int myClockPin = 12; // Pin connected to SH_CP of 74HC595
int myDataPin = 11;  // Pin connected to DS of 74HC595

int ID = 0;

void setup()
{
  //set pins to output so you can control the shift register
  pinMode(myLatchPin,   OUTPUT);
  pinMode(myClockPin,   OUTPUT);
  pinMode(myDataPin,    OUTPUT);

  Serial.begin(9600);

  // Start the I2C Bus as Slave on address 9
  Wire.begin(9);
  Wire.onReceive(receiveEvent);

  for(int pin = 4; pin < 8; ++pin)
  {
	 pinMode(pin, INPUT);
	 digitalWrite(pin, HIGH);
  }

	ID =  ~(digitalRead(4) << 0 |
			  digitalRead(5) << 1 |
			  digitalRead(6) << 2 |
			  digitalRead(7) << 3) & 0xF;

	Serial.println("Ready :) ID: " + String(ID));
}

void p(char * buf, char *fmt, ... )
{
  va_list args;
  va_start (args, fmt );
  vsnprintf(buf, 512, fmt, args);
  va_end (args);
}

char receiveEventBuf[128];
int receiveEventindex = 0;

void completeBuffer()
{
  receiveEventBuf[receiveEventindex] = 0;

  receiveEventindex = 0;
}

long numberToDisplay = 0;

void receiveEvent(int bytes)
{
  int incommingID = 0;

  while(Wire.available())
  {
    // read the incoming byte:
    char incomingByte = Wire.read();
    //Serial.println(incomingByte);
    if(incomingByte == 'i')
    {
      completeBuffer();

      incommingID = atoi(receiveEventBuf);
    }
    else if(incomingByte == 'p')
    {
      completeBuffer();

		if(incommingID == ID)
		{
			numberToDisplay = min(99999, abs(atoi(receiveEventBuf)));
		}
    }
	 else if(incomingByte == 'a')
	 {
		 completeBuffer();

		 char buf[16];

		 p(buf, "%iI", ID);
		 Serial.println(buf);
		 Wire.beginTransmission(9); // transmit to device #9
		 Wire.write(buf);
		 Wire.endTransmission();
	 }
	 else if(incomingByte == 'I')
	 {
		 completeBuffer();
		 Serial.println("echo!");
		 Serial.println(" ID: " + String(atoi(receiveEventBuf)));
	 }
    else
    {
      receiveEventBuf[receiveEventindex++] = (char)incomingByte;
    }
  }
}

// the heart of the program
void shiftOut(byte myDataOut, bool invert = false)
{
  // This shifts 8 bits out MSB first,
  //on the rising edge of the clock,
  //clock idles low

  //internal function setup
  int i=0;
  int pinState;
  pinMode(myClockPin, OUTPUT);
  pinMode(myDataPin, OUTPUT);

  //clear everything out just in case to
  //prepare shift register for bit shifting
  digitalWrite(myDataPin, 0);
  digitalWrite(myClockPin, 0);

  //for each bit in the byte myDataOut�
  //NOTICE THAT WE ARE COUNTING DOWN in our for loop
  //This means that %00000001 or "1" will go through such
  //that it will be pin Q0 that lights.
  if(!invert)
  {
    for (i=7; i>=0; i--)  {
      digitalWrite(myClockPin, 0);
      if ( myDataOut & (1<<i) ) { pinState= 1 ;}
      else { pinState= 0; }
      digitalWrite(myDataPin, pinState);
      digitalWrite(myClockPin, 1);
      digitalWrite(myDataPin, 0);
    }
  }
  else
  {
    for (i=0; i < 8; i++)
    {
      digitalWrite(myClockPin, 0);
      if ( myDataOut & (1<<i) ) { pinState= 1 ;}
      else { pinState= 0; }
      digitalWrite(myDataPin, pinState);
      digitalWrite(myClockPin, 1);
      digitalWrite(myDataPin, 0);
    }
  }

  digitalWrite(myClockPin, 0);
}

char serialBuffer[256];
bool serialBufferReady = false;
int serialBufferCounter = 0;

void handleSerial()
{
   bool echoCommands = true;

   if (serialBufferReady)
   {
      serialBufferReady = false;

      char * command = strtok(serialBuffer, " \r\n");    // Everything up to the '=' is the color name

      if(!strcmp(command, "help") || !strcmp(command, "?"))
      {
         Serial.println("Help:");
         Serial.println();
			Serial.println("ID Get device ID.");
			Serial.println("devices List all I2C device IDs");
      }
      else if(!strcmp(command, "n"))
      {
         numberToDisplay = atol(strtok(NULL, " \r\n"));

         //getNixiesValues(numToDisplay, currNixie);

         if(echoCommands)
         {
            Serial.print("Set number ");
            //LL2Serial(numToDisplay);
				Serial.print(numberToDisplay);
            Serial.println();
         }
      }
		else if(!strcmp(command, "ID"))
		{
			Serial.println("ID: " + String(ID));
		}
		else if(!strcmp(command, "devices"))
		{
			Wire.beginTransmission(9); // transmit to device #9
			char buf[16];
			p(buf, "a");
			Wire.write(buf);
			Wire.endTransmission();
		}
   }
   else while (Serial.available())
   {
      const char c = Serial.read();

      serialBuffer[serialBufferCounter++] = c;

      if ((c == '\n') || (serialBufferCounter == sizeof(serialBuffer)-1))
      {
         serialBuffer[serialBufferCounter] = '\0';

         serialBufferCounter = 0;

         serialBufferReady = true;
      }
   }
}

void loop()
{
  handleSerial();

  digitalWrite(myLatchPin, LOW);

  long num = numberToDisplay;

  long a = num % 10; num /= 10;
  long b = num % 10; num /= 10;
  long c = num % 10; num /= 10;
  long d = num % 10; num /= 10;
  long e = num % 10; num /= 10;

  const byte map1[] = {8, 9, 2, 0, 1, 5, 3, 7, 6, 4};
  const byte map2[] = {0, 2, 9, 8, 3, 7, 6, 4, 5, 1};

  shiftOut(map2[d] | map2[e] << 4, false);
  shiftOut(map2[b] | map2[c] << 4, false);
  shiftOut(map1[a]);

  digitalWrite(myLatchPin, HIGH);
}

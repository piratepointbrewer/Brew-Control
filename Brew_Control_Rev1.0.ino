#include <hd44780.h>

#include <SoftwareWire.h>

#include <neotimer.h>


/*Brew Control Rev 1.0

===========================================================

Rev 1.0 - Digital control of Pump and Heater Control SSR
via UNO/Nano. Two level sensors will monitor liquid level
in BK via "Upper" and "Lower" level sensors.
Sensor will be place to control liquid level
within 1 gallon generally placed with "Upper"
at the 9 gallon level and "Lower at the 8 gallon level.
Sensor elements are connected to Uno/Nano via- 74HC00
Quad 2 input NAND. Common element for all sensors is
circuit common and  can be the BK vessel. Logic  output
"HIGH" indicates the presence of liquid. One gate input
on each is tied to VCC. The sensor input is tied to VCC
via a 470K resistor to be pulled low by liquid forcing
the gate output to be "HIGH" to the processor.
Program Logic will be:
"Upper" AND "Lower" = "HIGH"   Pump On
"Upper" AND "Lower" = "LOW"  Pump Off
PumpPID and HeatPID monitor the output of the SYL-2362 PID
Temperature Controllers curruently used and have presidence.
The signal from the HeatPID must be "HIGH" for this Logic to
turn on the HeatSSR. Similarly the PumpPID used for cooling
must be "HIGH" for the Micro controller to operate the pump.
Future will incorporate a third sensor at the 6 gallon level
to overide the HeatPID and shut off Heat if the liquid level
falls below the 6 gallon level as the temp sensor will
loose contact with the liquid and the PID will call for full
uncontrolled heat.
Default and Error Ouput condition is Heat Off and Pump Off.
Rev 2.0 - Debug 1.0 and add new level probe to BK using the
BK as common for probe.
Also added 6 and 12 gallon probes. The "Below" 6 gal auto
cuts off HeatPID and can signify End of Strike condition.
12 gal level is a warning not to overfill BK when draining
MT to start Boil.
Added Neotimer.h  and created Blink Function to Blink as
Pump turned on.

Rev 2.1 - Writing code as function getting ready to use
Timers to call at regular intervals. Will select a
Non-Blocking timer library

Rev 2.2 & 2.3 - Added lcd to monitor functions and learn how.
NOTE: Using "SoftwareWire" library to reassign I2C pins to
D2 and D3. Using hd44780.h libraty for LCD as it uses avove
SoftwareWire for I2C communication. Also include
hd44780ioClass/hd44780_I2Cexp.h. Timer work still to be
done but may incorporate here.

Rev 2.4
Uses BkFull, now called BoilOver, probe to kill HSSR for
5 sec to prevent boil over.   Change analog pin for key
pad from A7 used on NANO debug to A0 on UNO to prevent
possible conflict with I2C defaut pins A4 and A5. Key pad
scheme uses a 5 step voltage divider passed via
Normally Open pushbuttons to analog A0 and a 1 meg pull up.
Each step is 1 volt on divider including 0 vdc. Steps are:
0 vdc = "Back", 1 vdc = "Forward", 2 vdc = "Increase", 3 vdc
= "Decrease", 4 vdc = "Select", 5 vdc used as NO KEY. Scheme
is set up to use analogReference(EXTERNAL) and system
V++ to AREF.
Rev 2.5 Incorporate GetVal and ParseVal.  Start pullin
the Brew Control together so will save all combined files &
functions into Brew Contro Rev 1.0
*/
// ============ LIBRARIES ===================================


Neotimer BlinkTime(0); // Must have a value even just 0
Neotimer boilKilltime(5000); // Boil over kill time
Neotimer mtDrnTime(120000); // 2 min drain time in recycle
Neotimer bkPmpTime(45000); // 45 sec pmp to mt in recycle
Neotimer DmashTime(3600000); // Default mash time, 60 min
Neotimer DmshoutTime(1200000); // Default Mash Out time = 20 min
Neotimer DboilTime(3600000); // Default boil time is 60 min
/*Neotimer dpyRefTime (0);
Neotimer dpyRefTime (0);
Neotimer dpyRefTime (0);
Neotimer dpyRefTime (0);
Neotimer dpyRefTime (0);
Neotimer i_oRefTime(0);
Neotimer OddTime (0);
*/
// Library and needed construct data for I2C on pin 2 & 3

// make pins sda & scl for LCD
const int Sclpin = 2;
const int Sdapin = 3;
SoftwareWire Wire(Sdapin, Sclpin);
// Library and construct data for LCD display
 // main hd44780 header
// i2c expander i/o class header
#include <hd44780ioClass/hd44780_I2Cexp.h>
// declare lcd object  & address, auto config expander chip
hd44780_I2Cexp lcd(0x20);
// ============== FUNCTION DECLARATI0NS  ====================
void SetUp();
void Run();
void Strike();
void Mash();
void MashOut();
void Boil();
void Cool();
void Blink();
void TestPIDs();
void TestLevels(int c, int d, int e, int f);
void GetKey();
int GetVal();
void ParseVal();
// =============== CONSTANTS =================================
const int SerialRxpin = 0;
const int SerialTxpin = 1;
// const int Sclpin    = 2;  // repeated for clarity
// const int Sdapin    = 3;  // repeated for clarity
// Constants for Level Control
const int UpperPin = 4;
const int LowerPin = 5;
const int SixGalPin = 6;
const int BoilOverPin = 7;
const int BlinkPin = 12;
const int LedPin = LED_BUILTIN; // pin 13
// Constants for Heat Control
const int HeatPID = 8;
const int PumpPID = 9;
const int PumpSSR = 10;
const int HeatSSR = 11;
// LCD geometry
const int LCD_COLS = 20;
const int LCD_ROWS = 4;
// Constanst to Select Operational Modes
/*
const int Home              = 0;
const int Run               = 1;
const int PreH              = 4;
const int Strk              = 5;
const int Mash              = 6;
const int MshO              = 7;
const int Boil              = 8;
const int Co0l              = 9;
*/
// Displat Constants for Debug Level & Heat Control
const String Bk = "Bkfl ";
const String Sx = "6gal ";
const String Hp = "Hpid ";
const String Hs = "Hssr ";
const String Lo = "Loer ";
const String Up = "Uper ";
const String Pp = "Ppid ";
const String Ps = "Pssr ";
const String On = "On  ";
const String Of = "Off ";
// Display Constants for Operational Modes
const String Mod = "Mode ";
const String StUp = "SetUp ";
const String Ru = "Run ";
const String Pr = "Program ";
const String St = "Strike ";
const String Ma = "Mash ";
const String MaO = "Mash Out ";
const String Bl = "Boil ";
const String Cl = "Cool ";
const String Ppb = "Pirate Point Brewery ";
// Display Constants for  GetVal
const String Dpynums = "Numbers = ";
const String ValueIn = "Value in = ";
const String ValueOut = "Value out = ";
const String Time = "Time = ";
const String Point = "^";
const String Blank = " ";
// Display Constants for GetKey
const int alogKeypin = A0;
const String Bak = "<-"; // Returned keyVal = 1
const int Ba = 1;
const String Fwd = "->"; // Returned keyVal = 2
const int Fd = 2;
const String Inc = "+"; // Returned keyVal = 3
const int In = 3;
const String Dec = "-"; // Returned keyVal = 4
const int De = 4;
const String Sel = "@"; // Returned keyVal = 5
const int Se = 5;
// ============ GLOBAL VARIABLES ============================
// Global Variables for Main Loop
volatile int Mode = 0;
// Global Variables for Level & Heat control
volatile int SixGalLevel = LOW; // Kill HeatSSR  below level
volatile int BoilOver = LOW; // Kill boil at/above
volatile int AboveSix = LOW;
volatile int HeatPIDon = LOW;
volatile int HeatOn = LOW; //
volatile int BoilKill = LOW;
volatile int UpperLevel = LOW; // no liquid detected
volatile int LowerLevel = LOW; // As above
volatile int PumpPIDon = LOW;
volatile int PumpRun = LOW;
volatile int BeepBlink = LOW; // Beep Blink State
volatile int BlinkCount = 3;
// Global Variables for GetKey
volatile int keyFlag = LOW;
volatile int keyVal = 0;
volatile int lastKey;
volatile String keyStr;
// Global Variables for GetVal
volatile int Drow = 2;
volatile int Didx = 0; // pointer to Valarray display position
volatile int Dmin = 10;
volatile int Dmax = 12;
volatile int Validx = 0; // pointer to valarray
volatile int Valmin = 0;
volatile int Valmax = 2;
volatile int Icol = 0;
volatile int Irow = 0;
volatile int Valarray[4]
{
  0, 0, 0
};

// Global Variables for  ParseVal
volatile int inputVal = 165;
volatile int outputVal = 0;
// Default Tempseatures and Times
volatile int STtp = 160; // strike temp = 160*F
volatile int MAtp = 155; // mash temp = 155*F
volatile int MAtm = 075; // mash time = 75 min
volatile int MOtp = 168; // mash out temp = 168*F
volatile int MOtm = 020; // mash out time = 20 min
volatile int BLtm = 075; // boil max time
volatile int COtp = 072; // cool down temp = 72*F
// Define Global Pointers
int * p;
int * p1;
int * p2;
int * p3;
// ++++++++++++ SETUP RUN ONCE ++++++++++++++++++++++++++++++
void setup()
{
  // initialize  serial port for debug
  Serial.begin(9600);
  while (!Serial)
  {
    // wait on serial port
  }
  // Intialize analog pin reference to External
  // analogReference (EXTERNAL);
  // pins 0 & 1 are not defined and remain default here
  pinMode(UpperPin, INPUT_PULLUP);
  pinMode(LowerPin, INPUT_PULLUP);
  pinMode(SixGalPin, INPUT_PULLUP);
  pinMode(BoilOverPin, INPUT_PULLUP);
  pinMode(PumpPID, INPUT);
  pinMode(HeatPID, INPUT);
  pinMode(PumpSSR, OUTPUT);
  pinMode(HeatSSR, OUTPUT);
  pinMode(Sclpin, OUTPUT);
  pinMode(Sdapin, OUTPUT);
  pinMode(BlinkPin, OUTPUT);
  pinMode(LedPin, OUTPUT);
  // insure inactive starting state
  digitalWrite(LedPin, LOW);
  digitalWrite(PumpSSR, LOW);
  digitalWrite(HeatSSR, LOW);
  digitalWrite(BlinkPin, LOW);
  // initialize display
  lcd.begin(LCD_COLS, LCD_ROWS);
  lcd.clear();
  lcd.noAutoscroll();
  lcd.lineWrap();
  delay(2000);
  int Col = 0;
  int Row = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(Ppb);
  lcd.setCursor(0, 2);
  lcd.print("Fill BK '@' = Start");
  lcd.setCursor(9, 3);
  lcd.cursor();
  // lcd.blink ();
  while (keyVal != Se)
  {
    GetKey();
  }
}

// +++++++++++++++ MAIN LOOP ++++++++++++++++++++++++++++++++
void loop()
{
  
  // Call  Test PIDs Function
  TestPIDs();
  // Call TestLevels Function
  TestLevels();
  // Set outputs  Heat, Pump
  if (AboveSix == HIGH
  && HeatPIDon == HIGH
  && BoilKill == LOW)
  {
    digitalWrite(HeatSSR, HIGH); // turn Heat on
  }
  else
  {
    digitalWrite(HeatSSR, LOW); // turn heat off
  }
  lcd.setCursor(0, 3);
  String tempH1 = Hs;
  String tempH2 = tempH1 += digitalRead(HeatSSR);
  lcd.print(tempH2);
  if (PumpPIDon == HIGH && PumpRun == HIGH)
  {
    digitalWrite(PumpSSR, HIGH); // Turn Pump on
    if (BlinkTime.repeat(6, 100))
    {
      digitalWrite(BlinkPin, !digitalRead(BlinkPin));
    }
  }
  else
  {
    if (PumpPIDon == LOW || PumpRun == LOW)
    {
      digitalWrite(PumpSSR, LOW);
      digitalWrite(BlinkPin, LOW);
      BlinkTime.repeatReset();
    }
  }
  lcd.setCursor(10, 3);
  String tempP1 = Ps;
  String tempP2 = tempP1 += digitalRead(PumpSSR);
  lcd.print(tempP2);
}

// ++++++++++++ MAIN LOOP END ++++++++++++++++++++++++++++++


// ************ FUNCTION CODE ************************

// ------------- Home Mode  -------------------------------
void Home()
{
  return;
}

// return;
// }
// -------------    Manual Mode  ----------------------------
void SetUp()
{
  return;
}

// -------------     Auto Mode  -----------------------------
void Run()
{
  return;
}

// ------------- Program Mode  ------------------------------
void Program()
{
  return;
}

// ------------- Pre Heat Mode  -----------------------------
void Preheat()
{
  return;
}

// ------------- Strike Mode  -------------------------------
void Strike()
{
  return;
}

// ------------- Mash Mode  ---------------------------------
void Mash()
{
  return;
}

// ------------- Mash Out Mode  -----------------------------
void MashOut()
{
  return;
}

// ------------- Boil Mode  ---------------------------------
void Boil()
{
  return;
}

// ------------- Cool Mode  --------------------------------
void Cool()
{
  return;
}

// -------------- TEST PIDs HEAT/PUMP ----------------------
void TestPIDs()
{
  // State of Heat PID
  if (digitalRead(HeatPID) == HIGH)
  {
    HeatPIDon = HIGH; // Heat temp control calls for heat
  }
  else
  {
    HeatPIDon = LOW;
  }
  /*
  lcd.setCursor(0, 2);
  String tempHp1 = Hp;
  String tempHp2 = tempHp1 += digitalRead(a);
  lcd.print(tempHp2);
  */
  // State of Pump PID
  if (digitalRead(PumpPID) == HIGH)
  {
    PumpPIDon = HIGH; // Heat temp control calls for heat
  }
  else
  {
    PumpPIDon = LOW;
  }
  /*
  lcd.setCursor(10, 2);
  String tempPp1 = Pp;
  String tempPp2 = tempPp1 += digitalRead(b);
  lcd.print(tempPp2);
  */
  return;
}

// -------------------------------------------------------------
// Test Liquid Levels
void TestLevels()
{
  UpperLevel = digitalRead(UpperPin);
  LowerLevel = digitalRead(LowerPin);
  SixGalLevel = digitalRead(SixGalPin);
  // Boil Over??
  BoilOver = digitalRead(BoilOverPin);
  if (BoilKill == LOW && BoilOver == HIGH)
  {
    BoilKill = HIGH;
    boilKilltime.start();
  }
  if (boilKilltime.done())
  {
    boilKilltime.stop();
    boilKilltime.reset();
    BoilKill = LOW;
  }
  lcd.setCursor(0, 0);
  String tempB1 = Bk;
  String tempB2 = tempB1 + digitalRead(BoilOverPin);
  lcd.print(tempB2);
  // Above Six
  if (SixGalLevel == HIGH)
  {
    AboveSix = HIGH; // level is above 6 gal
  }
  else
  {
    AboveSix = LOW; // level is below 6 gal
  }
  // At/Above Upper & Lower level
  if (UpperLevel == HIGH && LowerLevel == HIGH)
  {
    PumpRun = HIGH; // indicates level is high
  }
  if (UpperLevel == LOW && LowerLevel == LOW)
  {
    PumpRun = LOW; // indicates level is low
  }
  return;
}

// ---------------------------------------------------------
// Get Key Value
void GetKey()
{
  int val = analogRead(alogKeypin); // read the input pin
  delay(50);
  // Serial.println(val);
  if (val >= 920)
  {
    keyFlag = LOW;
  }
  else
  {
    if (val < 920 && keyFlag == HIGH)
    {
      keyVal = 0; // exit
    }
    else
    {
      if (val >= 0 && val <= 50)
      {
        keyVal = 1; // Is Ba = Back "<-"
      }
      if (val >= 200 && val <= 250)
      {
        keyVal = 2; // Is Fd = Forward "->"
      }
      if (val >= 400 && val <= 500)
      {
        keyVal = 3; // Is In = Tncrement
      }
      if (val >= 600 && val <= 750)
      {
        keyVal = 4; // Is De = Decrement
      }
      if (val >= 800 && val <= 900)
      {
        keyVal = 5; // Is Se = Select "@"
      }
      keyFlag = HIGH;
    }
  }
  return;
}

// --------------------------------------------------
// ParseVal
void ParseVal()
{
  // put your main code here, to run repeatedly:
  float result = 0;
  int a = 0; // will hold 100's
  int b = 0; // will hold 10's
  int c = 0; // will hold 1's
  int rem = 0; // will hold remainders
  int Value = inputVal;
  // ----------------- 100's  --------------------
  result = Value / 100.000; // =1.65
  a = result; // = 1
  // ------------------  10's  ------------------
  rem = a * 100.00; // = 1 * 100
  result = result * 100; // 1.65 * 100 = 165
  result = result - rem; // 165 - 100 = 65
  result = result / 10.00; // 65 / 10 = 6.5
  b = result; // = 6
  rem = b * 10.00; // 60
  // ----------------  1's  -------------------------
  result = result * 10; // 6.5 * 10 = 65
  c = result - rem; // 65 - 60 = 5
  Validx = Valmin;
  Valarray[Validx] = a;
  Validx = Validx + 1;
  Valarray[Validx] = b;
  Validx = Validx + 1;
  Valarray[Validx] = c;
}

// --------------- END FUNCTIONS ---------------------------

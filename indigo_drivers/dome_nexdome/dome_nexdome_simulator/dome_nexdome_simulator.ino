//#include <EEPROM.h>

#ifdef ARDUINO_SAM_DUE
#include <avr/dtostrf.h>
#define Serial SerialUSB
#endif

/*
 *  This is NexDome simulator based on the original firmware by Gerry Rozema
 *  https://github.com/grozzie2/NexDome
 *  
 *  The code is not clean as there are many leftovers from the original firmware.
 *  
 *  Created 2019 by Rumen G. Bogdanovski 
 */

#define VERSION_MAJOR 1
#define VERSION_MINOR 10

/*
 *   Arduino pins
 *
 *   0 - Serial
 *   1 - Serial
 *   2 - Irq Sensor (home for rotator, open for shutter)
 *   3 - Irq Sensor (unused rotator, closed shutter)
 *   4 -
 *   5 - Button
 *   6 - Button
 *   7 -
 *   8 - Reserved for AltSoftSerial if not on leonardo
 *   9 - Soft serial for AltSoftSerial if not on Leonardo
 *   10- Motor Enable
 *   11- Motor Dir
 *   12- Motor Step
 *   13- On board led, we dont use it
 *
 *   These 3 used only when the small prototype hacked for the big easy driver
 *   is in use for bench testing
 *   A0- MS1
 *   A1- MS2
 *   A2- MS3
 *
 */

//  Pin definitions
//  on the uno, ie 328 based units, only pins 2 and 3 are available for interrupts
//  for the rotator, home sensor will be on pin2 and for the shutter, both pins used for limit switches
//  home sensor is on pin 2, we use interrupts here to detect home sensor
//#define HOME 2
//  We dont use the irq on pin 3 for the dome rotator
//  but we do use it for the shutter switches
//#define OTHER_IRQ 3

/*
 *   Defining which pins for serial gets tricky
 *   on the leonardo, no big deal, we just use the standard hardware serial port on pins 0 and 1
 *   but for uno etc, if we intelligently choose our serial pins, we can use alternate better
 *   performing libraries
 *
 *   SoftSerial can go on any pins
 *   AltSoftSerial can only go on tx 9 rx 8 on the uno (or other atmega328) boards
 *   There is also 'yetanothersoftserial', but it uses the two interrupt pins, 2 and 3
 *   which we want for our sensors
 *
 */
#define SERIAL_TX 9
#define SERIAL_RX 8


//  Enable line for the stepper driver
#define EN 10
//  Direction line for the stepper driver
#define DIR 11
//  Step line for the stepper driver
#define STP 12
//  Our two buttons for manual movement
#define BUTTON_EAST 5
#define BUTTON_WEST 6
// RG-11 rain sensor
#define RG11  7


//  Physical definitions of the gears
//  This is the gearbox on the motor
//float ReductionGear=(float)15+((float)3/(float)10);
#define REDUCTION_GEAR 15.3
#define DOME_TEETH 288
#define GEAR_TEETH 15
#define DOME_TURN_TIME 90
// set this to match the type of steps configured on the
// stepper controller
#define STEP_TYPE 8


//  For reading our input voltage
#define VPIN A0

//  if we define our serial ports this way, then re-compiling for different port assignments
//  on different processors, becomes easy and we dont have to hunt all over chaning
//  code to reference different serial definitions
#define Computer Serial
//#define Wireless Serial


#define SHUTTER_STATE_NOT_CONNECTED 0
#define SHUTTER_STATE_OPEN 1
#define SHUTTER_STATE_OPENING 2
#define SHUTTER_STATE_CLOSED 3
#define SHUTTER_STATE_CLOSING 4
#define SHUTTER_STATE_UNKNOWN 5
#define SHUTTER_VERSION_LENGTH 5

//  The shutter sleep period ss 15 seconds
//  Lets query it on every second sleep cycle
//  but send one second early to ensure we catch it
//  when it wakes up
#define SHUTTER_SLEEP_WAIT 29000
//  When in alive mode, the timing for each cycle is one second
//  Query slightly early so the query is waiting when it wakes up
#define SHUTTER_AWAKE_WAIT 900

// RG-11
#define NOT_RAINING	1
#define RAINING		0

//  Globals needed
//  this one is set by the home sensor interrupt routines
//  needs to be volatile because of access during interrupts
volatile bool HomeSensor=false;
volatile bool SenseRising=false;
float HeadingError=0;
float LastHeadingError=0;
//long int LastWirelessCheck;

int ShutterState=SHUTTER_STATE_CLOSED;
float requestedShutterPosition=0;
float ShutterPosition=0;
unsigned int ShutterQueryTime;
char ShutterVersion[SHUTTER_VERSION_LENGTH];
unsigned long int ShutterHibernateTimer=0;
unsigned long int LastCommandTime=0;

int BatteryVolts=0;
int ShutterBatteryVolts=0;
int LowVoltCutoff=0;
bool DomeIsReversed=false;
int rg11State = NOT_RAINING;

//  The dome class will use an accel stepper object to run the motor
//AccelStepper accelStepper(AccelStepper::DRIVER, STP, DIR);
//unsigned int currentPosition = 0;
//unsigned int targetPosition = 0;

#define EEPROM_LOCATION 10
#define SIGNATURE 1036
typedef struct DomeConfiguration {
  int signature;
  long int StepsPerDomeTurn;
  float HomeAzimuth;
  float ParkAzimuth;
  bool IsReversed;
  float Checksum;
} dome_config;

//  Functions that need definitions
void ConfigureWireless();
void HomeInterrupt();


//  Now declare the class for our dome
class NexDome
{
  private:
    long int targetPosition;
    long int currentPosition;
    long int LastHomeCount;
  public:
    NexDome();
    ~NexDome(){}; //  empty destructor because it'll never get used

    //  exposed variables
    bool Active;  //  if the motor is in motion this is true
                  //  exposed publicly because it's performance sensative to reference
    bool FindingHome;
    bool Calibrating;
    
    bool isAtHome();
    bool HasBeenHome;

    float HomeAzimuth;
    float ParkAzimuth;
    long int StepsPerDomeTurn;
    
    int SetStepsPerDomeTurn(long int);
    void MoveTo(long int);
    long int CurrentPosition();
    void Stop();
    bool Run();
    float GetHeading();
    int SetHeading(float);
    int AtHome();   //  called by interrupt routines when home sensor is detected
    void FindHome();
    void Calibrate();
    int Sync(float);

    long int AzimuthToTicks(int);
    bool SaveConfig();
    bool ReadConfig();

    int WriteCalibrationToEEprom();
    int ReadCalibrationFromEEprom();
};

// Every class needs a constructor
NexDome::NexDome()
{
  Active=false; //  we are not in motion when we start up
  targetPosition = 0;
  currentPosition = 0;
  LastHomeCount=0;
  FindingHome=false;
  //SettingHome=false;
  Calibrating=false;
  HomeAzimuth=0;  //  an uncalibrated dome
  ParkAzimuth=180;
  HasBeenHome=false;
  //accelStepper.setPinsInverted(true,false,false);
  memset(ShutterVersion,0,SHUTTER_VERSION_LENGTH);
}

bool NexDome::SaveConfig()
{
  dome_config cfg;

  //  zero the structure so currently unused parts
  //  dont end up loaded with random garbage
  memset(&cfg,0,sizeof(cfg));

  cfg.signature=SIGNATURE;
  cfg.StepsPerDomeTurn=StepsPerDomeTurn;
  cfg.HomeAzimuth=HomeAzimuth;
  cfg.ParkAzimuth=ParkAzimuth;
  cfg.IsReversed=DomeIsReversed;

  //EEPROM.put(EEPROM_LOCATION,cfg);
  return true;
}

bool NexDome::ReadConfig()
{
  dome_config cfg;
  //unsigned char *pt;
  //int value;

  //  zero the structure so currently unused parts
  //  dont end up loaded with random garbage
  memset(&cfg,0,sizeof(cfg));
  //EEPROM.get(EEPROM_LOCATION,cfg);
  /*
  pt=(unsigned char *)&cfg;
  for(int x=0; x<sizeof(dome_config); x++) {
    value=EEPROM.read(EEPROM_LOCATION+x);
    pt[x]=(unsigned char) value;
  }
  */
  if(cfg.signature != SIGNATURE) {
    //Computer.write("Invalid eeprom data");
    return false;
  }
  SetStepsPerDomeTurn(cfg.StepsPerDomeTurn);
  HomeAzimuth=cfg.HomeAzimuth;
  ParkAzimuth=cfg.ParkAzimuth;
  DomeIsReversed=cfg.IsReversed;

  //if(DomeIsReversed) accelStepper.setPinsInverted(false,false,false);

  //Computer.print("Steps per turn from eeprom is ");
  //Computer.print(cfg.StepsPerDomeTurn);
  //Computer.println("");
  //Computer.println(HomeAzimuth);
  //Computer.println(ParkAzimuth);
  //Computer.print("Reversed ");
  //Computer.println(DomeIsReversed);
  return true;
}


int NexDome::SetStepsPerDomeTurn(long int steps)
{
  long int StepsPerSecond;

  //Computer.println("Setting steps per turn to ");
  //Computer.println(steps);

  StepsPerDomeTurn=steps;
  StepsPerSecond=StepsPerDomeTurn/DOME_TURN_TIME;
  if(StepsPerSecond < 2000) StepsPerSecond=2000;
  //Computer.print(StepsPerSecond);
  //Computer.println(" Steps per second");
  //accelStepper.setMaxSpeed(StepsPerSecond);
  //  Set a ramp time
  //accelStepper.setAcceleration(StepsPerSecond*2);

  return 0;
}

bool NexDome::Run()
{
  static unsigned long int lastcall = micros();
  static unsigned long int now;
  static bool setHomeSensor = true;
  long int pos;
  
  if (targetPosition == currentPosition) {
    Active = false;
  } else {
    Active = true;
  }
  if(Active) {
    now = micros();
    long int timediff = now - lastcall;
    if (timediff < 200) {
      return Active;
    }
    lastcall = now;
    pos = currentPosition;
    if ((DomeIsReversed && SenseRising) || (!DomeIsReversed && !SenseRising)) currentPosition--;
    else currentPosition++;
    if(pos < 0) {
      while(pos < 0) pos=pos+StepsPerDomeTurn;
      currentPosition = pos;
    }
    if(pos >= StepsPerDomeTurn) {
      while(pos >= StepsPerDomeTurn) pos=pos-StepsPerDomeTurn;
      currentPosition = pos;
    }
    if ((currentPosition == LastHomeCount) && (setHomeSensor)) { 
      HomeSensor = true;
      setHomeSensor = false; 
    } else if (currentPosition != LastHomeCount) {
      setHomeSensor = true;
    }
  }
  
  return Active;
}

long int NexDome::CurrentPosition()
{
  return currentPosition;
}

//  for the stop function, we want to abandon a target
//  and just start the deceleration, but we want it to stop
//  on an even step boundary, so we do some math hoops to
//  ensure our stop point is on a whole step, not a microstep
void NexDome::Stop()
{
  FindingHome=false;
  Calibrating=false;
  
  if(!Active) return;   //  we are already stopped

  targetPosition = currentPosition;
  return;
}

//  this is a raw move to a stepper count
void NexDome::MoveTo(long int p)
{
  if(p > currentPosition) {
    SenseRising=true;
  } else {
    SenseRising=false;
  }

  if (p > StepsPerDomeTurn) p = p - StepsPerDomeTurn;
  else if (p < 0) p = p + StepsPerDomeTurn;
  
  targetPosition=p;
  return;
}

long int NexDome::AzimuthToTicks(int az)
{
  long int ticks;
  int r;
  ticks=(float)StepsPerDomeTurn/(float)360*az;
  //  now round to whole steps, dropping microsteps
  //r=ticks%STEP_TYPE;
  //ticks=ticks-r;
  return ticks;
}

float NexDome::GetHeading()
{
  /* floating point math for get heading
   *  prefer floats for accurately placing
   *  the power pickup to charge the shutter
   *  but it introduces significant processing
   *  overhead
   */

  float Heading;
  Heading=(float)CurrentPosition()/(float)StepsPerDomeTurn*360;
  //  in case we need to do another step
  //  do a run sequence now
  if(Active) Run();
  while(Heading < 0) Heading+=360;
  while(Heading >= 360) Heading-=360;
  if(Active) Run();
  return Heading;
}

int NexDome::SetHeading(float h)
{
  float target=0;
  //long int TargetSteps;
  float current;
  int r;
  float delta;
  float newdelta;
  //bool GoingBackwards;
  //long int StepsToGo;

  FindingHome=false;
  //SettingHome=false;
  Calibrating=false;
  current=GetHeading();
  delta=h-current;
  if(delta == 0) return 0; //  we are already there

  if((delta <= 180)&&(delta >= -180)) {
    target=current+delta;
  }
  if(delta > 180) {
    //  its more than 180 degrees ahead
    newdelta=360-delta;
    target=current-newdelta;
  }
  if(delta < -180) {
    newdelta=360+delta;
    target=current+newdelta;
  }

/*
  Computer.print("heading now ");
  Computer.println(current);
  Computer.print("desired heading ");
  Computer.println(h);
  Computer.print("delta is ");
  Computer.println(delta);
  Computer.print("new target is ");
  Computer.println(target);
*/



  /*
  target=h/360.0*StepsPerDomeTurn;
  */
  targetPosition=target/360*StepsPerDomeTurn;
  //Computer.println(CurrentPosition());
  //Computer.println(TargetSteps);

  MoveTo(targetPosition);
  return 0;
}

int NexDome::Sync(float val)
{
  long int newaz;

  newaz=AzimuthToTicks(val);
  /*
  long int pos = LastHomeCount - newaz;

  if(pos < 0) {
    while(pos < 0) pos=pos+StepsPerDomeTurn;
  }
  if(pos >= StepsPerDomeTurn) {
    while(pos >= StepsPerDomeTurn) pos=pos-StepsPerDomeTurn;
  }
  LastHomeCount = newaz;
  */ 
  //Computer.print("Setting new tick count ");
  //Computer.print(newaz);
  //accelStepper.setCurrentPosition(newaz);
  currentPosition = newaz;
  targetPosition = currentPosition;
  
  //Computer.print(" sync with ");
  //Computer.println(val);
  return 0;
}


int NexDome::AtHome()
{
  int r;

  HasBeenHome=true;
  LastHomeCount=CurrentPosition();
  
  if(FindingHome) {
    if(Calibrating) {
      Calibrating=false;
    }
    FindingHome=false;
    MoveTo(LastHomeCount);
  }
  return 0;
}

void NexDome::FindHome()
{
  //  we dont have to find home if we are already there
  if(isAtHome()) return;
  SetHeading(HomeAzimuth);
  FindingHome=true;
}

void NexDome::Calibrate()
{
  HomeAzimuth=0;
  //accelStepper.setCurrentPosition(0);
  currentPosition = 0;
  MoveTo(CurrentPosition() + 5000);
  Calibrating=true;
  FindingHome=true;
}

bool NexDome::isAtHome()
{
  if((currentPosition >= LastHomeCount) && (currentPosition <= LastHomeCount+10)) return true;
  return false;
}

NexDome Dome;


void setup() {
  float MotorTurnsPerDomeTurn;
  float StepsPerGearTurn;
  long int StepsPerDomeTurn;
  //float StepsPerSecond;
  long int StepsPerDegree;

  Computer.begin(9600);
  //Wireless.begin(9600);

  MotorTurnsPerDomeTurn = (float)DOME_TEETH / (float) GEAR_TEETH;
  StepsPerGearTurn=200.0*(float)REDUCTION_GEAR*(float)STEP_TYPE;
  StepsPerDomeTurn=MotorTurnsPerDomeTurn*StepsPerGearTurn;
  StepsPerDegree=StepsPerDomeTurn/360;

  Dome.SetStepsPerDomeTurn(StepsPerDomeTurn);

  Dome.ReadConfig();

  ShutterQueryTime=SHUTTER_SLEEP_WAIT;
  ConfigureWireless();

  if(Dome.isAtHome()) {
    //  the dome is at the home sensor now
    if(Dome.HomeAzimuth != 0) Dome.Sync(Dome.HomeAzimuth);
  }

}


//  We dont need to check the buttons
//  on every loop, it adds a lot of overhead
//  routing thru digitalRead functions
//  We only need to do it 10 times a second max

// check RG11 status
// if it's not connected the pin is constantly High, aka not raining
int CheckRG11()
{
  int RG11_state;

  //RG11_state = digitalRead(RG11);
  
  RG11_state = NOT_RAINING;
  return RG11_state;
}

//  define a buffer and pointers for our serial data state machine
// this definition conflicts with arduino headers in later versions
// we can just run with what they have defined for now
#define MY_SERIAL_BUFFER_SIZE 20

char SerialBuffer[MY_SERIAL_BUFFER_SIZE];
int SerialPointer=0;
bool SerialTarget=true;

//  Now define a buffer and pointers for our serial link over wireless
//  for talking to the shutter
char WirelessBuffer[MY_SERIAL_BUFFER_SIZE];
int WirelessPointer=0;

//  and some variables used to time out the state machines

unsigned long int LastShutterResponse=0;
unsigned long int LastShutterKeepAlive=0;
bool ShutterAlive=false;
bool DoingWirelessConfig=false;
int WirelessConfigState=0;
bool FoundXBee=false;

/*
 * When processing time sensative serial commands
 * we use sprintf and write to avoid Serial library overhead
 * which causes the stepper to stutter during command
 * exchanges if using simple print and println functions
 *
 * On arduino, sprintf doesn't support floating point
 * so for float returns we format with dtostrf
 *
 * We process only a subset of commands when the dome is
 * in motion, because some of the shutter commands can tak
 * a few seconds to complete, and we dont want to make our
 * state machines so complex they dont fit in arduino memory
 *
 * The simple solution, only process dome status commands while
 * in motion, and process the rest when dome is not running
 *
 */
void ProcessSerialCommand()
{

  char buf[20];
  //  reset our drop dead timer
  LastCommandTime=millis();

  /* Abort current motion */
  if(SerialBuffer[0]=='a') {
    Dome.Stop();
    // Abort shutter movenebt too
    if(ShutterPosition <= 0) {
      ShutterPosition = 0;
      ShutterState = SHUTTER_STATE_CLOSED;
    } else ShutterState = SHUTTER_STATE_OPEN;

    Computer.println("A");
  }
  /*  Raw position count */
  if(SerialBuffer[0]=='p') {
    sprintf(buf,"P %ld\n",Dome.CurrentPosition());
    Computer.write(buf,strlen(buf));
    SerialPointer=0;
    return;
  }
  /* is dome in motion */
  if(SerialBuffer[0]=='m') {
    int state=0;
    //Computer.write("M ");
    //  if dome is in motion, figure out what kind of motion
    if(Dome.Active) {
      Dome.Run();
      //  default is just a normal move
      state=1;
      if(Dome.FindingHome) state=2;
      if(Dome.Calibrating) state=3;
    }
    sprintf(buf,"M %d\n",state);
    Computer.write(buf);
    //Computer.write("\n");
    //Computer.println(state);
    SerialPointer=0;
    return;
  }
  /* query current heading */
  if(SerialBuffer[0]=='q') {
    //Computer.print("H ");
    //Computer.println(Dome.GetHeading());
    dtostrf(Dome.GetHeading(),2,1,buf);
    if(Dome.Active) Dome.Run();
    ////sprintf(buf,"H %s\n",b);
    Computer.write("Q ");
    Computer.write(buf);
    Computer.write("\n");
    //Computer.print("Q ");
    //Computer.println(Dome.GetHeading());
    SerialPointer=0;
    return;
  }


  /*
   * These commands should not be processed when the dome is in motion
   *
   */

   /* query shutter state */
   if(SerialBuffer[0]=='u') {
    if(Dome.Active) Dome.Run();
    Computer.print("U ");
    Computer.print(ShutterState);
    Computer.print(" ");
    Computer.println(rg11State);
   }
   /*  get shutter position  */
   if(SerialBuffer[0]=='b') {
    if(Dome.Active) Dome.Run();
    Computer.print("B ");
    Computer.println(ShutterPosition);
   }
   /*  Open Shutter  */
   if(SerialBuffer[0]=='d') {
		if(rg11State == RAINING) {
			Computer.println("E");
		}
		else {
			Computer.println("D");
			if(ShutterState != SHUTTER_STATE_NOT_CONNECTED) {
			  ShutterState=SHUTTER_STATE_OPENING;
        requestedShutterPosition = 90;
			}
		}
	}
  /*  Close Shutter  */
  if(SerialBuffer[0]=='e') {
    Computer.println("D");
    if(ShutterState != SHUTTER_STATE_NOT_CONNECTED) {
      ShutterState=SHUTTER_STATE_CLOSING;
      requestedShutterPosition = 0;
    }
  }
  /*  Set Shutter Position */
  if(SerialBuffer[0]=='f') {
    Computer.println("F");
    requestedShutterPosition = atof(&SerialBuffer[1]);
    if(ShutterPosition >= requestedShutterPosition) {
      ShutterState=SHUTTER_STATE_CLOSING;
    } else {
      ShutterState=SHUTTER_STATE_OPENING;
    }
  }

  // Wake up the shutter so it's more responsive while config
  //  dialog is on screen
  if(SerialBuffer[0]=='x') {
    //Wireless.println("x");
    Computer.println("X");
    //ShutterQueryTime=SHUTTER_AWAKE_WAIT;
  }
  /* sync on this heading */
  if(SerialBuffer[0]=='s') {
    float h;
    float hnow;
    float delta;
    //float oldhome;
    float newhome;
    //  dont adjust home sensor if we have not seen it to
    //  verify where it is, this prevents a sync on park position
    //  from screwing it up when we power up
    h=atof(&SerialBuffer[1]);
    if(Dome.HasBeenHome) {
      //  We are syncing, lets calculate the new offset for the home
      //  sensor
      hnow=Dome.GetHeading();
      delta=hnow-h;
      newhome=Dome.HomeAzimuth-delta;
      if(newhome < 0) newhome=newhome+360;
      if(newhome > 360) newhome=newhome-360;
      if(newhome==360) newhome=0;
      Dome.HomeAzimuth=newhome;
      Dome.SaveConfig();
    } else {
      //Computer.println("Dome has not been home");
    }
    if((h >=0)&&(h<360)) {
       Computer.print("S ");
       Computer.println(h);
       Dome.Sync(h);
    } else {
      Computer.println("E ");
    }
  }
  if(SerialBuffer[0]=='o') {
    Computer.print("O ");
    Computer.println(LastHeadingError);
    //  Now set this to the last one we caught
    //  So we will report the next one, even if it's tiny
    LastHeadingError=HeadingError;
  }
  /* current status of steps per turn */
  if(SerialBuffer[0]=='t') {
    Computer.print("T ");
    Computer.println(Dome.StepsPerDomeTurn);
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");

  }
  /* query for home azimuth */
  if(SerialBuffer[0]=='i') {
    Computer.print("I ");
    Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");

  }
  /* Set the home azimuth */
  if(SerialBuffer[0]=='j') {
    //  dont set this if we got an empty set
    if((SerialBuffer[1]!=0x0a)&&(SerialBuffer[1]!=0x0d)) {
      float newaz;
      newaz=atof(&SerialBuffer[1]);
      if((newaz >=0)&&(newaz < 360)) Dome.HomeAzimuth=newaz;
    }
    //  Respond same as a query for home azimuth
    Computer.print("I ");
    Computer.println(Dome.HomeAzimuth);
    Dome.SaveConfig();
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");

  }
  /* query for park azimuth */
  if(SerialBuffer[0]=='n') {
    Computer.print("N ");
    Computer.println(Dome.ParkAzimuth);
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");

  }
  /* Set the park azimuth */
  if(SerialBuffer[0]=='l') {
    float newaz;
    newaz=atof(&SerialBuffer[1]);
    if((newaz >=0)&&(newaz < 360)) Dome.ParkAzimuth=newaz;
    //  Respond same as a query for park
    Computer.print("N ");
    Computer.println(Dome.ParkAzimuth);
    Dome.SaveConfig();
    //Computer.println(" Steps per Dome turn");
    //Computer.println(Dome.HomeAzimuth);
    //Computer.println(" Home Azimuth");

  }

/*  goto based on a heading */
  if(SerialBuffer[0]=='g') {
    float target;
    target=atof(&SerialBuffer[1]);
    if((target >=0.0)&&(target <= 360.0)) {
      SerialTarget=true;
      Dome.SetHeading(target);
      Computer.println("G");
    } else {
      Computer.println("E");
    }
  }
  /* find the home position */
  if(SerialBuffer[0]=='h') {
    Dome.FindHome();
    SerialTarget=true;
    Computer.println("H");
  }
  /* are we at the home position */
  if(SerialBuffer[0]=='z') {
    Computer.print("Z ");
    if(Dome.isAtHome()) Computer.println("1");
    else {
      if(Dome.HasBeenHome) {
        Computer.println("0");
      }else {
        Computer.println("-1");
      }
    }
  }
  /* query for battery info */
  if(SerialBuffer[0]=='k') {
    if(SerialBuffer[1]==' ') {
      int newcutoff;
      newcutoff=atoi(&SerialBuffer[1]);
      //Wireless.print("b ");
      //Wireless.println(newcutoff);
    }
    ShutterBatteryVolts = 751; /*7.51 V*/
    Computer.print("K ");
    Computer.print(BatteryVolts);
    Computer.print(" ");
    Computer.print(ShutterBatteryVolts);
    Computer.print(" ");
    Computer.println(LowVoltCutoff);
  }
  /* calibrate the number of steps in a full turn */
  if(SerialBuffer[0]=='c') {
    //  calibrate the dome
    //  In order to calibrate, we need to be at the home position
    if(Dome.isAtHome()) {
      Computer.println("C");
      SerialTarget=true;
      Dome.Calibrate();

    } else {
      Computer.println("E");
    }
  }
  //  get / set the reversed flag
  if(SerialBuffer[0]=='y') {
    if(SerialBuffer[1]==' ') {
      //  This is a command to set the reverse flag
      if(SerialBuffer[2]=='1') {
        //  only do this if we are actually changing
        if(!DomeIsReversed) {
          DomeIsReversed=true;
          Dome.SaveConfig();
          //accelStepper.setPinsInverted(false,false,false);
        }
      } else {
        if(DomeIsReversed) {
          DomeIsReversed=false;
          Dome.SaveConfig();
          //accelStepper.setPinsInverted(true,false,false);
        }
      }
    }
    Computer.print("Y ");
    Computer.println(DomeIsReversed);

  }
  //  query/set for shutter hibernate timer
  if(SerialBuffer[0]=='r') {
    if(SerialBuffer[1]==' ') {
      ShutterHibernateTimer=atol(&SerialBuffer[1]);
    }
    Computer.print("R ");
    Computer.println(ShutterHibernateTimer);
  }
  /*  restart xbee wireless */
  if(SerialBuffer[0]=='w') {
    Computer.println("W");
    ConfigureWireless();
  }

  /* get firmware version */
  if(SerialBuffer[0]=='v') {
    Computer.print("VNexDome V ");
    Computer.print(VERSION_MAJOR);
    Computer.print(".");
    Computer.print(VERSION_MINOR);
    if(ShutterVersion[0] != 0) {
      Computer.print(" NexShutter V ");
      Computer.println(ShutterVersion);
    }
    Computer.println("");
  }

  SerialPointer=0;
  return;
}

void IncomingSerialChar(char a)
{

  if((a=='\n')||(a=='\r')) {
    //  dont process an empty line feed
    if(SerialPointer > 0) {
      ProcessSerialCommand();
    }
    memset(SerialBuffer,0,MY_SERIAL_BUFFER_SIZE);
    SerialPointer=0;
    return;
  }

  SerialBuffer[SerialPointer]=a;
  SerialPointer++;
  if(SerialPointer==MY_SERIAL_BUFFER_SIZE) {
    //  we are going to overflow the buffer
    //  erase it and start over
    memset(SerialBuffer,0,MY_SERIAL_BUFFER_SIZE);
    SerialPointer=0;
  } else {
    SerialBuffer[SerialPointer]=0;
  }
  return;
}

/*
 * Stuff we need for handling communication with the shutter
 *
 */


void ConfigureWireless()
{
  //  do nothing for now
  //return;
  //Computer.println("Sending + to xbee");
  delay(1100);
  DoingWirelessConfig=true;
  WirelessConfigState=0;
  memset(WirelessBuffer,0,MY_SERIAL_BUFFER_SIZE);
  WirelessPointer=0;
  //Wireless.print("+++");
}


void ProcessShutterData()
{

  //Computer.println(WirelessBuffer);
  if(WirelessBuffer[0]=='O') {
    if(WirelessBuffer[1]=='K') {
      //  The xbee unit is ready for configuration
      FoundXBee=true;
      if(DoingWirelessConfig) {
        switch(WirelessConfigState) {
          case 0:
            //  We could do the whole setup in just one command
            //  but it gives back the same number of OK responses
            //  So we have to parse out every OK anyways, may as well just
            //  do one setting per OK return
            //Wireless.println("ATID5555,CE1,PL0,SM4,SP5DC,ST100,CN");
            //Wireless.println("ATID5555");
            //Computer.println("Setting wireless id");
            break;

          case 1:
            //Wireless.println("ATCE1");
            //Computer.println("Setting cordinator");
            break;

         case 2:
            //Wireless.println("ATPL0");
            //Computer.println("Setting Power");
            break;

          case 3:
            //Wireless.println("ATSM4");
            //Wireless.println("ATSM0");
            //Computer.println("Setting sleep mode");
            break;

          case 4:
            //  set sleep period, this is the co-ordinator
            //  so it defines how long we will hold a message
            //  for the endpoint
            //Wireless.println("ATSP5DC");
            //Computer.println("Setting sleep period");
            break;

          case 5:
            //Wireless.println("ATST100");
            //Computer.println("Setting sleep wait time");
            break;

           case 6:
            //Wireless.println("ATCN");
            //Computer.println("Exit command mode");
            break;

         default:
            //  Finished configuring, now see if the shutter is alive
            //Wireless.println("s");
            //Computer.println("Wireless config finished");
            DoingWirelessConfig=false;
            break;
        }
        WirelessConfigState++;
      }
      //Computer.println("Clear wireless buffer");
      memset(WirelessBuffer,0,MY_SERIAL_BUFFER_SIZE);
      WirelessPointer=0;
      return;
    }
  }
  // update our timer for the keep alive routines
  if(!ShutterAlive) {
    if(WirelessBuffer[0]=='S') {
      //Computer.println(WirelessBuffer);
      //Computer.println("Shutter woke up");
      ShutterAlive=true;
    }
  }
  if(ShutterAlive) {
    if(WirelessBuffer[0]=='S') {
      long int newShutterState;
      LastShutterResponse=millis();
      LastShutterKeepAlive=millis();  //  mark this time for our keepalive routines as well
      //  the first character in the response is the shutter state
      //ShutterQueryTime=SHUTTER_SLEEP_WAIT;
      if(WirelessBuffer[2]=='0') ShutterQueryTime=SHUTTER_AWAKE_WAIT;
      else ShutterQueryTime=SHUTTER_SLEEP_WAIT;
      switch(WirelessBuffer[1]) {
        case 'C':
          newShutterState=SHUTTER_STATE_CLOSED;
          break;
        case 'O':
          newShutterState=SHUTTER_STATE_OPEN;
          break;
        case 'P':
          newShutterState=SHUTTER_STATE_OPENING;
          ShutterQueryTime=SHUTTER_AWAKE_WAIT;
          break;
        case 'D':
          newShutterState=SHUTTER_STATE_CLOSING;
          ShutterQueryTime=SHUTTER_AWAKE_WAIT;
          break;
        default:
          newShutterState=SHUTTER_STATE_UNKNOWN;
          break;
        //default:
        //  ShutterState=SHUTTER_STATE_NOT_CONNECTED;
        //  break;
      }
      if(newShutterState != ShutterState) {
        ShutterState=newShutterState;
        //Computer.print("Shutter State ");
        //Computer.println(ShutterState);
      }
      //  shutter has responded with a status
      //  lets ask it for a position
     // if((ShutterState != SHUTTER_STATE_OPENING)&&(ShutterState != SHUTTER_STATE_CLOSING))Wireless.println("p");
    }
    if(WirelessBuffer[0]=='P') {
      ShutterPosition=atof(&WirelessBuffer[1]);
      //Computer.println("Shutter Position ");
      //Computer.println(q);
      //  We got a position
      //  lets ask for battery voltage
     // Wireless.println("b");
    }
    if(WirelessBuffer[0]=='B') {
      char *ptr;
      //ShutterBatteryVolts=atoi(&WirelessBuffer[1]);
      //  now find the low voltage cutoff if it exists
      ptr=&WirelessBuffer[2];
      while((ptr[0]!=0)&&(ptr[0]!=' ')&&(ptr[0]!=0x0a)&&(ptr[0]!=0x0d)) ptr++;
      if(ptr[0]==' ') {
        LowVoltCutoff=atoi(&ptr[0]);
      }
      if(ShutterHibernateTimer==0) {
        //Wireless.println("h");
      }
      if(ShutterVersion[0]==0) {
        //  if we dont have a version for the shutter
        //  fetch it now
        //Computer.println("Fetch Shutter Version");
       // Wireless.println("v");
      }
     }
    if(WirelessBuffer[0]=='V') {
      //Computer.print("Shutter Version ");
      //Computer.println(WirelessBuffer);
      memcpy(ShutterVersion,&WirelessBuffer[12],4);
      //Computer.println(ShutterVersion);
    }
    if(WirelessBuffer[0]=='H') {
      if(WirelessBuffer[1]==' ') {
        ShutterHibernateTimer=atol(&WirelessBuffer[1]);
      }
    }
  }

  // clear the buffer now that it's processed
  memset(WirelessBuffer,0,MY_SERIAL_BUFFER_SIZE);
  WirelessPointer=0;

  return;

}

void IncomingWirelessChar(char a)
{
  //Computer.print(a);
  if((a=='\n')||(a=='\r')) {
    return ProcessShutterData();
  }
  //Computer.println((int)a);
  WirelessBuffer[WirelessPointer]=a;
  WirelessPointer++;
  if(WirelessPointer==MY_SERIAL_BUFFER_SIZE) {
    memset(WirelessBuffer,0,MY_SERIAL_BUFFER_SIZE);
    WirelessPointer=0;
  } else {
    WirelessBuffer[WirelessPointer]=0;
  }
  if(a < 0) {
    Computer.println("Wireless Clearing garbage");
    WirelessPointer=0;
    WirelessBuffer[0]=0;
  }
  //Computer.print(a);
  return;
}

unsigned long int LastBatteryCheck=0;

int CheckBattery()
{
  int volts;

  volts=analogRead(VPIN);
  //Computer.println(volts);
  volts=volts/2;
  volts=volts*3;
  //v=(float)volts/(float)100;
  //BatteryVolts=volts;
  BatteryVolts = 752; /*7.52 V*/
  //Computer.println(volts);
  return 0;
}

void loop() {
  if(HomeSensor) {
    float h;
    h=Dome.GetHeading();
    HeadingError=Dome.HomeAzimuth-h;
    //  if home position is within a degree of zero
    //  need to make an adjustment for rollover
    if(HeadingError < -359) HeadingError=HeadingError+360;
    Dome.AtHome();
    HomeSensor=false;
  }
  
  if(Dome.FindingHome) {
    if(SenseRising) {
      Dome.MoveTo(Dome.CurrentPosition()+5000);
    } else {
      Dome.MoveTo(Dome.CurrentPosition()-5000);
    }
  }

  // check the rain sensor
  rg11State = CheckRG11();
  if(rg11State == RAINING) {
    // close shutter !!!!
    if(ShutterState != SHUTTER_STATE_NOT_CONNECTED && ShutterState == SHUTTER_STATE_OPEN) {// why would you have a rain sensor without a shutter ?! :)
      //Wireless.println("c");
      ShutterState=SHUTTER_STATE_CLOSING;
      requestedShutterPosition = 0;
      }
  }

  //  is there any data from the host computer to process
  if(Computer.available()) {
    IncomingSerialChar(Computer.read());
  }
  
  if (!Dome.Run()) {
    //  When the dome is not moving
    if((HeadingError > 1.0) || (HeadingError < -1.0)) {
      float pos;

      //Computer.print("Heading correction ");
      //Computer.println(HeadingError);

      pos=Dome.GetHeading();
      pos=pos+HeadingError;
      Dome.Sync(pos);
      LastHeadingError=HeadingError;
      HeadingError=0;
    }
    
    unsigned long int now;
    now=millis();

    if(now < LastShutterKeepAlive) {
    //  the millisecond timer has rolled over
      LastShutterKeepAlive=now;
    }
    if((now - LastShutterKeepAlive) > ShutterQueryTime) {
      CheckBattery();
      if(!FoundXBee) {
        ConfigureWireless();
      } else {
        //  send shutter a status query
        //Computer.println("Sending shutter status request");
        //Wireless.println("s");
        //Wireless.println(millis());
        //  and get the battery voltage from the shutter
        //Wireless.println("b");
      }
      //Wireless.println(AliveCount++);
      LastShutterKeepAlive=now;
    }

    switch (ShutterState) {
      case SHUTTER_STATE_OPENING:
        if (requestedShutterPosition <= ShutterPosition) {
          if(ShutterPosition <= 0) {
            ShutterPosition = 0;
            ShutterState = SHUTTER_STATE_CLOSED;
          } else ShutterState = SHUTTER_STATE_OPEN;
        } else {
          if (requestedShutterPosition > ShutterPosition) ShutterPosition += 0.0001;
        }
        break;
      case SHUTTER_STATE_CLOSING:
        if (requestedShutterPosition >= ShutterPosition) {
          if(ShutterPosition <= 0) {
            ShutterPosition = 0;
            ShutterState = SHUTTER_STATE_CLOSED;
          } else ShutterState = SHUTTER_STATE_OPEN;
        } else {
          if (requestedShutterPosition < ShutterPosition) ShutterPosition -= 0.0001;
        }
        break;
    }
  }
}

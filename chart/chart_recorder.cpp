/* chart_recorder.cpp - Simulated Chart Recorder (Auratic Interpretter)
 by Wes Modes (wmodes@gmail.com) & SL Benz (slbenzy@gmail.com)
 29 January 2017 
*/

#include <Servo.h>
#include <math.h>
using namespace std;

// CONSTANTS

const bool IDLESWEEP = true;
const int IDLEPERIOD = 600;  // number of moves between start and end of full cycle

const int PENMIN = 50;       // minimum allowable rotation of servo
const int PENMAX = 170;     // mIDLEaximum allowable rotation of servo
const int PENAMP = (PENMAX - PENMIN) / 2; // max allowable amplitude
const int PENREST = PENMIN + PENAMP;  // where pen rests, center of min & max
const int PENFREQ = 1;      // number of full waves to make in each cycle
const int ACTIVEPERIOD = 25;  // number of moves between start and end of full cycle

const int GLOBAL_WAIT = 30;

const int AVAILPENS[] = {9, 10, 11, 3, 5, 6};   // avail arduino pins (order of pref)
const int MAXPENS = sizeof(AVAILPENS);
const int PENSINUSE = 2;    // how many pens are we actually using

String reqId = "id";
String rspId = "id:chart";
String reqStatus = "status";
String reqStart = "start";
String reqStop = "stop";
String reqDebug = "debug";
String reqNoDebug = "nodebug";
String rspAck = "OK";
String reqHandshake = "hello?";
String rspHandshake = "hello!";

// Globals
bool activated = false;
bool debug = false;

// Set up 

int penPos[PENSINUSE];      // postion of servo
int penX[PENSINUSE];        // graph position of x (vertical value)
int penPer[PENSINUSE];      // period of one wave
int penAmp[PENSINUSE];      // amplitude of current movement 0 to 100%
bool penMoving[PENSINUSE];  // check whether still traveling
Servo servo[PENSINUSE];     // declare an array of server objects
int datasetPos[PENSINUSE];  // records where we are in the dataset
int globalPeriod = IDLEPERIOD;  // number of moves between start and end of full cycle

// an idle dataset
int dataset0[] = {50};

// a sorta EKG dataset
int dataset1[] = {
    5, 10, 5, 30, 5, 10, 5,
    5, 10, 10, 75, 10, 10, 5,
    25, 15, 5, 15, 25,
    5, 10, 5, 10, 5, 10, 5,
    5, 10, 10, 75, 10, 10, 5,
    25, 15, 5, 15, 25,
    5, 10, 5, 30, 5, 10, 5,
    25, 15, 5, 15, 25,
    25, 50, 75, 100, 75, 100, 75, 50, 25,
    10, 45, 100, 45, 10,
    5, 10, 10, 75, 10, 10, 5,
    25, 15, 5, 15, 25,
    5, 5, 5, 10, 5, 5, 5,
    5, 10, 10, 75, 10, 10, 5,
    5, 10, 5, 30, 5, 10, 5,
    25, 15, 5, 15, 25,
    25, 75, 25,
    25, 15, 5, 15, 25,
    5, 10, 5, 30, 5, 10, 5,
    25, 50, 75, 100, 75, 100, 75, 50, 25,
    25, 15, 5, 15, 25,
    10, 45, 100, 45, 10,
    5, 10, 5, 10, 5, 10, 5,
    5, 10, 10, 75, 10, 10, 5,
    5, 5, 5, 10, 5, 5, 5,
    25, 10, 10, 10, 25, 10, 10, 10,
    25, 15, 5, 15, 25,
    25, 10, 10, 10, 25, 10, 10, 10,
    25, 75, 25,
    5, 10, 10, 75, 10, 10, 5
};

// a sorta heartbeat dataset
int dataset2[] = {
  5, 5, 5, 5, 20, 100, 50, 
  5, 5, 5, 5, 5, 
  5, 5, 5, 5, 5, 
  5, 5, 5, 5, 5
};

int* dataset[] = {
  dataset0,
  dataset1,
  dataset2
};

int datalength0 = sizeof(dataset0)/sizeof(*dataset0);
int datalength1 = sizeof(dataset1)/sizeof(*dataset1);
int datalength2 = sizeof(dataset2)/sizeof(*dataset2);

int datalength[] = {
  datalength0,
  datalength1,
  datalength2
};

// // calculate length of array (sigh, C++)
// const int DATASIZE = sizeof(dataset)/sizeof(*dataset);

void penSetup(int penNo) {
  servo[penNo].attach(AVAILPENS[penNo]);  // attaches the servo object to specified pin
  penReset(penNo);
}

void penStart(int penNo, int amp)
{
  penPos[penNo] = PENREST;
  penX[penNo] = 0;

  penPer[penNo] = globalPeriod * amp / 100;
  if (debug) {
    Serial.print (", globalPeriod: ");
    Serial.print (globalPeriod);
    Serial.print (", amplitude: ");
    Serial.print (amp);
    Serial.print (", penPeriod: ");
    Serial.println (penPer[penNo]);
  }
  penAmp[penNo] = amp;
  penMoving[penNo] = true;
  penPosition(penNo, PENREST);
}

void penReset(int penNo)
{
  //Serial.println("RESETing!");
  penPos[penNo] = PENREST;
  penX[penNo] = 0;
  penPer[penNo] = 1;
  penAmp[penNo] = 0;
  penMoving[penNo] = false;
  //penStatus(penNo);
  //penPosition(penNo, penPos[penNo]);
  //Serial.println("Done RESETing!");
}

void penMove(int penNo)
{
  // if we aren't traveling, we shouldn't be moving
  if (! penMoving[penNo]) {
    // Serial.println("Done traveling");
    return;
  }
  // we calculate new pen Y-position according to sin function of X-position
  // float penY = sin(2 * (float)M_PI * PENFREQ * ((float)penX[penNo] / globalPeriod));
  float r = (float)penX[penNo] / penPer[penNo];
  float penY = sin(2 * (float)M_PI * PENFREQ * r);
  if (debug) {
    Serial.print (" penX: ");
    Serial.print (penX[penNo]);
    Serial.print (", penPeriod: ");
    Serial.print (penPer[penNo]);
    Serial.print (", r (penX/PenPer): ");
    Serial.print (r);
    Serial.print (", penY: ");
    Serial.println(penY);
  }
  // now we scale it
  int newPos = (int)(PENREST + (PENAMP * penY * penAmp[penNo] / 100));
  // check to make sure pen hasn't hit our limits (not likely, but a good practice)
  if (newPos > PENMAX) {
    newPos = PENMAX;
  }
  else if (newPos < PENMIN) {
    newPos = PENMIN;
  }
  // okay, we like our newPos, let's set it
  penPos[penNo] = newPos;
  // increment our X coordinate
  penX[penNo]++;
  // are we at the end of the period?
  if (penX[penNo] > penPer[penNo]) {
    penPos[penNo] = PENREST;
    penX[penNo] = 0;
    penPer[penNo] = 1;
    penAmp[penNo] = 0;
    penMoving[penNo] = false;
  }
  else {
    // now move the actual servo
    penPosition(penNo, penPos[penNo]);
    // Serial.print(penX[penNo]);
    // Serial.print(", ");
    // Serial.print(penY);
    // Serial.print(", position: ");
    // Serial.println(penPos[penNo]);
    // Serial.print(".");
  }
}

void penPosition(int penNo, int pos)
{
  servo[penNo].write(pos);
}

void penStatus(int penNo)
{
  Serial.print("Pen: ");
  Serial.print(penNo);
  Serial.print(" on pin: ");
  Serial.println(AVAILPENS[penNo]);

  Serial.print("Period: ");
  Serial.print(penPer[penNo]);
  Serial.print(", Amplitude: ");
  Serial.println(penAmp[penNo]);

  Serial.print("Position: ");
  Serial.print(penPos[penNo]);
  Serial.print(", PenX: ");
  Serial.print(penX[penNo]);
  Serial.print(", Pen moving: ");
  Serial.println(penMoving[penNo]);
  Serial.println("");
}

void startAll()
{
  activated = true;
  globalPeriod = ACTIVEPERIOD;
  for (int penNo = 0; penNo < PENSINUSE; penNo += 1) {
    datasetPos[penNo] = random(datalength[penNo+1]);
  }
}

void stopAll()
{
  activated = false;
  globalPeriod = IDLEPERIOD;
}

void moveAll() 
{
  for (int penNo = 0; penNo < PENSINUSE; penNo += 1) {
    penMove(penNo);
  }
  delay(GLOBAL_WAIT);
}

void checkForRequests()
{
  // do we have a request from the master?
  if (Serial.available() > 0) 
  {
    // get request off USB port
    String reqMaster = Serial.readString();
    //Serial.print("We received: ");
    //Serial.println(reqMaster);
    // did we receive a HANDSHAKE request?
    if (reqMaster.indexOf(reqHandshake) >= 0) {
      Serial.println(rspHandshake);
    }
    // did we receive an ID request?
    else if (reqMaster.indexOf(reqId) >= 0) {
      //Serial.print("We sent: ");
      Serial.println(rspId);
    }
    // did we receive a STATUS request?
    else if (reqMaster.indexOf(reqStatus) >= 0) {
      Serial.println(reqStatus + ":");
      Serial.print("  PensInUse:");
      Serial.println(PENSINUSE, DEC);
      Serial.print("  Activated:");
      Serial.println(activated, DEC);
      Serial.print("  IdleSweep:");
      Serial.println(IDLESWEEP, DEC);
      Serial.print("  Debug:");
      Serial.println(debug, DEC);
    }
    // did we receive a START request?
    else if (reqMaster.indexOf(reqStart) >= 0) {
      Serial.println(reqStart + ":" + rspAck);
      startAll();
    }
    // did we receive a STOP request?
    else if (reqMaster.indexOf(reqStop) >= 0) {
      Serial.println(reqStop + ":" + rspAck);
      stopAll();
    }
    // did we receive a NODEBUG request?
    else if (reqMaster.indexOf(reqNoDebug) >= 0) {
      Serial.println(reqNoDebug + ":" + rspAck);
      debug = false;
    }
    // did we receive a DEBUG request?
    else if (reqMaster.indexOf(reqDebug) >= 0) {
      Serial.println(reqDebug + ":" + rspAck);
      debug = true;
    }
    else {
      Serial.println("Unknown-request:" + reqMaster);
    }
  }
}

void doTheThings()
{
  if (activated || IDLESWEEP) {
    int newAmp = 0;
    int datasetNum;
    //Serial.println("Working...");
    // delay(100);
    // for each attached pen, we update it once per cycle
    for (int penNo = 0; penNo < PENSINUSE; penNo += 1) {
      //penStatus(penNo);
      // if we aren't activated, but idlesweep is on
      if (!activated) {
        datasetNum = 0;
      }
      else {
        datasetNum = penNo + 1;
      }
      // if we are not in the middle of a wave...
      if (! penMoving[penNo]){
        // ... get a new wave from the dataset
        int newAmp = dataset[datasetNum][datasetPos[penNo]];
        if (debug) {
          Serial.print("PenNo: ");
          Serial.print(penNo);
          Serial.print(", Dataset: ");
          Serial.print(datasetNum);
          Serial.print(", DatasetPos: ");
          Serial.print(datasetPos[penNo]);
          Serial.print(", NewAmp: ");
          Serial.println(newAmp);
        }
        penStart(penNo, newAmp);
        datasetPos[penNo]++;
        // check to see if we've run out of data
        if (datasetPos[penNo] >= datalength[datasetNum]) {
          datasetPos[penNo] = 0;
        }
        //penStatus(penNo);
      }
      else {
        penMove(penNo);
      }
    }
    delay(GLOBAL_WAIT);
  }
}

void setup() 
{
  randomSeed(analogRead(0));
  Serial.begin(9600);      // open the serial port at 9600 bps:
  Serial.setTimeout(100);
  //Serial.println("\n\n\n");
  for (int penNo = 0; penNo < PENSINUSE; penNo += 1) {
    penSetup(penNo);
    datasetPos[penNo] = PENREST;
  }
  delay(2000);
}

void loop() 
{
  checkForRequests();
  doTheThings();
}
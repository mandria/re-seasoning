/*
 _ _    _ _  _  _ _  _ . _  _
| (/_  _\(/_(_|_\(_)| ||| |(_|
                            _|
Written by Mandria - recipient.cc
Released under MIT licence.
*/

///////
// mi mancano:
//          X- (dare una piccola tolleranza prima di muoversi)  -le variazioni dei valori solo al movimento dei pot 

#define DEBUG

//potentiometers vars
const int pot3Pin = A0;
const int pot2Pin = A2;
const int pot1Pin = A3;
int pot1Val = 0;
int pot2Val = 0;
int pot3Val = 0;
int pot1ValOld = 0;
int pot2ValOld = 0;
int pot3ValOld = 0;

//button vars
#include "OneButton.h"
const int buttPin = 12;
OneButton button(buttPin, true);
// menu
//LV0 reading | LV1 temp menu | LV2 hum menu | LV3 repeat and duration | LV4 on off switch functions | LV5 writing parameters to eprom and get back to level 0
int menuLevel = 0;
const int maxLevel = 5;
// relay pins R1-P7 | R2-P6 | R3-P5 | R4-P4
int Fridge = 7;
int Humidifier = 6;
int extFan = 5;
int movFan = 4;
//temerature and humidity
float ft;
float fh;
int t;
int h;
//Treshold and Timer settings
//T and H Threshold
int thresT; // =0?; 2° tollerance suggested
int thresH; // =0?; 3% tollerance suggested
int tollT; //tolerrance temperature
int tollH; //tollerance humidity
//
bool humEn;
bool extrEn;
bool mfanEn;
bool raiseMode=false;
bool movWhum;

//Libraries and imports
//watchdog
#include <avr/wdt.h>
//LCD
#include <Wire.h>
#include <LCD03.h>
LCD03 lcd;
//DHT
#include "DHT.h"
#define DHTPIN 2
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
//EPROM
#include <EEPROM.h>
int eeAddress = 0;
struct Settings{
int thresT;
int thresH;
int tollT;
int tollH;
int MovFanTimer;
int MovFanDur;
int ExtFanDur;
int HumDur;
bool humEn;
bool extrEn;
bool mfanEn;
bool movWhum;
};
//Timers
#include "MillisTimer.h"
//timers
MillisTimer timerFan;
MillisTimer timerMovFanDur;
MillisTimer timerExtFanDur;
MillisTimer timerHumDur;
MillisTimer timerSensRead;
MillisTimer timerBackLit;
MillisTimer timerWatchdog;
#ifdef DEBUG
  MillisTimer timerDebug;
   //Macros are usually in all capital letters.
  #define SPR(...)    Serial.print(__VA_ARGS__)     //DPRINT is a macro, debug print
  #define SPRLN(...)  Serial.println(__VA_ARGS__)   //DPRINTLN is a macro, debug print with new line
#endif
//timer interval internal air (in mS)
int MovFanTimer;
int MovFanDur;
int ExtFanDur;
int HumDur;
unsigned long BackLitDur = 120000;


void setup() {
#ifdef DEBUG
Serial.begin(9600);
Serial.println(F("SETUP ARDUINO IS STARTED"));
Serial.println(F("------------------------"));
#endif
pinMode(Fridge, OUTPUT);
pinMode(Humidifier, OUTPUT);
pinMode(extFan, OUTPUT);
pinMode(movFan, OUTPUT);
dht.begin();  
button.attachClick(click1);
button.attachLongPressStart(longPressStart1);
button.attachLongPressStop(longPressStop1);
button.attachDoubleClick(doubleclick1);
lcd.begin(16, 2);
lcd.backlight();
lcd.print("Loading         Configuration...");
loadSettings();
TimersSetup();
delay(5000);
lcd.clear();
lcd.noBacklight();
wdt_enable(WDTO_8S);
}

void loop() {
  button.tick();
  readPots();
  constMenu();
  Timers();
  FunctionalLoop();
}

//function to display information on menu tabs
void constMenu(){
  /////////////////////visualization data
    if(menuLevel==0){
    lcd.home();
    lcd.print(" TMP  HUM  TIMER");
    if (t < 10){
      lcd.print("0");
      lcd.print(t);
    } else{
      lcd.print(t);
    }
    lcd.print((char)223);
    lcd.print("C  ");
    if (h < 10){
      lcd.print("0");
      lcd.print(h);
    } else{
      lcd.print(h);
    }
    lcd.print("% ");
    if(mfanEn==true){
    unsigned long countdown=timerFan.getRemainingTime();
    int seconds=countdown%60000/1000;
    int minutes=countdown/1000/60;
    if (minutes < 10){
      lcd.print("00");
      lcd.print(minutes);
    } else if(minutes < 100){
      lcd.print("0");
      lcd.print(minutes);
    }else{
      lcd.print(minutes);
    }
    lcd.print(".");
    if (seconds < 10){
      lcd.print("0");
      lcd.print(seconds);
    } else {
      lcd.print(seconds);
    }
    } else {
      lcd.print("   off");
    }
  }
  ///////////////////// TEMPERATURE
  if(menuLevel==1){
    lcd.home();
    lcd.print("TRHT TTOL       ");
    if (pot1Val < 10){
      lcd.print("0");
      lcd.print(thresT);
    } else{
      lcd.print(thresT);
    }
    lcd.print((char)223);
    lcd.print("C ");
    if (pot2Val < 10){
      lcd.print("0");
      lcd.print(tollT);
    } else{
      lcd.print(tollT);
    }
    lcd.print((char)223);
    lcd.print("C  ");
    //set var from pot
    
  }
  /////////////////////// HUMIDITY
  if(menuLevel==2){
    lcd.home();
    lcd.print("TRHH HTOL   HDUR");
    lcd.print(" ");
    if (thresH < 10){
      lcd.print("0");
      lcd.print(thresH);
    } else{
      lcd.print(thresH);
    }
    lcd.print("% ");
    if (tollH < 10){
      lcd.print("0");
      lcd.print(tollH);
    } else{
      lcd.print(tollH);
    }
    lcd.print("%    ");
    if(HumDur<10){
      lcd.print("00");
      lcd.print(HumDur);
    } else if(HumDur<100){
      lcd.print("0");
      lcd.print(HumDur);
    } else{
      lcd.print(HumDur);
    }
    lcd.print("s");
  }
  /////////////////////// set repeating timers and duration
  if(menuLevel==3){
    lcd.home();
    lcd.print("MVTI MVDR MVWHU");
    lcd.print("m");
    if (MovFanTimer < 10){
      lcd.print("00");
      lcd.print(MovFanTimer);
    }else if(MovFanTimer < 100){
      lcd.print("0");
      lcd.print(MovFanTimer);
    } else{
      lcd.print(MovFanTimer);
    }
    lcd.print("  s");
    if (MovFanDur < 10){
      lcd.print("00");
      lcd.print(MovFanDur);
    } else if(MovFanDur < 100){
      lcd.print("0");
      lcd.print(MovFanDur);
    } else{
      lcd.print(MovFanDur);
    }
    lcd.print("  ");
    if (movWhum == 1){
      lcd.print("off");
    }else {
      lcd.print(" on");
    }
  }
  ///////////////////////
  if(menuLevel==4){
    lcd.home();
    lcd.print(" HUM  EXTF  MFAN");
    if(humEn==0){
      lcd.print(" off"); 
    } else {
      lcd.print("  on"); 
    }
    if(extrEn==0){
      lcd.print("   off"); 
    } else {
      lcd.print("    on"); 
    }
    if(mfanEn==0){
      lcd.print("   off"); 
    } else {
      lcd.print("    on"); 
    }
  }
  ///////////////////////
  if(menuLevel==5){
    //lcd.home();
    //lcd.print("writing to eprom");
  }
}
//function to trigger one time action from menu changes
void oneTimeMenu(){
  backlitOn();
  if(menuLevel==0){
    lcd.clear();
    //lcd.home();
    //lcd.print("Level0 reding sensor");
  }
  if(menuLevel==1){
    lcd.clear();
    readPotsInit();
    //lcd.home();
    //lcd.print("setting temp");
  }
  if(menuLevel==2){
    lcd.clear();
    readPotsInit();
    //lcd.home();
    //lcd.print("setting hum");
  }
  if(menuLevel==3){
    lcd.clear();
    readPotsInit();
    //lcd.home();
    //lcd.print("setting repetition");
  }
  if(menuLevel==4){
    lcd.clear();
    readPotsInit();
    //lcd.home();
    //lcd.print("switch on off");
  }
  if(menuLevel==5){
    lcd.clear();
    lcd.home();
    lcd.print("writing settings");
  }
}

void readPotsInit(){
if (menuLevel >= 1 && menuLevel <= 4){

pot1ValOld = analogRead(pot1Pin);
pot2ValOld = analogRead(pot2Pin);
pot3ValOld = analogRead(pot3Pin);

if(menuLevel==1){
  //temperature reading map
  //temp
  pot1ValOld = map (pot1ValOld, 0, 1023, 1, 30);
  //temp toll
  pot2ValOld = map (pot2ValOld, 0, 1023, 1, 10);
}
if(menuLevel==2){
  //Humidity reading map
  //hum %
  pot1ValOld = map (pot1ValOld, 0, 1023, 0, 99);
  //hum toll %
  pot2ValOld = map (pot2ValOld, 0, 1023, 1, 10);
  //hum dur Seconds
  pot3ValOld = map (pot3ValOld, 0, 1023, 1, 120);
  
}
if(menuLevel==3){
  //repeat and duration
  //mov fan repeat timer from 1 to 30 min (x * 60 *1000 = x *60000)
  pot1ValOld = map (pot1ValOld, 0, 1023, 1, 60);
  //mov fan duration in seconds
  pot2ValOld = map (pot2ValOld, 0, 1023, 5, 120);
  //ext fan duration in seconds
  pot3ValOld = map (pot3ValOld, 0, 512, 0, 1);
  
}
if(menuLevel==4){
 //I/O outputs
  //HUM IO
  pot1ValOld = map (pot1ValOld, 0, 512, 0, 1);
  //ETRACT FAN
  pot2ValOld = map (pot2ValOld, 0, 512, 0, 1);
  //MOV FAN
  pot3ValOld = map (pot3ValOld, 0, 512, 0, 1);
   
}
}
}

void readPots(){
if (menuLevel >= 1 && menuLevel <= 4){
pot1Val = analogRead(pot1Pin);
pot2Val = analogRead(pot2Pin);
pot3Val = analogRead(pot3Pin);

if(menuLevel==1){
  //temperature reading map
  //temp
  pot1Val = map (pot1Val, 0, 1023, 1, 30);
  //temp toll
  pot2Val = map (pot2Val, 0, 1023, 1, 10);
  //check if value changed
  if(pot1Val!=pot1ValOld){
  thresT=pot1Val;
  pot1ValOld=pot1Val;
  }
  if(pot2Val!=pot2ValOld){
  tollT=pot2Val;
  pot2ValOld=pot2Val;
  }
  
}
if(menuLevel==2){
  //Humidity reading map
  //hum
  pot1Val = map (pot1Val, 0, 1023, 0, 99);
  //hum toll
  pot2Val = map (pot2Val, 0, 1023, 1, 10);
  //hum dur in seconds
  pot3Val = map (pot3Val, 0, 1023, 1, 120);
  if(pot1Val!=pot1ValOld){
  thresH=pot1Val;
  pot1ValOld=pot1Val;
  }
  if(pot2Val!=pot2ValOld){
  tollH=pot2Val;
  pot2ValOld=pot2Val;
  }
  if(pot3Val!=pot3ValOld){
  HumDur=pot3Val;
  pot3ValOld=pot3Val;
  }
  
}
if(menuLevel==3){
  //repeat and duration
  //mov fan repeat timer from 1 to 30 min (x * 60 *1000 = x *60000)
  pot1Val = map (pot1Val, 0, 1023, 1, 60);
  //mov fan duration in seconds
  pot2Val = map (pot2Val, 0, 1023, 5, 120);
  //ext fan duration in seconds
  pot3Val = map (pot3Val, 0, 512, 0, 1);
  if(pot1Val!=pot1ValOld){
  MovFanTimer=pot1Val;
  pot1ValOld=pot1Val;
  }
  if(pot2Val!=pot2ValOld){
  MovFanDur=pot2Val;
  pot2ValOld=pot2Val;
  }
  if(pot3Val!=pot3ValOld){
  movWhum=pot3Val;
  pot3ValOld=pot3Val;
  }
}
if(menuLevel==4){
 //I/O outputs
  //HUM IO
  pot1Val = map (pot1Val, 0, 512, 0, 1);
  //ETRACT FAN
  pot2Val = map (pot2Val, 0, 512, 0, 1);
  //MOV FAN
  pot3Val = map (pot3Val, 0, 512, 0, 1);
  if(pot1Val!=pot1ValOld){
  humEn=pot1Val;
  pot1ValOld=pot1Val;
  }
  if(pot2Val!=pot2ValOld){
  extrEn=pot2Val;
  pot2ValOld=pot2Val;
  }
  if(pot3Val!=pot3ValOld){
  mfanEn=pot3Val;
  pot3ValOld=pot3Val;
  }
  
}
}
}

void click1() {    
  if(menuLevel <= 3){
  menuLevel++;
  } else{
  menuLevel=0;  
  }
  oneTimeMenu();
} // click1
// This function will be called once, when the button1 is pressed for a long time.
void longPressStart1() {
  menuLevel=5;
  oneTimeMenu();
  saveSettings();
} 

void longPressStop1() {
  menuLevel=0;
  TimersSetup();
}

void doubleclick1(){
  backlitOn();
}

void saveSettings(){
   Settings Configuration{
 thresT,
 thresH,
 tollT,
 tollH,
 MovFanTimer,
 MovFanDur,
 ExtFanDur,
 HumDur,
 humEn,
 extrEn,
 mfanEn,
 movWhum
};
EEPROM.put(eeAddress, Configuration);
lcd.clear();
lcd.print("settings saved");
}

void loadSettings(){
  Settings Configuration;
  EEPROM.get(eeAddress, Configuration);
  thresT=Configuration.thresT;
  thresH=Configuration.thresH;
  tollT=Configuration.tollT;
  tollH=Configuration.tollH;
  MovFanTimer=Configuration.MovFanTimer;
  MovFanDur=Configuration.MovFanDur;
  ExtFanDur=Configuration.ExtFanDur;
  HumDur=Configuration.HumDur;
  humEn=Configuration.humEn;
  extrEn=Configuration.extrEn;
  mfanEn=Configuration.mfanEn;
  movWhum=Configuration.movWhum;
}





void FunctionalLoop(){
  int HTU = thresH+tollH;
  int HTD = thresH-tollH;
  int TT = thresT+tollT;

if(t!=0 && h!=0){
 if (t > TT){
  raiseMode=true;
 } else if(t <= thresT){
  raiseMode=false;
}

if(raiseMode==true){
  digitalWrite(Fridge, HIGH);
} else {
  digitalWrite(Fridge, LOW);
}

if(extrEn==1){
  if (h > HTU){
  digitalWrite(extFan, HIGH);
 } else {
  digitalWrite(extFan, LOW);
 }
  }
  
  if(humEn==1){
if (h < HTD ){
  digitalWrite(Humidifier, HIGH);
  if(movWhum==true){
  digitalWrite(movFan, HIGH);  
  }
 } else {
  digitalWrite(Humidifier, LOW);
  if(movWhum==true){
  digitalWrite(movFan, LOW);
  }
 }
 
  }
}
// if (t > TT){
//  digitalWrite(Fridge, HIGH);
// } else{
//  digitalWrite(Fridge, LOW);
// }
//  if(extrEn==1){
//  if(timerExtFanDur.isRunning()){
//    digitalWrite(extFan, HIGH);
//  } else {
//    digitalWrite(extFan, LOW);
//  }
//  }
//  if(humEn==1){
//   if(timerHumDur.isRunning()){ 
//  digitalWrite(Humidifier, HIGH);
// } else {
//  digitalWrite(Humidifier, LOW); 
// }
}

void tempSens(MillisTimer &mt){
  
  fh = dht.readHumidity();
  ft = dht.readTemperature();
  h=(int)fh;
  t=(int)ft;
  
// if (isnan(h) || isnan(t)) {
//    lcd.home(); 
//    lcd.print("Failed to read from DHT sensor!");
//    return;
//  }
}

void backlitOn(){
  lcd.backlight();
  timerBackLit.reset();
  timerBackLit.start();
  //Serial.print(timerBackLit.getRemainingTime());
}
void backlitOff(MillisTimer &mt){
  timerBackLit.stop();
  lcd.noBacklight();

}
void runMovFan(MillisTimer &mt){
  timerFan.stop();
  timerMovFanDur.reset();
  timerMovFanDur.start();
  digitalWrite(movFan, HIGH);
}
void endRunMovFan(MillisTimer &mt){
  digitalWrite(movFan, LOW);
  timerFan.reset();
  timerFan.start();
}

void endRunExtFan(MillisTimer &mt){
  timerExtFanDur.stop();
  timerExtFanDur.reset();
}
void endRunHum(MillisTimer &mt){
  timerHumDur.stop();
  timerHumDur.reset();
}

void resetWatchdog(MillisTimer &mt){
  wdt_reset();
  #ifdef DEBUG
  SPRLN(F("watchdog resetted in time"));
  #endif
}

#ifdef DEBUG
int freeRam () 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}
void runDebug(MillisTimer &mt){
  SPR(F("DEBUG PRINT - millis: "));
  SPRLN(millis());
  SPR(F("free memory: "));
  SPRLN(freeRam());
  SPR(F("float temperature: "));
  SPR(ft);
  SPR(F(" integer temperature: "));
  SPRLN(t);
  SPR(F("float humidity: "));
  SPR(fh);
  SPR(F(" integer humidity: "));
  SPRLN(h);
  SPR(F("threshold temperature: "));
  SPR(thresT);
  SPR(F(" tollerance temperature: "));
  SPRLN(tollT);
  SPR(F("threshold humidity: "));
  SPR(thresH);
  SPR(F(" tollerance humidity: "));
  SPRLN(tollH);
  SPR(F("humEn : "));
  SPR(humEn);
  SPR(F(" extrEn : "));
  SPR(extrEn);
  SPR(F(" mFanEn : "));
  SPRLN(mfanEn);  
  SPR(F("movFAn is running?: "));
  SPRLN(timerMovFanDur.isRunning());
  SPR(F("free memory byte: "));
  SPRLN(freeRam());
  SPRLN(F("--------------"));  
}
#endif

void TimersSetup(){
  #ifdef DEBUG
  timerDebug.setInterval(4000);
  timerDebug.expiredHandler(runDebug);
  timerDebug.start();
  #endif
  timerWatchdog.setInterval(6000);
  timerWatchdog.expiredHandler(resetWatchdog);
  timerWatchdog.start();
//timer interval internal air (in mS)
//int MovFanTimer;
//int MovFanDur;
//int ExtFanDur;
//int BackLitDur = 15000;
//bool humEn;
//bool extrEn;
//bool mfanEn;
//timerr movement fan duration with conversion from minutes to millis
unsigned long MovFanTimerMillis=MovFanTimer*60000;
timerFan.setInterval(MovFanTimerMillis);
timerFan.expiredHandler(runMovFan);
if(mfanEn==1){
  //Serial.println("moving fan enabled");
  timerFan.reset();
  timerFan.start();
}
//from seconds to millis
unsigned long MovFanDurMillis=(unsigned long)MovFanDur*1000;
timerMovFanDur.setInterval(MovFanDurMillis);
timerMovFanDur.expiredHandler(endRunMovFan);
timerMovFanDur.setRepeats(1);

//extraction duration
unsigned long ExtFanDurMillis=(unsigned long)ExtFanDur*1000;
timerExtFanDur.setInterval(ExtFanDurMillis);
timerExtFanDur.expiredHandler(endRunExtFan);
timerExtFanDur.setRepeats(1);

//humidifier duration
unsigned long HumDurMillis=(unsigned long)HumDur*1000;
timerHumDur.setInterval(HumDurMillis);
timerHumDur.expiredHandler(endRunHum);
timerHumDur.setRepeats(1);

//sensor read
timerSensRead.setInterval(4000);
timerSensRead.expiredHandler(tempSens);
timerSensRead.start();
//Backlight timers
timerBackLit.setInterval(BackLitDur);
timerBackLit.expiredHandler(backlitOff);
timerBackLit.setRepeats(1);
}

void Timers(){
  #ifdef DEBUG
  timerDebug.run();
  #endif
  timerWatchdog.run();
  timerBackLit.run();
  timerSensRead.run();
  timerFan.run();
  timerMovFanDur.run();
  //timerExtFanDur.run();
  //timerHumDur.run();
//  if(timerMovFanDur.isRunning()){
//    //Serial.println("moving fan on");
//    digitalWrite(movFan, HIGH);
//  } else{
//    digitalWrite(movFan, LOW);
//  }
//  if(extrEn==1){
//  if(timerExtFanDur.isRunning()){
//    digitalWrite(extFan, HIGH);
//  } else {
//    digitalWrite(extFan, LOW);
//  }
//  }
//  if(humEn==1){
//   if(timerHumDur.isRunning()){ 
//  digitalWrite(Humidifier, HIGH);
// } else {
//  digitalWrite(Humidifier, LOW); 
// }
//}
//Serial.println(timerFan.getRemainingTime());
}



#include "Timer.h"

Timer t;

const int RED_R = 5;
const int YELLOW_R = 6;
const int GREEN_R = 7;
const int BEEP_R =8;

const int OUTIN_B = 14;
const int STEP_B = 3;
const int START_B = 2;
const int REC_B = 4;

const long long INDOOR_TIME = 90000LL;
const long long OUTDOOR_TIME = 150000LL;
const long long yellowTime = 1000*30;
long long greenTime = 0;

const long preTime = 1000*10; 

int greenID=-1, yellowID=-1, redID=-1;

const long BEEP_TIME = 500;
const long BEEP_INTERVAL = 1000;

const int IDLE_STATE = 0;
const int ABCICLE_STATE = 1;
const int CDCICLE_STATE =2;
int state;

boolean recupero = false;

void setup() {
  initRele();
  initButton(START_B);
  initButton(STEP_B);
  initButton(OUTIN_B);
  initButton(REC_B);

  state = IDLE_STATE;
  toRed();
  
  
  // initialize serial:
  Serial.begin(9600);  
  Serial.println("Centralina avviata");
}

void loop() {
  //aggiorno timer
  t.update();
  
  //NB: il digital read dei bottoni restituisce: 
  //    0 se sono pigiati
  //    1 altrimenti
  
  //Controllo outdoor (2:30) indor (1:30)
  if(!digitalRead(OUTIN_B)) {
    //indor
    greenTime = INDOOR_TIME;
  } else {
    //outdoor
    greenTime = OUTDOOR_TIME;

  }    
  
  //start button control
  startButtonCtrl();
  
  switch(state) {
   case IDLE_STATE:
     toRed(); 
      
     break;
     
   case ABCICLE_STATE:
     stepCtrl();        
     break;
     
     
   case CDCICLE_STATE:
     stepCtrl();
     break;
  }
  

}

void initButton(int b) {
  pinMode(b, INPUT);
  digitalWrite(b, HIGH);  
}

void initRele() {
  for(int i=5; i<13; i++) {
    pinMode(i, OUTPUT);
    digitalWrite(i, HIGH);
  }
}


void toGreen() {
 beep(1);
 
 digitalWrite(GREEN_R, LOW); 
 digitalWrite(YELLOW_R, HIGH); 
 digitalWrite(RED_R, HIGH); 
 
 yellowID = t.after(greenTime, toYellow);
}

void toYellow() {
 digitalWrite(GREEN_R, HIGH); 
 digitalWrite(YELLOW_R, LOW); 
 digitalWrite(RED_R, HIGH); 

 redID = t.after(yellowTime, toRed);
}

void toRed() {
 digitalWrite(GREEN_R, HIGH); 
 digitalWrite(YELLOW_R, HIGH); 
 digitalWrite(RED_R, LOW); 

 if(state == ABCICLE_STATE) {
   beep(2);
   greenID = t.after(preTime, toGreen);
   state = CDCICLE_STATE;
 } else if ( state == CDCICLE_STATE) {
   if(!digitalRead(REC_B)) {
     //richiesta recupero 4 beep 
     beep(4);  
   } else {
     beep(3);
   }
   state = IDLE_STATE;
 }
}

void beep(int count) {
  if(count > 0)
    startBeep();
  
  if(count >1)
    t.every(BEEP_INTERVAL, startBeep, count-1);
}

void startBeep() {
  digitalWrite(BEEP_R, LOW);
  t.after(BEEP_TIME, stopBeep);
}

void stopBeep() {
  digitalWrite(BEEP_R, HIGH);
}

void stopTimers() {
 if(redID >=0)
  t.stop(redID);
 
 if(yellowID >=0)
  t.stop(yellowID);
 
 if(greenID >=0)
  t.stop(greenID);
 
 greenID = yellowID = redID = -1; 
}

void stepCtrl() {
 if(!digitalRead(STEP_B) && digitalRead(RED_R) == HIGH) {
   stopTimers();
   toRed();
 }
}


void startButtonCtrl() {
  static int debounceStart =0;
  static boolean startPush = false; 
  
  if(digitalRead(START_B))
    debounceStart++;
  if(!startPush && !digitalRead(START_B)) {
    startPushed(); 
    startPush= true;
    debounceStart=0;
  }
  if(startPush && debounceStart>500 && digitalRead(START_B))
    startPush=false;
}

void startPushed() {
  Serial.println("start premuto");
  
  if(state == IDLE_STATE) {
     Serial.println("START");
     
     if(!digitalRead(REC_B)) {
       //richiesta recupero accettata
       recupero = true;
       state = CDCICLE_STATE;
     } else {
       recupero = false;
       state = ABCICLE_STATE;
     }

     beep(2);
     greenID = t.after(preTime, toGreen);
  } else if(state == ABCICLE_STATE || state == CDCICLE_STATE) {
     Serial.println("RESET"); 
     stopTimers();
     state = IDLE_STATE;
  }
}
  
  

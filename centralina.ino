#include "Timer.h"

Timer t;

//Pin arduino:
const int RED_R = 12;      //relay rosso
const int YELLOW_R = 11;   //relay giallo
const int GREEN_R = 10;    //relay verde
const int BEEP_R = 8;      //relay sirena

const int OUTIN_B = 13;   //switch outdoor-indoor
const int STEP_B = 3;     //pulsante step
const int START_B = 2;    //pulsante start
const int REC_B = 4;      //switch recupero

const unsigned long long int INDOOR_TIME = 3000LL;    // 1 min e 30 sec
const unsigned long long int OUTDOOR_TIME = 150000LL;  // 2 min e 30 sec

const unsigned long long int yellowTime = 2000LL;     // tempo giallo: 30 sec
unsigned long long int greenTime = OUTDOOR_TIME;                  // tempo verde: da assegnare 

const unsigned long long int preTime = 3000LL; //tempo prima dello start: 10 sec

int greenID=-1, yellowID=-1, redID=-1; // ID per il timer

const unsigned long long int BEEP_TIME = 500LL;      //tempo sirena: 0.5 sec
const unsigned long long int BEEP_INTERVAL = 1000LL; //intervallo sirena: 1 sec

const int IDLE_STATE = 0;    
const int ABCICLE_STATE = 1;
const int CDCICLE_STATE =2;
int state;  //stato della macchina
boolean RED_STATE=0, YELLOW_STATE=0, GREEN_STATE=0, BEEP_STATE=0;

boolean recupero = false; //stato recupero

void setup() {
  
  initRele();
  initButton(START_B);
  initButton(STEP_B);
  initButton(OUTIN_B);
  initButton(REC_B);
  
  //stato iniziale 
  state = IDLE_STATE;
  toRed();
  
  // Inizializzo porta seriale
  Serial.begin(9600);  
  Serial.println(F("Centralina avviata"));
}

void loop() {
  //aggiorno timer
  t.update();

  digitalWrite(GREEN_R, !GREEN_STATE); 
  digitalWrite(YELLOW_R, !YELLOW_STATE); 
  digitalWrite(RED_R, !RED_STATE);
  digitalWrite(BEEP_R, !BEEP_STATE);

  
  //NB: il digital read dei bottoni restituisce: 
  //    0 se sono pigiati
  //    1 altrimenti
  
  //Controllo outdoor (2:30) indor (1:30) 
  //assegno il tempo del verde di conseguenza
  if(!digitalRead(OUTIN_B)) {
    //indor
    greenTime = INDOOR_TIME;
  } else {
    //outdoor
    greenTime = OUTDOOR_TIME;

  }    
  
  //controllo pressione bottone start
  startButtonCtrl();
  
  switch(state) {
   case IDLE_STATE:
     toRed(); 
      
     break;
     
   case ABCICLE_STATE:
     //controllo pressione bottone step
     stepCtrl();        
     break;
     
     
   case CDCICLE_STATE:
     //controllo pressione bottone step
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
 //accende il verde
 GREEN_STATE  = 1;
 YELLOW_STATE = 0;
 RED_STATE    = 0;
 
 //esegue toYellow() dopo il tempo assegnato al verde
 yellowID = t.after(greenTime, toYellow);
 
 if(greenTime == 90000LL)
  Serial.println(F("greentime IN"));
 else if(greenTime == 150000LL)
  Serial.println(F("greentime OUT"));
 else
  Serial.println(F("greentime unknow"));

 beep(1);
}

void toYellow() {
 //accende il giallo
 GREEN_STATE  = 0;
 YELLOW_STATE = 1;
 RED_STATE    = 0; 

 //esegue toRed() dopo il tempo assegnato al giallo
 redID = t.after(yellowTime, toRed); 
 if(redID == -1)
  Serial.println(F("yellow time error"));
 else
  Serial.println(F("yellow time"));
}

void toRed() {
 //accende il rosso
 GREEN_STATE  = 0;
 YELLOW_STATE = 0;
 RED_STATE    = 1; 

 if(state == ABCICLE_STATE) {
   //esegue toGreen() dopo il tempo pre inizio tiri
   greenID = t.after(preTime, toGreen);
   if(greenID == -1)
    Serial.println(F("pre time error"));
   else
    Serial.println(F("pre time"));
   beep(2);

   //passa allo stato CD
   state = CDCICLE_STATE;
   
 } else if ( state == CDCICLE_STATE) {
   //controlla se c'è richiesta di recupero
   if(!digitalRead(REC_B)) {
     //richiesta recupero 4 beep 
     beep(4);  
   } else {
     beep(3);
   }
   //passo allo stato di attesa
   state = IDLE_STATE;
 }
}

//esegue un beep "count" volte
void beep(int count) {
  
  if(count > 0)
    startBeep();
  
  if(count >1) 
    t.every(BEEP_INTERVAL, startBeep, count-1);
}

void startBeep() {
  
  BEEP_STATE = 1;
  if(t.after(BEEP_TIME, stopBeep) == -1)
   Serial.println(F("start error"));
  else
  Serial.println(F("start beep"));
}

void stopBeep() {
  Serial.println(F("stop beep"));
  BEEP_STATE = 0;
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
  //il debounce evita paciughi nella lettura della pressione del bottone
  static int deb =0;
  static boolean startPush = false; 
  
  if(digitalRead(STEP_B)) {
    deb = 0;
    startPush = false;
  }
  
  if(!digitalRead(STEP_B)) {
    deb++;
  }
  
  if(RED_STATE==0 && !startPush && deb>100) {       // && digitalRead(RED_R) == HIGH) {
   Serial.println(F("step"));   
   //e' stato premuto il bottone di step a ciclo avviato
   //interrompo i timers, e passo al rosso
   stopTimers();
   toRed();
   deb = 0;
   startPush = true;
  }
}


void startButtonCtrl() {
  //il debounce evita paciughi nella lettura della pressione del bottone
  static int deb =0;
  static boolean startPush = false; 
  
  if(digitalRead(START_B)) {
    deb = 0;
    startPush = false;
  }
  
  if(!digitalRead(START_B)) {
    deb++;
  }

  if(!startPush && deb > 100) {
    //il bottone e' stato premuto
    startPushed();
    deb = 0;
    startPush = true;
  } 

}

void startPushed() {
  Serial.println(F("start premuto"));
  
  if(state == IDLE_STATE) {
     Serial.println(F("START"));
     
     if(!digitalRead(REC_B)) {
       //richiesta recupero accettata
       recupero = true;
       state = CDCICLE_STATE;
     } else {
       //no recupero, si passo allo stato AB
       recupero = false;
       state = ABCICLE_STATE;
     }

     //dopo il preTime esegue toGreen()
     greenID = t.after(preTime, toGreen);
     if(greenID == -1)
      Serial.println(F("pre time error"));
     else
      Serial.println(F("pre time"));
     beep(2);
     
  } else if(state == ABCICLE_STATE || state == CDCICLE_STATE) {
     //premere il bottone start durante fa resetta sutto allo stato di attesa
     Serial.println(F("RESET")); 
     stopTimers();
     state = IDLE_STATE;
  }
}
  
  

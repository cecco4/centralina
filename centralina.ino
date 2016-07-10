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

const int INDOOR_TIME = 90*4;    // 1 min e 30 sec
const int OUTDOOR_TIME = 150*4;  // 2 min e 30 sec

const int yellowTime = 30*4;     // tempo giallo: 30 sec
int greenTime = OUTDOOR_TIME;                  // tempo verde: da assegnare 
const int preTime = 10*4; //tempo prima dello start: 10 sec

int secs = 0; //timer

int greenID=-1, yellowID=-1, redID=-1; // ID per il timer

const int BEEP_TIME = 500;      //tempo sirena: 0.5 sec
const int BEEP_INTERVAL = 1000; //intervallo sirena: 1 sec

const int RED_STATE = 0;    
const int PRE_STATE = 1;
const int GREEN_STATE = 2;
const int YELLOW_STATE = 3;
int state;  //stato della macchina

const int IDLE_CYCLE = 0;    
const int AB_CYCLE = 1;
const int CD_CYCLE = 2;
int cycle = IDLE_CYCLE;

boolean recupero = false; //stato recupero

void setup() {
  
  initRele();
  initButton(START_B);
  initButton(STEP_B);
  initButton(OUTIN_B);
  initButton(REC_B);
  
  //stato iniziale 
  state = RED_STATE;
  cycle = IDLE_CYCLE;
    
  // Inizializzo porta seriale
  Serial.begin(9600);  
  Serial.println(F("Centralina avviata"));

  t.every(250, tick);
}

void loop() {
  //aggiorno timer
  t.update();

  digitalWrite(GREEN_R, state != GREEN_STATE); 
  digitalWrite(YELLOW_R, state != YELLOW_STATE); 
  digitalWrite(RED_R, state != RED_STATE && state != PRE_STATE);

  
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
  
  switch(state) {
   case RED_STATE:
     red(); 
     break;
     
   case PRE_STATE:
     pre();       
     break;
     
     
   case GREEN_STATE:
     green();
     break;

   case YELLOW_STATE:
     yellow();
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

void red() {

  if(cycle == IDLE_CYCLE) {
    //controllo pressione bottone start
    if(startButtonCtrl()) {
      
      if(digitalRead(REC_B)) {
        Serial.println("START");
        cycle = AB_CYCLE;
        state = PRE_STATE;
        secs = 0;
        beep(2);      
       
      } else {
        //richiesta recupero accettata
        Serial.println("START CON RECUPERO");
        recupero = true;
        cycle = CD_CYCLE;
        state = PRE_STATE;
        secs = 0;
        beep(2); 
      }
    }
    
  } else if(cycle == AB_CYCLE) {
      cycle = CD_CYCLE;
      state = PRE_STATE;
      secs = 0;
      beep(2);
      
  } else if(cycle == CD_CYCLE) {
      cycle = IDLE_CYCLE;
      state = RED_STATE;
      secs = 0;

      if(!recupero)
        beep(3);
      else
        beep(4);
      recupero = false;
  }
}

void pre() {
  
  if(secs > preTime) {
    state = GREEN_STATE;
    secs = 0;
    beep(1);
  }
}

void green() {
  
  if(secs > greenTime) {
    state = YELLOW_STATE;
    secs = 0;
    
  } else if(stepCtrl()) {
    state = RED_STATE;
    secs = 0;    
  }
}

void yellow() {
  
  if(secs > yellowTime || stepCtrl()) {
    state = RED_STATE;
    secs = 0;
  }
}



//esegue un beep "count" volte
void beep(int count) {
  Serial.print(count, DEC);
  Serial.println(" beep");

  if(count > 4)
    count == 4;

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


boolean stepCtrl() {
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
   return true;
   deb = 0;
   startPush = true;
  }
  return false;
}


boolean startButtonCtrl() {
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
    deb = 0;
    startPush = true;
    return true;
  } 

  return false;
}



void tick() {
  if(cycle != IDLE_CYCLE) {
    Serial.print(++secs, DEC);
    Serial.print(".");
  }
 
}


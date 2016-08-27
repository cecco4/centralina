#define __ASSERT_USE_STDERR

#include <assert.h>
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

const int INDOOR_TIME = 90;    // 1 min e 30 sec
const int OUTDOOR_TIME = 210;  // 3 min e 30 sec

const int yellowTime = 30;     // tempo giallo: 30 sec
int greenTime = OUTDOOR_TIME;                  // tempo verde: da assegnare 
const int preTime = 10; //tempo prima dello start: 10 sec

int secs = 0; //timer
int SEC_ID = -1;

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

}

void loop() {
  //aggiorno timer
  t.update();

  if(RED_STATE != 0 || PRE_STATE != 1 || GREEN_STATE != 2 
     || YELLOW_STATE != 3)
     assert(false);

  if(state != GREEN_STATE && state != YELLOW_STATE &&
     state != RED_STATE && state != PRE_STATE) {
    assert(state);
   }

  if(GREEN_R !=10 || YELLOW_R != 11 || RED_R !=12)
    assert(false);

  digitalWrite(GREEN_R, state != GREEN_STATE); 
  digitalWrite(YELLOW_R, state != YELLOW_STATE); 
  digitalWrite(RED_R, state != RED_STATE && state != PRE_STATE);

  
  //NB: il digital read dei bottoni restituisce: 
  //    0 se sono pigiati
  //    1 altrimenti

  if(INDOOR_TIME != 90 || OUTDOOR_TIME != 210)
    assert(false);

  //Controllo outdoor (2:30) indor (1:30) 
  //assegno il tempo del verde di conseguenza
  if(!digitalRead(OUTIN_B)) {
    //indor
    greenTime = INDOOR_TIME;
  } else {
    //outdoor
    greenTime = OUTDOOR_TIME;

  }    

  if(cycle != IDLE_CYCLE && cycle != AB_CYCLE && cycle != CD_CYCLE)
    assert(false);
  
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

      SEC_ID = t.every(1000, tick);
      if(SEC_ID <0)
        assert(false);
      
      Serial.print("timer ");
      Serial.print(SEC_ID, DEC);
      Serial.print("  ");
      
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

      if(SEC_ID <0)
        assert(false);
      Serial.print("timer ");
      Serial.print(SEC_ID, DEC);
      Serial.println(" stop");
      t.stop(SEC_ID);
      SEC_ID = -1;
  }
}

void pre() {
  if(preTime != 10)
    assert(false);
    
  if(secs <0 || secs >preTime)
    assert(false);
  
  if(secs >= preTime) {
    state = GREEN_STATE;
    secs = 0;
    beep(1);
  }
}

void green() {
  if(greenTime != INDOOR_TIME && greenTime != OUTDOOR_TIME)
    assert(false);
    
  if(secs <0 || secs >greenTime)
    assert(false);
  
  if(secs >= greenTime) {
    state = YELLOW_STATE;
    secs = 0;
    
  } else if(stepCtrl()) {
    state = RED_STATE;
    secs = 0;    
  }
}

void yellow() {
  
  if(yellowTime != 30)
    assert(false);
    
  if(secs <0 || secs >yellowTime)
    assert(false);
    
  if(secs >= yellowTime || stepCtrl()) {
    state = RED_STATE;
    secs = 0;
  }
}



//esegue un beep "count" volte
void beep(int count) {
  if(count <= 0)
    assert(false);
    
  Serial.print(count, DEC);
  Serial.println(" beep");

  if(count > 4)
    count == 4;

  if(count > 0)
    startBeep();

  if(BEEP_INTERVAL != 1000)
    assert(false);

  if(count >1) {
    int T_ID= t.every(BEEP_INTERVAL, startBeep, count-1);

   if(T_ID == -1)
      assert(false);
  }
}

void startBeep() {
  if(BEEP_R != 8)
    assert(false);

  if(BEEP_TIME != 500)
    assert(false);
  
  digitalWrite(BEEP_R, LOW);
  int T_ID = t.after(BEEP_TIME, stopBeep);

  if(T_ID == -1)
    assert(true);
}

void stopBeep() {
  if(BEEP_R != 8)
    assert(false);
  
  digitalWrite(BEEP_R, HIGH);
}


boolean stepCtrl() {
  //il debounce evita paciughi nella lettura della pressione del bottone
  static int deb =0;
  static boolean startPush = false; 

  if(deb  <0)
    assert(false);
  if(STEP_B != 3)
    assert(false);
  
  if(digitalRead(STEP_B)) {
    deb = 0;
    startPush = false;
  }
  
  if(!digitalRead(STEP_B)) {
    deb++;
  }
  
  if(!startPush && deb>100) {       // && digitalRead(RED_R) == HIGH) {
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

  if(deb  <0)
    assert(false);
  if(START_B != 2)
    assert(false);
  
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
  if(secs < 0 || secs > 210)
    assert(false);
  
  if(cycle != IDLE_CYCLE) {
    Serial.print(++secs, DEC);
    Serial.print(".");
  }
 
}

// handle diagnostic informations given by assertion and abort program execution:
void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
    // transmit diagnostic informations through serial link. 
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}


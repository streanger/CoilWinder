//"nawijarka" by stranger. Was made from 14-17 of Januray 2017.

/*******encoder********/
#include <RotaryEncoder.h>
RotaryEncoder encoder(A2, A3);


/*******button*********/
const int button = A4;                // the number of button pin                
int buttonState;                      // the current reading from the input pin
int lastButtonState = LOW;            // the previous reading from the input pin
unsigned long lastDebounceTime = 0;   // the last time the output pin was toggled
unsigned long debounceDelay = 50;     // the debounce time; increase if the output flickers
int buttonTrue = LOW;


/*********lcd**********/
#include <ShiftLCD.h>
ShiftLCD lcd(4,2,3);
int cursHor = 4;      //polozenie poziome: 0-15
int cursVert = 0;     //polozenie pionowe: 0-1
//some string here. Actually not in used.
String welcome = "Witaj (...) aby kontynuowac.";
String wybor1 = "Okresl grubosc drutu";
String wybor2 = "Okresl ilosc zwojow";
String wybor3 = "Okresl dlugosc cewki";


/*******steppers*******/
// 5,6,7,8; 9,10,11,12
int stepper1Pins[] = {5, 6, 7, 8};                                      //piny silnika nr1
int stepper2Pins[] = {9, 10, 11, 12};                                   //piny silnika nr2

int stepLow[4][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};          //low torque
int stepHigh[4][4] = {{1,1,0,0},{0,1,1,0},{0,0,1,1},{1,0,0,1}};         //high torque
int stepLowBack[4][4] = {{0,0,0,1},{0,0,1,0},{0,1,0,0},{1,0,0,0}};      //low torque; back
int stepHighBack[4][4] = {{0,0,1,1},{0,1,1,0},{1,1,0,0},{1,0,0,1}};     //high torque; back

//licznik krokow pelnego obrotu
int steps1Value = 60;              // UWAGA! ustaw wartosc odpowiadajaca wybranemu silnikowi
int stepsInRound1 = steps1Value;    //zmienna liczaca kroki silnika1 
//int stepsInRound2 = 24;           //zmienna liczaca kroki silnika2; aktualnie nieuzywana
int goOut = LOW;                    //zmienna pracy silnika; gdy == HIGH -> finito

/* silnik przesuwajacy
 *  258 krokow ~ 38mm
 *  136 krokow ~ 20mm
 *  
 *  ---zalozmy ze
 *  112 ~ 38mm
 *  59 ~ 20mm
 */
 



//time intervals
unsigned long stepper1Start = 0;        
unsigned long stepper1Interval = 60;         //to controll the speed
unsigned long stepper2Start = 0;
//long stepper2Interval = 300; 
int steppersDivider = 1;                     //it can gain values from "pos" variable         
int step1State = LOW;                        //if its HIGH, stepper is changing phase
int step2State = LOW;
int x1 = -1;                                 //zmienna stanu silnika1
int x2 = -1;                                 //zmienna stanu silnika2
int y1=0;                                    //zmienna fazy silnika1
int y2=0;                                    //zmienna fazy silnika2


//speed
const int speedPot = A1; // potencjometr pod³¹czony do pinu analogowego A1
int generalInterval = 200;


//variables and constants
int tab[] = {};   //not used now
int actualDisplay = "Initialization.."; //it gonna change throughout the program
int posTrue;    //pozycja rzeczywista: 0-19
int pos;        //pozycja przesunieta: 1-20
int z=0;        //zmienna stanu przycisku
int clearPos = 0;
String toStart = " <OK>   |    ";
int toGo = LOW;    //to run the steppers

//parameters
int thickness = 0;  //aktualna grubosc drutu
int rounds = 0;       //aktualna liczba zwojow
int layers = 0;       //liczba warstw
static int menuChoice=1;




void setup() {
  pinMode(button, INPUT_PULLUP);                            //button pullup state
  for (int pin=5; pin <13; pin++) {pinMode(pin, OUTPUT);}   //steppers - pin initialization
  
  pinMode(speedPot, INPUT);                                 //potentiometer to set e.g. speed;
  pinMode(13, OUTPUT);
   
  //Serial.begin(57600);
  //Serial.println("SimplePollRotator example for the RotaryEncoder library.");
  PCICR |= (1 << PCIE1);    // This enables Pin  (...) input pins or Port C.
  PCMSK1 |= (1 << PCINT10) | (1 << PCINT11);  // This enables (...) 2 and 3 of Port C.

  /**************LCD******************/
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.println("Initialization...");
  delay(600);
  lcd.clear();
}

  
//obsluga przerwania
ISR(PCINT1_vect) {encoder.tick();} // just call tick() to check the state.

 
/**************main_loop*******************/
void loop() {
  //stepperTest();
  /*Steppers values: thickness, rounds, layers */
  buttonPush();               //odczyt stanu przycisku  
  menu(z);                    //menu; wybor parametrow
  toGo = checkFinito();       //sprawdzenie zezwolenia na prace
  if (toGo == HIGH) {showParameters(); work(thickness, rounds, layers);}   
}
/**************main_loop*******************/



/**************enkoder******************/
//zwraca pozycje enkodera
int enkoderPos() {
  static int pos = 0;
  int newPos = encoder.getPosition();
  if (pos != newPos) {
    Serial.print(newPos);
    Serial.println();
    pos = newPos;
    lcd.clear(); 
  } 

  if (pos > 19) {
    encoder.setPosition(0);
  }
  if (pos < 0) {
    encoder.setPosition(20+pos);
  }
  if (pos > 9) {
    cursHor = 3;
  }
  else {
    cursHor = 4;
  }
  lcd.setCursor(cursHor, cursVert);
  int pos2 = pos+1;
  return pos; 
}
/**************enkoder******************/


/************button_push****************/
//it works the same as inside main loop(), but its clearly at least :)
void buttonPush() {
  int reading = digitalRead(button);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceDelay) {
    if (reading != buttonState) {
      //przycisk zostal wlaczony i wylaczony
      buttonState = reading;
      // jezeli buttonState==HIGH, to znaczy to, ze reading mial stan wysoki
      // a to oznacza, ze zanotowano zmiane stanu z wysokiego na niski
      // puszczono przycisk
      if (buttonState == HIGH) {
        //here is the act
        lcd.clear();
        //lcd.print("udalo sie!");
        z++;
        buttonTrue = HIGH;  //oznacza jednorazowe klikniecie
      }
      else buttonTrue = LOW;   
    }
  }
  lastButtonState = reading;
}
/************button_push****************/



void menu(int menuChoice) {
  pos = enkoderPos()+1;
  switch (menuChoice) {
    case 1:{
      lcdUp();
      lcd.print("Witaj w menu");
      lcd.setCursor(1,1);
      lcd.print("Ustaw parametry");
      break;
    }

    case 2: {
      if (clearPos == 0) {
        encoder.setPosition(0);
        clearPos = 1;
      }
      lcdUp();
      lcd.print("Dlugosc karkasu:");
      lcdDown();
      //thickness = float(pos/10);
      thickness = pos*2;      //parametr
      lcd.print(thickness);
      lcd.setCursor(6,1);
      lcd.print("[mm]");
      break;
      }

    case 3: {
      if (clearPos == 1) {
        encoder.setPosition(0);   //ustawienie domyslnej pozycji
        clearPos = 2;
      }
      lcdUp();
      lcd.print("Liczb zwojow: ");
      lcdDown();
      rounds = pos*4;      //parametr       (it was pos*10 in previous)
      lcd.print(rounds);
      break; 
      }
      
     case 4: {
      if (clearPos == 2) {
        encoder.setPosition(0);   //ustawienie domyslnej pozycji
        clearPos = 3;
      }
      lcdUp();
      lcd.print("Liczba warstw: ");
      lcdDown();
      layers = pos;         //paramter
      lcd.print(pos);
      lcd.setCursor(6,1);
      lcd.print("[n]");
      break; 
      }
    
      case 5: {
      lcd.setCursor(0,0);
      lcd.print("Parametry zadane");
      lcd.setCursor(1,1);
      lcd.print("Kontynuowac?");
      delay(1100);
      z++;
      break;
     }
      case 6: {
      if (clearPos == 3) {
        encoder.setPosition(0);   //ustawienie domyslnej pozycji
        clearPos = 4;
        lcd.clear();
      }
      lcdUp();
      lcd.print("Kontynuj|Zmien");
      lcd.setCursor(1,1);
      if ((pos % 2) == 1){
        toStart = " <OK>   |    ";
      }
      else {
         toStart = "        | <OK>";
      }
      lcd.print(toStart);
      break;
     }

     default: {
      if (clearPos == 4) {
        encoder.setPosition(0);   //ustawienie domyslnej pozycji
        clearPos = 5; lcd.clear();
        } 
      }
   }
}

/*****its almost useless****/
int lcdUp() {lcd.setCursor(1,0);}
int lcdDown() {lcd.setCursor(4,1);}
/*****Almost I said!********/

int convertParameters() {
  //do something
  delay(100);
  int output=1;
  return output;
}

int restoreFlags() {
    //if you redeclare the same value here it changes to local
    z=1;              //zmienna stanu przycisku (do menu glownego)
    toStart = " <OK>   |    ";
    thickness = 0;    //aktualna grubosc drutu
    rounds = 0;       //aktualna liczba zwojow
    layers = 0;
    clearPos = 0;     //zmienna parametrow
    encoder.setPosition(0);
    toGo = LOW;
  //restoring all parameters and flags
}


int checkFinito() {
  if (z==7) {
    if ((z==7) && ((pos % 2) == 1)) {toGo = HIGH;}  //kontynuuj z zadanymi parametrami
    else {restoreFlags();}      //ustaw domyslne parametry i ponow wybor
    }
  return toGo;
  }

//silnik nawijajacy
void stepperMove1(int torque[4][4], int stepperX[4]) { 
    //praca licznika :)
    //stepper1Interval jest zmienna zalezna od parametrow
    unsigned long currentMillis1 = millis();
    if (currentMillis1 - stepper1Start >= stepper1Interval) {
      stepper1Start = currentMillis1;       // ponowny start licznika
      step1State = HIGH;                    // impuls dla silnika1
    }
    //praca silnika ;)
    if (step1State == HIGH){
      stepsInRound1--;   //zmienna liczaca kroki
      x1++;
      if ( x1 > 3){x1=0;}
      step1State = LOW;
      for (int y1=0; y1<=3;y1++) {digitalWrite(stepperX[y1], torque[x1][y1]);} 
  }
}   


//silnik przesuwajacy
void stepperMove2(int torque[4][4], int stepperX[4],int stepper2Interval) { 
    //praca licznika :)
    //stepper1Interval jest zmienna zalezna od parametrow
    unsigned long currentMillis2 = millis();
    if (currentMillis2 - stepper2Start >= stepper2Interval) {
      stepper2Start = currentMillis2;       // ponowny start licznika
      step2State = HIGH;                    // impuls dla silnika1
    }
    //praca silnika ;)
    if (step2State == HIGH){
      //stepsInRound2--;                    //zmienna liczaca kroki; aktualnie nieuzywana
      x2++;
      if ( x2 > 3){x2=0;}
      step2State = LOW;
      for (int y2=0; y2<=3;y2++) {digitalWrite(stepperX[y2], torque[x2][y2]);} 
  }
}   

void work(int thickness,int rounds,int layers) {
  const int roundsTrue = rounds;        //wartosci pierwotne
  //int stepper2Interval = (21-thickness)*100;    
  int reverse = HIGH;
  int goOut = LOW;
  lcd.clear();

  //variables against lcd floating
  int clearLCD1 = 0;
  int clearLCD2 = 0;
  int clearLCD3 = 0;
  int clearLCD4 = 0;
  int clearLCD5 = 0;
  while (goOut == LOW){
    /***to controll inside with using encoder***/
    pos = enkoderPos();
    pos = pos+1;
    if (pos < 1) pos=1;
    steppersDivider = pos;
    //steppersDivider = 1;      //normal
    //steppersDivider = 20;     //Vmax
    stepper1Interval = 100/steppersDivider;                      //it was 200/stepper..
    
    //zmiana DANGER!
    //if (thickness == 5) { thickness=11;}
    
    //int stepper2Interval = (21-thickness)*188/steppersDivider;  

    //int stepNo = (thickness*258)/38;
    int stepNo = (thickness*100)/38;                             //try it experimentally
    int stepper2Interval = (60*roundsTrue*stepper1Interval)/stepNo;      
    /***to controll inside with using encoder***/

    
    /*******lcd section****************/
    if ((stepsInRound1 == 99) && (clearLCD1 == 0)) {lcd.clear(); clearLCD1=1;}
    if ((stepsInRound1 == 9) && (clearLCD2 == 0)) {lcd.clear(); clearLCD2=1;}
    if ((rounds == 99) && (clearLCD3 == 0)) {lcd.clear(); clearLCD3=1;}
    if ((rounds == 9) && (clearLCD4 == 0)) {lcd.clear(); clearLCD4=1;}
    if ((layers == 9) && (clearLCD5 == 0)) {lcd.clear(); clearLCD5=1;}
    lcdUp();
    lcd.print("W trakcie...");
    lcd.setCursor(1,1);
    lcd.print(stepsInRound1);
    //lcd.print(reverse);
    lcd.setCursor(6,1);
    lcd.print(rounds);
    lcd.setCursor(11,1);
    lcd.print(layers);
    /*******lcd section****************/
        
    if (stepsInRound1 < 1){
      stepsInRound1 = steps1Value;    //przywrocenie wartosci krokow na pelny obrot
      rounds--;               //dekrementacja liczby zwojow
      clearLCD1=0;            
      clearLCD2=0;
      }        
    if (rounds < 1) {
      layers--;               //dekrementacja liczby warstw
      rounds = roundsTrue;    //ustawienie pierwotnej wartosci liczby zwojow
      reverse = !reverse;     //zmiana kierunku obrotow silnika2
      clearLCD3=0;            //zmienne czyszczenia ekranu
      clearLCD4=0;            //jak wyzej
      } 
        
    /***********praca silnikow*********************/
    if (layers > 0){stepperMove1(stepHigh,stepper1Pins);} //praca ciagla silnika nr1
    else (goOut = HIGH);          //zakonczenie nawijania; zezwolenie na wyjscie

    //praca silnika nr 2 - prawo/lewo w zaleznosci od aktualnej warstwy
    if (reverse == HIGH){stepperMove2(stepLowBack, stepper2Pins, stepper2Interval);}      
    else {stepperMove2(stepLow, stepper2Pins, stepper2Interval);}                         
    /***********praca silnikow*********************/
  }

  
  //zakonczenie pracy; wyzerowanie zmiennych
  for (int pin=5; pin <13; pin++) {digitalWrite(pin, LOW);}
  lcd.clear();
  lcdUp();
  lcd.print("Zakonczono");
  restoreFlags();
  clearLCD1,clearLCD2,clearLCD3,clearLCD4,clearLCD5 = 0;
  delay(2000);
}


void stepperTest() {
  /************ stepper test *******************/
  buttonPush();
  pos = enkoderPos();
  pos = pos+1;
  int t = pos*10;
  if (t < 5) {t=10;}  //zabezpieczenie przed przeskokiem enkodera
  stepper1Interval = t;
    
  if (z == 1){
    digitalWrite(13, HIGH);
    stepperMove1(stepLow, stepper1Pins);
  }
  else {
    digitalWrite(13, LOW);
    stepperMove1(stepLowBack, stepper1Pins);
  }
  if (z == 2){
    z=0;
  }
  /************ stepper test *******************/
}


void showParameters() {
    //wyswietlenie zadanych parametrow 
    lcd.clear(); lcdUp(); lcd.print("grubosc [0.1mm]");
    lcd.setCursor(7,1); lcd.print(thickness); delay(1500);
    
    lcd.clear(); lcdUp(); lcd.print("zwoje [n]");
    lcd.setCursor(7,1); lcd.print(rounds); delay(1500);

    lcd.clear(); lcdUp(); lcd.print("warstwy [n]"); 
    lcd.setCursor(7,1); lcd.print(layers); delay(1500);
    }

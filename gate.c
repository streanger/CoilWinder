//button debounce in function, by stranger. 21.01.2017

/*******button*********/
const int button1 = 2;                 // the number of button pin                
int buttonState1;                      // the current reading from the input pin
int lastButtonState1 = LOW;            // the previous reading from the input pin
unsigned long lastDebounceTime1 = 0;   // the last time the output pin was toggled
int button1Flag = LOW;

const int button2 = 5;                 // the number of button pin                
int buttonState2;                      // the current reading from the input pin
int lastButtonState2 = LOW;            // the previous reading from the input pin
unsigned long lastDebounceTime2 = 0;   // the last time the output pin was toggled
int button2Flag = LOW;

//for both
unsigned long debounceDelay = 50;      // the debounce time
int z =0;                              // zmienna pomocnicza


/*******leds*******/
const int led1 = 8;
const int led2 = 3;
int led1State = LOW;
int led2State = LOW;


/*******steppers*******/
int stepperPins[] = {9, 10, 11, 12};                                //piny silnika nr1

int stepLow[4][4] = {{1,0,0,0},{0,1,0,0},{0,0,1,0},{0,0,0,1}};      //low torque
int stepHigh[4][4] = {{1,1,0,0},{0,1,1,0},{0,0,1,1},{1,0,0,1}};     //high torque
int stepLowBack[4][4] = {{0,0,0,1},{0,0,1,0},{0,1,0,0},{1,0,0,0}};  //low torque; back
int stepHighBack[4][4] = {{0,0,1,1},{0,1,1,0},{1,1,0,0},{1,0,0,1}}; //high torque; back

//licznik krokow pelnego obrotu
int fullSteps = 258;              // UWAGA! (...) wybranemu silnikowi (258!)
const int limit = fullSteps+1;          
int actualStep = fullSteps;    //zmienna liczaca kroki silnika1 
//int goOut = LOW;             //zmienna pracy silnika

//time intervals
unsigned long stepperStart = 0;        
unsigned long stepperInterval = 20;         //to controll the speed     
int stepState = LOW;                        //if its HIGH, stepper is changing phase
int x = -1;                                 //zmienna stanu silnika1
int y=0;                                    //zmienna fazy silnika1


//speed regulation
int regulator = A4;
int regulationValue;


//kierunek pracy silnik
/*  
 *   4 - otwieranie
 *   2 - zamykanie
 *   3 - stop
 *   (2 i 4 mog¹ byæ odwrotnie :) )
 */
int direct = 3;       //stop



void setup() {
  pinMode(button1, INPUT_PULLUP);
  pinMode(button2, INPUT_PULLUP);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  pinMode(regulator, INPUT);
  for (int pin=9; pin<13; pin++) {pinMode(pin, OUTPUT);}  //stepper1 output
}

void loop() {
  buttonPush1();        //button1 value
  buttonPush2();        //button2 value
  regulation();         //regulation of speed
  switchDirect();       //switching direct & work
}


//button
void buttonPush1() {
  int reading1 = digitalRead(button1);
  if (reading1 != lastButtonState1) {
    lastDebounceTime1 = millis();
  }
  if ((millis() - lastDebounceTime1) > debounceDelay) {
    if (reading1 != buttonState1) {
      buttonState1 = reading1;
      if (buttonState1 == HIGH) {
        z++;                          //zmienna pomocnicza
        direct--;         //kierunek zmaykania bramy
        button1Flag = !button1Flag;   //flaga przycisku
      }
    }
  }
  lastButtonState1 = reading1;
  //return button1Flag;
}

void buttonPush2() {
  int reading2 = digitalRead(button2);
  if (reading2 != lastButtonState2) {
    lastDebounceTime2 = millis();
  }
  if ((millis() - lastDebounceTime2) > debounceDelay) {
    if (reading2 != buttonState2) {
      buttonState2 = reading2;
      if (buttonState2 == HIGH) {
        z++;                          //zmienna pomocnicza
        direct++;     //kierunek zmaykania bramy
        button2Flag = !button2Flag;   //flaga przycisku
      }
    }
  }
  lastButtonState2 = reading2;
  //return button2Flag;
}


void switchDirect() {
  if (direct < 3) { direct=2;}
  else if (direct > 3) { direct=4;}
  switch (direct) { 
    //open
    case 2: {
      if (actualStep < limit){
        stepperMove(stepLow, stepperPins);
      }
      //digitalWrite(led1, HIGH);
      //digitalWrite(led2, LOW);
      break;
    }
    //close
    case 4: {
      if (actualStep > 0){
       stepperMove(stepLowBack, stepperPins);
      //digitalWrite(led1, LOW);
      //digitalWrite(led2, HIGH); 
      } 
      break;    
    }

    //stop
    case 3: {
      //stepperStop();
      digitalWrite(led1, LOW);
      digitalWrite(led2, LOW); 
      break;    
    }
    //stop too
    default: {
      digitalWrite(led1, LOW);
      digitalWrite(led2, LOW); 
      break;         
    }
  }
}



//steppers move
void stepperMove(int torque[4][4], int stepperX[4]) { 
    unsigned long currentMillis = millis();
    if (currentMillis - stepperStart >= stepperInterval) {
      stepperStart = currentMillis;       // ponowny start licznika
      stepState = HIGH;                    // impuls dla silnika1
    }
    
    if (stepState == HIGH){
      if (direct == 2) {
        actualStep++;   //zmienna liczaca kroki
        led2State = !led2State;
        digitalWrite(led2, led2State);
      }
      else if (direct == 4) {
        actualStep--;
        led1State = !led1State;
        digitalWrite(led1, led1State);
      }
      
      x++;
      if ( x > 3) {x=0;}
      for (int y=0; y<=3;y++) {digitalWrite(stepperX[y], torque[x][y]);
      stepState = LOW;}  
  }
}  

void regulation() {
  regulationValue = analogRead(A4);
  stepperInterval = map(regulationValue,0,1024,5,55);
}


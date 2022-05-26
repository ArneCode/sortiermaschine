/**
 * @file sketch.ino
 * @brief Hauptdatei
 */
#include "customServo.h"
#include "animLcd.h"
#include "callHandler.h"
#include "header.h"
/**
 * @brief Gibt an, wie lange die Ladeanimation beim "Hochfahren" dauert
 * 
 */
const int LOADING_DURATION = 3000;
const int ANGLE_LEFT_HOLE = 180;
const int ANGLE_RIGHT_HOLE = 90;
const int ANGLE_CENTER = 130;
const int ANGLE_MIN = 45;
const int PIN_SERVO = 6;
const int PIN_STOPBUTTON = 13;
const int PIN_RED = 11;
const int PIN_GREEN = 10;
const int PIN_BLUE = 9;
const float SERVO_SPEED_DEFAULT = 0.01f;
const float SERVO_SPEED_FAST = 0.5f;
AnimatableLcd lcd(0x27, 16, 2);
CallHandler callHandler;
CustomServo servo;

int nWhite = 0;
int nBlack = 0;
int nOrange = 0;
bool doFlicker=false;



class ButtonHandler { //handels button clicks
    int pin;
    bool isPressed = false;
  public:
    void (*onclick)();
    ButtonHandler() {}
    ButtonHandler(int pin, void (*onclick)()): pin(pin), onclick(onclick) {
      pinMode(pin, INPUT_PULLUP);
    }
    void update() {
      bool isPressedNew = digitalRead(pin) == HIGH;
      if (isPressedNew != isPressed) { //is not being pressed now, but was being pressed
        if (isPressed) {
          Serial.println("click");
          onclick();
        }
      }
      isPressed = isPressedNew;
    }
};
enum Farbe {
  WHITE,
  BLACK,
  ORANGE,
  NOTHING
};
Farbe mesureColor(); //sonst erkennt Arduino Farbe nicht als typ an (https://forum.arduino.cc/t/syntax-for-a-function-returning-an-enumerated-type/107241)
Farbe mesureColor() {
  return ORANGE;//inputs hardcoden
  int hue = random(0,1000);//inputs simulieren
  //int hue=analogRead(A0);//tatsächlich Farbe messen
  if (hue <= 100) {
    return ORANGE;
  }
  if (hue < 500) {
    return WHITE;
  }
  if (hue < 830) {
    return NOTHING;
  }
  return BLACK;
}
/**
 * @fn
 * @brief Setzt die Farbe der RGB-Led
 * 
 * @param r Rot (0-255)
 * @param g Grün (0-255)
 * @param b Blau (0-255)
 */
void setLedColor(unsigned char r,unsigned char g, unsigned char b){ //unsigned char: 0-255
  analogWrite(PIN_RED,r);
  analogWrite(PIN_GREEN,g);
  analogWrite(PIN_BLUE,b);
}
/**
 * @brief wird ausgeführt wenn der Stop-Knopf gedrückt wird
 * 
 */
void stopButtonClicked() {
  static bool isStopped = false;//wird nur einmal initialisiert
  isStopped = !isStopped;
  if (isStopped) {
    Serial.println("stopping servo");
    servo.stop();
    callHandler.deleteCalls();
    /*static*/ auto call = new Callable*[1] {
      new LcdDotAnim("gestoppt, warte auf start", &lcd, 100000000000000) //ja, sollte ich vermutlich besser implementieren
    };
    callHandler.setCalls(call, 1);
    setLedColor(255,0,0);
    doFlicker=false;
  } else {
    callHandler.running = false;
  }
}
/**
 * @brief der Stop Knopf
 * 
 */
ButtonHandler stopButton;
/**
 * @brief hilf Funktion, Methoden können nicht als functionsparameter benutzt werden
 * 
 * @return true 
 * @return false 
 */
bool servoIsDone() {
  return servo.isDone();
}
/**
 * @brief wird am Anfang des Programms aufgerufen
 * 
 */
void setup() {
  Serial.begin(9600);
  Serial.println("setup");
  servo.attach(PIN_SERVO);
  lcd.init();
  servo.writeDirect(ANGLE_CENTER);
  callHandler.setCalls(new Callable*[1] {
    new LcdLoadingAnim("Lade", &lcd, LOADING_DURATION),
  }, 1);
  randomSeed(analogRead(A1));
  stopButton = ButtonHandler(PIN_STOPBUTTON, &stopButtonClicked);
}
#define GEH_ZURUECK \
  new LcdDotAnim("Gehe zur\365ck",&lcd),\
  new FuncCall([](){\
    servo.setSpeed(SERVO_SPEED_FAST);\
    servo.write(ANGLE_CENTER);\
  },&servoIsDone)

/**
 * @brief wird immer wieder ausgeführt
 * 
 */
void loop() {
  callHandler.update();
  lcd.update();
  servo.updatePos();
  stopButton.update();
  if(doFlicker){
    unsigned short b=(millis()/2)%400;//helligkeit: 0-512
    if (b>200){
      b=511-b; //wenn b größer als 255, wird die helligkeit kleiner
    }
    setLedColor(b,b/2,0);//orange
    if(random(3)==0){
      lcd.noBacklight();
    }else{
      lcd.backlight();
    }
  }
  if (callHandler.running) {
    return;
  }
  Farbe farbe = mesureColor();
  servo.setSpeed(SERVO_SPEED_DEFAULT);
  switch (farbe) {
    case WHITE:
      {
        nWhite++;
        Serial.println("white");
        callHandler.deleteCalls();
        /*static*/ auto callsWhite = new Callable*[6] {
          //static so that the space for the calls is only allocated once (https://cpp4arduino.com/2018/11/06/what-is-heap-fragmentation.html), 
          //didn't end up being necessary because at there is only one object in heap at one point in time (callHandler.deleteCalls())
          // new so that it is allocated on the heap
          //auto automatically sets the type, in this case Callable*[] (Callable**)
          new LcdString("Ball erkannt, vorsicht", &lcd, 1000), //objects get upcasted to Callable*
          new LcdDotAnim("Wei\342er Ball, drehe Links", &lcd, 0),
          new FuncCall([]() {
            servo.write(ANGLE_LEFT_HOLE);
          }, &servoIsDone),
          new LcdString("Angekommen", &lcd, 1000),
          GEH_ZURUECK //2 elemente
        };
        callHandler.setCalls(callsWhite, 6); //if the number is too large the program crashes
        setLedColor(255,255,255);
        break;
      }
    case BLACK:
      {
        nBlack++;
        Serial.println("black");
        callHandler.deleteCalls();
        /*static*/ auto callsBlack = new Callable*[6]  {
          new LcdString("Ball erkannt, vorsicht", &lcd, 1000),
          new LcdDotAnim("Schwarzer Ball, drehe Rechts", &lcd, 0),
          new FuncCall([]() {
            servo.write(ANGLE_RIGHT_HOLE);
          }, &servoIsDone),
          new LcdString("Angekommen", &lcd, 1000),
          GEH_ZURUECK
        };
        callHandler.setCalls(callsBlack, 6);
        setLedColor(0,0,255);
        break;
      }
    case NOTHING:
      {
        Serial.println("nothing");
        callHandler.deleteCalls();
        /*static*/ auto callsNothing = new Callable*[1] {
          new LcdString(String("Ball einlegen W:") + nWhite + String(" S:") + nBlack + String(" O:") + nOrange, &lcd, 1000)
        };
        callHandler.setCalls(callsNothing, 1);
        setLedColor(0,255,0);
        break;
      }
    case ORANGE:
      {
        doFlicker=true;
        nOrange++;
        Serial.println("orange");
        callHandler.deleteCalls();
        /*static*/ auto callsOrange = new Callable*[9] {
          new LcdDotAnim("\xC0ra\10g\xD9ner Ba\xEDl", &lcd, 0), //https://arduino.stackexchange.com/a/46833
          new FuncCall([]() {
            servo.write(ANGLE_RIGHT_HOLE);
          }, &servoIsDone),
          new LcdString("Fehler \11rkannt", &lcd, 1000),
          new FuncCall([]() {
            servo.setSpeed(SERVO_SPEED_FAST);
            servo.write(ANGLE_MIN);
          }, &servoIsDone),
          new FuncCall([]() {
            servo.setSpeed(SERVO_SPEED_DEFAULT);
          }, &servoIsDone),
          new FuncCall([]() {
            doFlicker=false;
            setLedColor(0,255,0);
            lcd.backlight();
          }),
          new LcdString("Fehler beseitigt", &lcd, 2000),
          GEH_ZURUECK
        };
        callHandler.setCalls(callsOrange, 9);
        break;
      }
  }
}
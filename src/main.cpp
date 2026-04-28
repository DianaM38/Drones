#include <Servo.h>

Servo esc1,esc2,esc3,esc4;

#define ESC1_PIN D3
#define ESC2_PIN D4
#define ESC3_PIN D5  // GPIO14
#define ESC4_PIN D6  // GPIO12

void setup() {
  esc1.attach(ESC1_PIN, 1000, 2000);
  esc2.attach(ESC2_PIN, 1000, 2000);
  esc3.attach(ESC3_PIN, 1000, 2000);
  esc4.attach(ESC4_PIN, 1000, 2000);

  // Armado de ambos ESCs
  esc1.writeMicroseconds(1000);
  esc2.writeMicroseconds(1000);  
  esc3.writeMicroseconds(1000);
  esc4.writeMicroseconds(1000);
  delay(3000); // Espera beeps
}

void avanzar(){


}


void retroceder(){



}


void arriba(){

}

void abajo(){
  
}


void loop() {
  // 50% throttle
  esc1.writeMicroseconds(1500);
  esc2.writeMicroseconds(1500);
  delay(3000);

  // Full
  esc1.writeMicroseconds(2000);
  esc2.writeMicroseconds(2000);
  delay(3000);



  // Stop
  esc1.writeMicroseconds(1000);
  esc2.writeMicroseconds(1000);
  delay(3000);
}
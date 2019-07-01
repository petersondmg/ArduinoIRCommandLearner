#include <IRremote.h>
#include <EEPROM.h>

// programming mode button
int BTN_PIN = 12;
// ir receiver pin
int RECV_PIN = 2;

IRrecv irrecv(RECV_PIN);
decode_results results;

// programming state control
// 0 - not programming
// 1 - programming main button
// 2 - programming action 1 button
// 3 - programming action 2 button
int prog = 0;

// irrelevant kk
int in = 0;
int i;

// programming button state
bool pressed;

// blinking LED state control
bool blk = HIGH;

// last result read by readir()
// returns false if has now value or value is -1
long irvalue;

// is true after main button's command is received.
// if no command is received in 2s after main button press
// or a command different than action 1 and action 2 is received
// it returns to false
bool waitmode;

// aux to measure spent time
unsigned long wnow = 0;

struct Memory {
  bool saved;
  long main;
  long aux1;
  long aux2;
};

Memory data;

int eeAddress = 0;

// blink LED 3 times to indicate success
void ok() {
  for (i = 0; i < 6; i++) {
    digitalWrite(LED_BUILTIN, !blk);
    blk = !blk;
    delay(200);
  }
  delay(500);
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, blk);
  pinMode(BTN_PIN, INPUT);
  Serial.begin(9600);
  irrecv.enableIRIn();

  EEPROM.get(eeAddress, data);

  Serial.print("saved:");
  Serial.println(data.saved);
  Serial.println(data.main, DEC);
  Serial.println(data.aux1, DEC);
  Serial.println(data.aux2, DEC);
}

// read IR command code and stores value into irvalue global.
// returns false if has no value or value is -1
bool readir() {
  if (irrecv.decode(&results)) {
    irvalue = results.value;
    irrecv.resume();
    if (irvalue != -1) {
      return true;
    }
  }
  return false;
}

// handle actions for aux commands
void wait_cmd() {
  if (wnow != 0 && millis() - wnow >= 2000) {
    waitmode = 0;
    wnow = 0;
    return;
  }
  wnow = millis();
  if (!readir()) {
    return;
  }
  if (irvalue == data.aux1) {
    // ACTION 1 
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
  }

  if (irvalue == data.aux2) {
    // ACTION 2
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(200);
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);

  }

  waitmode = 0;
  wnow = 0;
}

void loop() {
  if (waitmode) {
    wait_cmd();
    return;
  }

  pressed = digitalRead(BTN_PIN) == HIGH ? true : false;

  if (prog == 0 && pressed) {
    prog++;
    wnow = 0;
    return;
  }

  // always blink when programming
  if (prog != 0 && (wnow == 0 || millis() - wnow >= 1000)) {
    wnow = millis();
    digitalWrite(LED_BUILTIN, !blk);
    blk = !blk;
  }

  switch (prog) {
    // main btn
  case 1:
    if ( in != 1) {
      Serial.println("prog 1"); in = 1;
    }
    if (readir()) {
      Serial.print("irvalue main:");
      Serial.println(irvalue, DEC);
      data.main = irvalue;
      ok();
      prog++;
      return;
    }

    break;

    // aux btn 1
  case 2:
    if ( in != 2) {
      Serial.println("prog 2"); in = 2;
    }

    if (readir()) {
      Serial.print("irvalue aux1:");
      Serial.println(irvalue, DEC);
      if (irvalue == data.main) {
        return;
      }
      data.aux1 = irvalue;
      ok();
      prog++;
      return;
    }
    break;
    // aux btn 2
  case 3:
    if ( in != 3) {
      Serial.println("prog 3"); in = 3;
    }
    if (readir()) {
      Serial.print("irvalue aux2:");
      Serial.println(irvalue, DEC);
      if (irvalue == data.main || irvalue == data.aux1) {
        return;
      }
      data.aux2 = irvalue;
      ok();
      data.saved = true;
      EEPROM.put(eeAddress, data);
      prog = 0;
      ok();
      digitalWrite(LED_BUILTIN, HIGH);
      blk = HIGH;
      return;
    }
    break;
  }

  if (data.saved && prog == 0 && readir() && irvalue == data.main) {
    digitalWrite(LED_BUILTIN, LOW);
    delay(200);
    digitalWrite(LED_BUILTIN, HIGH);
    waitmode = true;
  }

}

/*
   So IR comunnications works with sending LOW or HIGH signals for some period of time.
   Example of simple IR protocol:
       +-----------------+   +-+    +----+    +-+
       |                 |   | |    |    |    | |
       |                 |   | |    |    |    | |
   +---+                 +---+ +----+    +----+ +----+
           STOP SIGNAL        0        1       0
   
   Of course other protocol can be much harder and differs in any way.


   Data Log:
   11111110000010000001110110000100 max Speed
    1101010000010000001110010100000 midle Speed

   11111110000001111001110010101100 - XY CENTER : UP
   11111110000011111001110010100100 - XY CENTER : DOWN
   11111110000010000001110010100011 - XY CENTER
   11111101111110000001110010100101 - XY RIGHT : CENTER
   11111111111110000001110010100011 - XY LEFT : CENTER

   11111101111101001001110010100001 - Button1 Off
   11111101111101001001110010110000 - Button1 On
*/

const int pinIR = 3;
// for test reason. Standard package is 32bit
const int CODE_LEN = 120;
volatile unsigned long durations[CODE_LEN];
volatile boolean bounced = 0;
// Stop signal that used to separate packages (in real it equal ~64k microseconds)
volatile int STOP = 20000; // microseconds

void setup() {
  pinMode(pinIR , INPUT);
  //setting interupt signal when IR signal goes from HIGH to LOW 
  attachInterrupt(digitalPinToInterrupt(pinIR), decodeIR, FALLING);
  //Linking Serial port for logging
  Serial.begin(9600);
}

// Interrupt function
void decodeIR() {
  //if already receiving then return
  if (bounced) {
    return;
  }
  bounced = 1;
  // preventring from other interrupts, disabling interrupts
  noInterrupts();
  int i;
  //for each "bit" in the code
  for (i = 0; i < CODE_LEN; i += 1) {
    //store the duration of the HIGH pulse for 100 000 microseconds
    durations[i] = pulseIn(pinIR, HIGH, 100000);
  }
  //enable interrupts
  interrupts();
}

void loop() {
  // reading data & processing
  if (bounced == 1) {
    int i;
    int SPEED, X, Y, Button1;
    unsigned long res = 0;
    boolean started = false;
    Serial.println("Starting");
    for (i = 0; i < CODE_LEN; i += 1) {
      if (started) {
        //if we got signal STOP signal, then start parsing
        if (durations[i] > STOP) {
          // if getting STOP signal, then stop parsing, package ready to proccess
          started = false;
          break;
        } else if (durations[i] > 400) {
          // if duration > 400 microseconds then set 1 bit
          res = (res << 1) + 1;
        } else if (durations[i] < 400) {
          // if duration < 400 microseconds then set 0 bit
          res = (res << 1);
        }
      } else {
        if (durations[i] > STOP) {
          // iterating until we get STOP signal
          started = true;
        }
      }
    }
    // Based on logged data that we got while pulling different triggers on remote controll 
    Serial.print("Speed:");
    SPEED = res >> 26;
    Serial.println(SPEED);
    Serial.print("Y:");
    //  Sign                               Value
    Y = (-2 * ((res & 0xF8000) >> 19) + 1) * ((res & 0x78000) >> 15);
    Serial.println(Y);
    //  Sign                                 Value
    X = (-2 * ((res & 0x2000000) >> 25) + 1) * ((res & 0x1F00000) >> 20);
    Serial.println(X);
    Button1 = (res & 0x10) >> 4;
    Serial.println(Button1);

    Serial.println(res, BIN);
    bounced = 0;
  }
}



/* 
 * Automate RGB LED, feeder motor, reflective feeder sensor, and wide beam sensor.
 *  -Turn blue LED on (using blue instead of red because the wide beam sensor shows red lights)
 *  -Wait 8 seconds or until feeder sensor is triggered, whichever comes first 
 *  -Save that time value
 *  -Feed fish (step motor using MOSFET switch)
 *  -Flash white LED 5 times 
 *  -Wait until fish swims away and triggers the wide beam sensor 
 *  -Wait some interval of time
 *  -Repeat from beginning for x trials
 *  -Export data manually
 *
 * If the libraries on lines 1-4 don't import correctly, 
 * go to Sketch / Include Library / Add .ZIP Library and select 
 * the IR_Libraries ZIP folder.
 * 
 * Tamara O'Malley, Monica Gray
 * 08 August 2017
 * Updated 8/10/17 - changed LED from red to blue
 * Updated 8/11/17 - added variable for time delay between trials
 */

// set up variables for use in entire program

int placeholder = 1;

//RGB LED (common anode: R+GB)
int LED_R = 9;                      //the number of the RGB LED Red pin for the feeding light
int LED_G = 10;                     //the number of the RGB LED Green pin for the feeding light
int LED_B = 11;                     //the number of the RGB LED Blue pin for the feeding light
        
int LED_on = 0;                     //0 = ON at full brightness
int LED_off = 255;                  //255 = OFF

//FEEDER SENSOR (Keyence FU-23X reflective fiber sensor with FS-V21X IR amplifier)
boolean fish_got_food = false;    //whether the fish has gotten close enough to the feeder to trigger the sensor (we assume it ate)
boolean fish_through_gate = false;
int fs_count = 0;                   //tracks how many times the feeder sensor has been read; used to slow down printing of "waiting" statement 

//WIDE BEAM SENSOR (Keyence FU-A100 wide beam fiber sensor with FS-V21X IR amplifier)
int wideBeam_sensor_value = analogRead(A2);
int wideBeam_sensor_threshold = 50; //the level of output from the wide beam sensor that indicates the fish has passed through it
boolean fish_in_feeding_area = false;          //whether the fish has gone away from the feeding area, through the wide beam sensor
int wbs_count = 0;                  //tracks how many times the wide beam sensor has been read; used to slow down printing of "waiting" statement

//FEEDER controlled by MOSFET switch (Med Associates ENV-203-14P 14 mg Pellet Dispenser)
int mosfetPin = 5;                  //the number of the MOSFET switch pin 

//TIMER variables
float gate_sensor_time = 0;
float feed_sensor_time = 0;
float start_time_secs = 0;
long randNumber = 0;


void setup() {
//begin SETUP section
  
  Serial.begin(9600);           //allow printing to the serial monitor
  randomSeed(analogRead(0));
  
  pinMode(LED_R, OUTPUT);       //set up pins connected to RGB leads for the feeding sensor
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  RGB_off();

  analogReference(INTERNAL);    //set up pins to read sensors
  pinMode(A0, INPUT);           //feeder sensor
  pinMode(A2, INPUT);           //wide beam sensor

  pinMode(mosfetPin, OUTPUT);   //set up pin connected to MOSFET switch controlling the feeder
  digitalWrite(mosfetPin, LOW); //initialize feeder to OFF  

  Serial.println("All unitless numbers were measured in seconds.");
}


void loop()                                 
{   
    fish_in_feeding_area = false;
    fish_through_gate = false;
    fish_got_food = false;                    //we'll need this later

    delay(500);                             //This waits .5 seconds so that a fish who is leaving the feeding area after waiting in there longer than the trial length won't immediately trigger the gate in the new trial
    
    start_time_secs = millis() / 1000.00;   //This records the uptime of the machine when the current loop was started. it needs to be saved as seconds to avoid overflow (i think? I'm taking precautions)
  
    //randNumber = random(300, 1000);           //random(minimum seconds, maximum seconds)
    randNumber = 30;
    Serial.print("\nThe following data were gathered in a trial with a length of "); Serial.println(randNumber);




    RGB_on(LED_off, LED_on, LED_off);               //green light on
    delay(500);                                     //this makes sure that the fish passing back through the gate after the previous trial doesn't trip the sensor
    while ( (millis()/1000.00 - start_time_secs) < 8 ) {
      check_for_fish();
      check_for_fish_leaving();
    }
    RGB_off();                                      //green light off



    for (int i=0; i <= 20; i++){          //This for loop flashes the light once, and then checks if the fish is near the sensor after each flash
      check_for_fish();  
      RGB_on(LED_on, LED_on, LED_on);     //White LED on
      check_for_fish();
      check_for_fish_leaving();  
      delay(75);                          //wait
      check_for_fish();
      check_for_fish_leaving();  
      RGB_off();                          //White LED off
      check_for_fish();
      check_for_fish_leaving();      
      delay(75);                          //wait
      check_for_fish();
      check_for_fish_leaving();
    }


 
    while (  millis()/1000 - start_time_secs <= randNumber ) { //essentially this should keep the machine checking to see if the sensor has been tripped for 59 seconds after the cycle started. i gave it a second to get back to the top of the code (does it need that? i dunno but i'm being careful)
      Serial.print(analogRead(A0)); Serial.print("\t"); Serial.println(analogRead(A2));
      check_for_fish();
      check_for_fish_leaving();
    }

    
    if (fish_got_food == false) {
      Serial.println("Fish did not approach feeding sensor");
    }

    while (fish_in_feeding_area == true) {
      check_for_fish_leaving();
    }
    
}


/* 
 * Function to turn on LED at desired brightness and color
 *
 * RGB_on(red brightness number, green brightness number, blue brightness number);
 *     
 *  -brightness number: 
 *      can enter an integer or use the "LED_on" variable
 *        on = 0 (very bright)
 *        off = 255
 *      mix and match to make different colors
 *        no light = all 3 numbers OFF (255)
 *        white = all 3 numbers ON @ same brightness
 *        red = red ON, green OFF, blue OFF
 *        etc
 */
void RGB_on(int redBrightness, int greenBrightness, int blueBrightness) 
{
  analogWrite(LED_R, redBrightness);     
  analogWrite(LED_G, greenBrightness);
  analogWrite(LED_B, blueBrightness);
  delay(50);
}



/* 
 * Function to turn on LED off
 *
 * RGB_off();
 */
void RGB_off()
{
  analogWrite(LED_R, LED_off);         
  analogWrite(LED_B, LED_off);           
  analogWrite(LED_G, LED_off);    
  delay(50);
}


/* 
 * Function to dispense food
 * 
 * dispense_food();
 * 
 *  -MOSFET switch controls feeder:
 *     digitalWrite(mosfetPin, HIGH) --> activates switch to dispense food
 *     digitalWrite(mosfetPin, LOW) --> resets switch
 *    
 *  -feeder needs at least 500 ms between signals
 */
void dispense_food() 
{
  digitalWrite(mosfetPin, HIGH);          //dispense food
  delay(500);

  digitalWrite(mosfetPin, LOW);           //reset feeder switch
  delay(500);
}

void check_for_fish()
{
  //Use these for trooubleshooting:
  //Serial.print(analogRead(A0)); Serial.print("\t");
  //Serial.println("");
  //Serial.print(analogRead(A2)); Serial.print("\t");
  //Serial.println(fish_through_gate);

  //Note that the A0(feeding) sensor inteprets close objects as LOW numbers, while the A2(gate) sensor interpets close objects as HIGH numbers.
  //This is why the analogRead(A0) "if loop" uses "less than", while the analogRead(A2) "if loop" uses "greater than".

  if ( (analogRead(A2) < 100) && (fish_through_gate == false) ) {
    gate_sensor_time = (millis() / 1000.00) - start_time_secs;
    Serial.print("fish went into feeding area at "); Serial.println(gate_sensor_time, 4);
    fish_through_gate = true;
    fish_in_feeding_area = true;
    delay(500);
    
  }
 
  if (fish_got_food == false) {                 //check to make sure the "fish approached..." statement hasn't already been printed this cycle
    delay(4);                                   //I have no idea why this delay is necessary, but the program won't run right without it
    if (fish_through_gate == true) {
      if (analogRead(A0) < 12) {                //do this bit if the sensor is being tripped (the 75 is essentially arbitrary):
        feed_sensor_time = (millis() / 1000.00) - start_time_secs;  //feeder_sensor_time is set to the current uptime(as seconds) minus the uptime at the start of the cycle(as seconds)
        Serial.print("fish was at the feeder at "); Serial.println(feed_sensor_time, 4);
        dispense_food();
        RGB_off;
        fish_got_food = true;                                         //set this so we can tell the computer not to print the "fish approached..." statement again
      }
    }
  }    
}


void check_for_fish_leaving()
{
  if (fish_in_feeding_area == true) {
    if (analogRead(A2) < 100) {
      if (fish_through_gate == false) {
        gate_sensor_time = (millis() / 1000.00) - start_time_secs;
        Serial.print("fish went into feeding area at "); Serial.println(gate_sensor_time, 4);
        fish_through_gate = true;
        fish_in_feeding_area = true;
      }
      else {
        gate_sensor_time = (millis() / 1000.00) - start_time_secs;
        Serial.print("fish left feeding area at "); Serial.println(gate_sensor_time, 4);
        fish_in_feeding_area = false;
      }
    }
  }
}


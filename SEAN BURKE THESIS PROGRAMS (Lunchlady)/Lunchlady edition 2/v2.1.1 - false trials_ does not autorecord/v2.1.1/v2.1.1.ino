

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
boolean fish_been_through_gate = false;
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
int cutoff = 15;
int delay_time = 0;

int false_trial_determiner = 0;
boolean flashing_is_ok = true;


void setup() {
//begin SETUP section
  
  Serial.begin(9600);           //allow printing to the serial monitor
  randomSeed(analogRead(0));
  
  pinMode(LED_R, OUTPUT);       //set up pins connected to RGB leads for the feeding sensor
  pinMode(LED_G, OUTPUT);
  pinMode(LED_B, OUTPUT);
  pinMode(A2, INPUT);           //wide beam sensor
  RGB_off();

  analogReference(INTERNAL);    //set up pins to read sensors
  pinMode(A0, INPUT);           //feeder sensor

  pinMode(mosfetPin, OUTPUT);   //set up pin connected to MOSFET switch controlling the feeder
  digitalWrite(mosfetPin, LOW); //initialize feeder to OFF  

  Serial.println("All unitless numbers were measured in seconds.");
}


void loop()                                 
{   
    //SETTING UP VARIABLES
    fish_got_food = false;                    //we'll need this later
    flashing_is_ok = false;
    delay_time = 0;
    false_trial_determiner = random(1,5);   //This determines how often a false trial will occur. random(1,3) means it has a 1/3 chance; random(1,5), a 1/5 chance; random(1,10), a 1/10 chance; etc.
    
    delay(1000);                             //This waits 1 seconds so that a fish who is leaving the feeding area after waiting in there longer than the trial length won't immediately trigger the gate in the new trial
    start_time_secs = millis() / 1000.00;   //This records the uptime of the machine when the current loop was started. it needs to be saved as seconds to avoid overflow (i think? I'm taking precautions)
    randNumber = random(300,500);           //random(minimum seconds, maximum seconds)
    //randNumber = 150;

    //USER INTERFACE
    Serial.print("The following data were gathered in a trial with a length of "); Serial.println(randNumber);
    if (false_trial_determiner == 1) {
      Serial.println("This trial was a FALSE TRIAL, meaning no food was dispensed.");
    }


    RGB_on(LED_off, LED_on, LED_off);               //green light on

    //WAIT UNTIL THE TRIAL ENDS; CONTINUALLY CHECK FOR FISH BEFORE THE CUTOFF
    while (  millis()/1000 - start_time_secs <= randNumber ) {
        
          //CONTINUALLY CHECK FOR FISH BEFORE CUTOFF so long as the trial is not a "false trial"
          //check the feeder so long as the fish hasn't gotten food yet; otherwise, flash the white light (unless the white light has already been flashed)
          while ( ((millis()/1000)-start_time_secs) < cutoff) {
            if (false_trial_determiner != 1) {
              
              if (fish_got_food == false) {
                if (feeder_check() == true) {
                  dispense_food();
                }
              }
              else {
                if (flashing_is_ok == true) {
                  flash_white_light();
                  flashing_is_ok = false;                           
                }
              }
              
            }

            else {
              if (feeder_check() == true) {
                RGB_off();
              }
            }

          }
        //TURN THE GREEN LIGHT OFF ONCE THE CUTOFF HAS BEEN SURPASSED
        RGB_off();
      }


    //NOTE IF THE FISH DID NOT GET FOOD
    if (fish_got_food == false) {
      Serial.print("Fish did not approach feeding sensor");
    }

}



void RGB_on(int redBrightness, int greenBrightness, int blueBrightness) 
{
  analogWrite(LED_R, redBrightness);     
  analogWrite(LED_G, greenBrightness);
  analogWrite(LED_B, blueBrightness);
  delay(50);
}



void RGB_off()
{
  analogWrite(LED_R, LED_off);         
  analogWrite(LED_B, LED_off);           
  analogWrite(LED_G, LED_off);    
  delay(50);
}



void dispense_food() 
{
  digitalWrite(mosfetPin, HIGH);          //dispense food
  delay(500);

  digitalWrite(mosfetPin, LOW);           //reset feeder switch
  delay(500);
}

boolean feeder_check()
{
  //Use these for trooubleshooting:
  // Serial.print(analogRead(A0)); Serial.print("\t");
  //Serial.print(analogRead(A2)); Serial.print("\t");
  //Serial.println("");
  //Serial.print(fish_been_through_gate);

  //Note that the A0(feeding) sensor inteprets close objects as LOW numbers, while the A2(gate) sensor interpets close objects as HIGH numbers.
  //This is why the analogRead(A0) "if loop" uses "less than", while the analogRead(A2) "if loop" uses "greater than".

  if (fish_got_food == false) {
    delay(4);                                   //I have no idea why this delay is necessary, but the program won't run right without itp
    if (analogRead(A0) < 12) {                //do this bit if the feeder is being tripped (the number is essentially arbitrary):
      feed_sensor_time = (millis() / 1000.00) - start_time_secs;  //feeder_sensor_time is set to the current uptime(as seconds) minus the uptime at the start of the cycle(as seconds)
      Serial.print("fish was at the feeder at "); Serial.println(feed_sensor_time, 4);
      fish_got_food = true;                                         //set this so we can tell the computer not to print the "fish approached..." statement again
      flashing_is_ok = true;
      return true;
    }
    else {
      return false;
    }
  }    
}

void flash_white_light()
{
  for (int i=0; i <= 15; i++){         
  RGB_on(LED_on, LED_on, LED_on);     
  delay(75);                         
  RGB_off();                          
  delay(75);
  }
}


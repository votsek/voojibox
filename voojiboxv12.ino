/*

Sailboat Race Sound Signal
    
    Copyright (C) 2013  Author: Ed Wojtaszek

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
    
    Revision History
    Version 1.0  May 01, 2013  Initial release
    Version 1.1  Jun 25, 2013  Add beep cue, Add mode 6, Delete Serial 7-Segment support
    Version 1.2  Feb 18, 2014  Change the behavior of the Mode and Start buttons - user must hold
                               the button for at least one second. This approach solves a problem
                               with the interrupt being triggered by a momentary voltage drop
                               on the 5v line.


When power is applied to the Arduino, the software will wait for the
StartSwitch interrupt to begin signalling. The power switch is an on/off toggle and the
start switch is a normally open pushbutton. Before the start switch is pressed, the user
has access to another normally open pushbutton to select the available operating
modes. This pushbutton is disabled once the signalling has begun.

The mode pushbutton triggers an interrupt that allows the user to step through the list
of modes that are available. Each pushbutton interrupt increments circularly through the
list of modes and displays them using the yellow and red LEDs. The yellow LED indicated that
the signals will be made for rolling starts. The red LED blinks to indicate signal sequence
to be used. The red LED blinks to indicate the mode approximately every second until the
start switch is depressed.

      Red
Mode  Blinks  Yellow
 0     1      OFF
 1     1      ON
 2     2      OFF
 3     2      ON
 4     3      OFF
 5     3      ON
 6     4      ON

After the start switch is pressed and for all modes, there are five internal beeps, five short claxon
signals, followed by five more internal beeps prior to the first start. These occur at one second
intervals. This is an aid to competitors, announcing the start signal will be made.

For Mode 0, the starting sequence is three minutes long. The sequence is the following
signals (Appendix S, Sound-Signal Starting System, The Racing Rules of Sailing for
2013 - 2016, U.S. Sailing).
  
  3 minutes  -  Three long claxon tones
  2 minutes  -  Two long tones
  1 minute 30 seconds - one long and three short tones
  1 minute   -  One long tone
  30 seconds -  Three short tones
  20 seconds -  Two short tones
  10 seconds -  One short
  5 seconds  -  One short
  4 seconds  -  One short
  3 seconds  -  One short
  2 seconds  -  One short
  1 second   -  One short
  0 seconds  -  One long start signal
  
For Mode 1, the sequence repeats with the start of the three minute interval for fleets subsequent
to the first being the long start signal at t-0.

Modes 2 and 3 use the Racing Rule of Sailing Rule 26 signalling. Sound signals are made at (t-5), (t-4),
(t-1), and at the start (t-0). For Mode 3, the sequence repeats with the start of the five minute
interval (t-5) being the start signal of the previous fleet at (t-0).

Modes 4 and 5 use the Racing Rules of Sailing Rule 29.2. The first sound signal is made at (t-6),
consisting of two long sounds. At (t-5), signalling reverts to Rule 26. For Mode 5, the signaling
is the same as for Mode 3 from (t-5) forward.

Mode 7 is a special rolling mode that I call the 3 x 1 Appendix S signalling. Appendix S signalling is used
to start the first fleet. Subsequent starts are signalled using the final minute sequence of Appendix S.
  
Rolling starts will continue until the device is turned off. After the start signal on non-rolling starts,
the StartSwitch interrupt will be enabled for the next sequence of signals. The mode may not be changed
by the user until the device is cycled off and back on.

During the signalling, the red LED is illuminated during the final minute of the sequence as an aid 
for the race committee. Also, for modes 2, 3, 4, and 5 there are three internal beeps during the final
fifteen seconds prior to each signal event. This is a race committee aid as well to alert them when
flags supplement the audible signals.

Note on addition or modification of signal sequences. There is a convention that I have used
to assure accuracy and account for processing during the signal sequence. The starting clock
value is read before any processing during the signal interval. The ending clock value is read
after the processing is completed. The difference is subtracted from the interval time to account for
that processing in the wait time. This should be done during any signal interval where there
is processing other than the wait itself.

*/

//
//  Constants
//

const byte maxMode = 6;     // Number for the highest mode implemented
//
// The following are used in wait-time calculations and include a bias to account for
// Arduino Pro clock error. Used once the signal sequence has started.
// Bias = -.024%
// Note: The bias is for a specific Uno board. The bias should be set to zero
// prior to upload of the software to a board for calibration. Calibration is recomended
// for both oscillator clock and resonator clock boards.
//
const unsigned long int oneMinute = 60000-14;    // one minute in ms
const unsigned long int threeMinutes = 180000-42;// Three minutes in ms
const unsigned long int thirtySeconds = 30000-7; // thirty seconds in ms
const unsigned long int tenSeconds = 10000-2;    // ten seconds in ms
const unsigned long int fiveSeconds = 5000-1;    // five seconds in ms

//
// The following are similar to the constants above. They are intended to allow for an
// audible, high frequency signal to the race committee for flag signal events. These flag event
// signals are made in the 15 second interval before the race signal sounds.
//
const unsigned long int fortyfiveSeconds = 45000-11;// Forty five seconds in ms
const unsigned long int fifteenSeconds = 15000-3;   // Fifteen seconds in ms
const unsigned long int twofortyFive = 165000-40;    // Two minutes forty five seconds in ms
//
// The following time constants are used for delays. They should not be used in wait-time
// calculations during signalling since they are not biased.
//
const unsigned long int fiveMinutes = 300000; // Five minutes in ms
const unsigned long int oneSecond = 1000;     // delay in ms for on and off phases
const unsigned long int redonDuration = 500;     // Red LED on duration in ms
const unsigned long int redoffDuration = 500;    // Red LED off duration
const unsigned long int beeponDuration = 500;    // Event beep duratin in ms
const unsigned long int beepoffDuration = 500;   // Event beep off duration in ms
const unsigned long int claxonlongDuration = 1500;  // Long signal duration in ms
const unsigned long int claxonshortDuration = 500;  // Short signal duration in ms
const unsigned long int claxonstartDuration = 2000; // Start signal duration in ms
const unsigned long int halfSecond = 500;  // Half second in ms
const unsigned long int buttonHold=1000;            // Time Mode or Start buttons must be held by the user
                                                    // to trigger the event
//
//  Input / Output Constants
//

const int relayPin = 4;	    // use this digital pin to drive the claxon relay, transistor base, or MOSFET gate
const int intZero = 0;      // StartSwitch interrupt, Uno digital pin 2
const int startintPin = 2;  // Start interrupt pin
const int intOne = 1  ;     // Mode Switch interrupt, Uno digital pin 3
const int modeintPin = 3;   // Mode interrupt pin corresponding with intOne above.
const int powerPin = 8;     // green power indicator LED pin
const int rollingPin = 10;  // yellow rolling start LED indicator pin
const int modePin = 12;     // red mode LED pin
const int beepPin = 11;     // pin used for piezo buzzer beep
const int fiveBeeps = 5;    // number of beeps = 5
const int threeBeeps = 3;   // number of beeps = 3
const unsigned int beepFreq = 2700;  // beep frequency is 2700 Hz

//
//  Variables
//

byte modeNumber;           // Signalling mode selected.
int startRoll=0;           // Flag to indicate an iteration of the rolling start
                           // Must be = 0 on the first call of a signal sequence
int modeChange = 0;        // Flag to indicate mode switch interrupt
int skipSignal = 1;        // The signalling is skipped until set to 0 by interrupt
int fiveTimes = 0;         // Counter for five second signals
int idleLoop = 1;          // StartSwitch function variable: idle while = 1
int loopForever = 1;       // Variable tested in rolling start that allows loop to run until power off
int selectCase = 0;        // Integer variableloaded with the mode value to switch to the signalling case
unsigned long int startClock = 0; // The start time of a signal activity
unsigned long int endClock = 0;   // The ending time of a signal activity
unsigned long int silenceDuration = 0;// Silent portion of one second signal
unsigned long int waitTime;       // Time to wait between signals

#include <EEPROM.h>        // EEPROM library used to store state info to persist on power off

void setup()
{
  
  modeNumber = EEPROM.read ( 0 );  // Read stored value of the option number in byte 0
  
  //
  // If the mode number has not been previously accessed, it will = 255.
  // If any number other than 0 through maxMode, it must be set to the starting value of 0
  //
  
  if ( modeNumber > maxMode )
  {
    modeNumber = 0; // Set it to the starting value of 0
  }

  pinMode(relayPin, OUTPUT);     // set claxon relay pin as an output
  pinMode(powerPin, OUTPUT);     // set the power light pin as output
  pinMode(rollingPin, OUTPUT);   // set the rolling start light pin as output
  pinMode(modePin, OUTPUT);      // set the mode light pin as output
  pinMode(modeintPin, INPUT);    // set the mode interrupt pin as an input
  pinMode(startintPin, INPUT);   // set the start interrupt pin as an input
  
  digitalWrite (powerPin, HIGH); // turn on the green power LED
  
  ShowMode ();                   // Use the yellow and red LEDs to indicate the initial mode
  
  attachInterrupt (intZero, StartSwitch, LOW); // set to interrupt on Start Switch closure
  attachInterrupt (intOne, ModeSwitch, LOW );  // Set to interrupt on each Mode Switch closure

}

void loop ()
{
  
  //
  ////////////////////////////////// Wait for interrupts ////////////////////////////
  //
  
  //
  // If mode change interrupt, update mode number, display, and EEPROM
  //
  
  if ( modeChange > 0 )
  {
    //
    // Mode change logic.
    //
    
    noInterrupts ();              // Disable interrupts
    
    delay (buttonHold);           // The user must hold the Mode Switch for at least
                                  // this duration.
                                  
    if (digitalRead (modeintPin) == LOW)
    {
      //
      // Do this only if the Mode Switch has been held by the user for at least the
      // duration given by modebuttonHold. That is, the button must still be held closed
      // after the delay.
      //
      
      modeNumber = modeNumber + 1;  // Increment the mode
    
      //
      // If the current mode exceeds the max
      //
    
      if (modeNumber > maxMode )
      {
        modeNumber = 0;  // Reset to the starting default
      }
      
      ShowMode ();       // Use the yellow and red LEDs to indicate the new mode
      EEPROM.write ( 0, modeNumber );  // Store mode in case power off
      
      //
      // Don't allow any further action until the Mode Switch is released by the user
      //
      while ( digitalRead (modeintPin) == LOW )
      {
        delay (100);
      }
      
    }
    
    modeChange = 0;    // Ready for next interrupt
    interrupts ();     // Enable interrupts
    
   
   
  }
  
  ShowMode ();        // Use the yellow and red LEDs to indicate the mode each tiime through the loop
  
  //
  // If start switch interrupt, start signalling
  //

  if (skipSignal < 1)
  {
    noInterrupts ();             // Temporarily hold interrupts until start switch depression is validated
    delay (buttonHold);          // The user must hold the Start switch at least this duration.
    
    if (digitalRead (startintPin) == LOW )
    {
      //
      // Wait until the user releases the Start button
      //
      while (digitalRead (startintPin) == LOW)
      {
        delay (100);
      }
      //
      // Do this only if the Mode Switch has been held by the user for at least the
      // duration given by modebuttonHold. That is, the button must still be held closed
      // after the delay.
      //
      
      interrupts ();               // Enable interrupts.
      detachInterrupt ( intZero ); // No interrupts - would distort the signal timing
      detachInterrupt ( intOne );
    
      //
      /////////// Let competitors know that the start sequence is about to begin //////////
      //
      // Five short beeps to mark five seconds
      //
    
      BeepEvent (fiveBeeps);
    
      //
      // Five short one second claxon signals to announce the starting sequence
      //

      silenceDuration = oneSecond - claxonshortDuration;// Constant for all loop iterations
    
      fiveTimes = 5;                  // Loop count

      while (fiveTimes>0)                 
      {
      
        SoundClaxon ( claxonshortDuration, silenceDuration);
      
        fiveTimes--;                   // Decrement the loop count
      }

      //
      // Five more beeps to mark five seconds
      //
    
      BeepEvent (fiveBeeps);
    
      //
      // This code section executes the case corresponding with the user mode selection.
      // Modes 0, 2, and 4 will return to the main loop to wait for user input as for a
      // power up. Modes 1, 3, and 5 will loop doing rolling starts until power is turned off.
      //
    
      selectCase = int (modeNumber) + 1;  // Convert mode to int
    
      switch ( selectCase )
      {
        case 1:
        //
        // Appendix S start
        //
      
        startRoll = 0;  // No rolling start
      
        AppendixS ();   // Signal the start
      
        break;
      
        case 2:
        //
        // Appendix S rolling start
        //
      
        startRoll = 0;  // Full sequence for first iteration
      
        //
        // Loop until the power is turned off
        //
      
        while ( loopForever > 0 )
        {
        
          AppendixS ();  // Signal the start
        
          startRoll = 1; // Next start skips the first signal to iterate rolling start
        
        }
      
        break;
      
        case 3:
        //
        // Rule 26 start
        //
      
        startRoll = 0;  // No rolling start
      
        RuleTwoSix ();  // Signal the start
      
        break;
      
        case 4:
        //
        // Rule 26 rolling start
        //
      
        startRoll = 0;  // Full sequence for the first set of signals
      
        //
        // Loop until the power is turned off
        //
      
        while ( loopForever > 0 )
        {
        
          RuleTwoSix ();  // Signal the start
        
          startRoll = 1;  // Next start skips the first signal to iterate rolling start
        
        }
      
        break;
      
        case 5:
        //
        // Rule 29.2 start
        //
      
        startRoll = 0;    // No rolling start
      
        RuleTwoNineTwo ();// Signal the start
      
        break;
      
        case 6:
        //
        // Rule 29.2 rolling start
        //
      
        startRoll = 0;  // Full sequence for the first set of signals
        
        //
        // Loop until the power is turned off
        //
      
        while ( loopForever > 0 )
        {
          
          RuleTwoNineTwo ();  // Signal the start
          
          startRoll = 1;      // Next start skips the recall start signal and the first signal to iterate rolling start
          
        }
          
        break;
        
        case 7:
        //
        // Appendix S with 1 minute roll for pursuit races
        //
        startRoll = 0;  // No rolling start
        
        AppendixS ();   // Start the race
        
        digitalWrite (modePin, HIGH);                    // Turn on the red LED and leave it on
        
        while ( loopForever > 0 )
        {
          
          AppendixSMinute ();  //; Run the one minute sequence
          
        }
        
        break;
      
      }
    }
    else
    {
      interrupts ();                              // Turn Interrupts back on.
      delay (100);
      attachInterrupt (intOne, ModeSwitch, LOW);  // Mode switch still OK
    }
    //
    ////////////////// Reinitialize to wait for StartSwitch closure ////////////////////////
    //
      
    
    skipSignal = 1;  // Skip signalling once again until the StartSwitch is closed
    attachInterrupt (intZero, StartSwitch, LOW); // Restart interrupt for the start switch
    
    // Note: mode switch interrupt is not allowed again if a start sequence was signalled
    // until power is cycled off and back on
    
  }
//
// End if - set up for next loop
//

delay ( oneSecond );
}


void StartSwitch()
{
  //
  // Interrupt service routine that senses the start switch and ends the idle loop
  //
  
  skipSignal = 0;  // Turn off the idle and start signalling

}

void ModeSwitch ()
{
  //
  // Interrupt service routine that senses the Mode Switch while in the idle loop.
  // Each time the Mode Switch contact is closed, interrupt one calls
  // this function. The modeChange flag is set to 1 to enable display of the
  // mode number.
  //
  
  modeChange = 1;  // Set the mode change flag to trigger update and display of new mode
  
}

void ShowMode ()
{
  //
  // Indicates the mode using the yellow and red LEDs. The green LED is lit for
  // non-zero odd mode numbers. The red LED blinks to indicate the Rule in use.
  // One blink is for Appendix S, two blinks for Rule 26, and three blinks for Rule 29.
  // Input:
  //   modeNumber = current mode number
  // Output:
  //   the yellow and red LEDs are updated to indicate the mode to the user
  //
  // Yellow LED state: 0 = off; 1 = on
  const int yellowState[7] = {0, 1, 0, 1, 0, 1, 1};
  //
  // Red LED number of blinks
  const int redBlinks[7] = {1, 1, 2, 2, 3, 3, 4};
  //
  const unsigned long int redblinkDuration = 250;  // Red blink duration in ms
  const unsigned long int redoffDuration = 250;    // Red off duration between blinks
  
  int arrayIndex = 0;  // Integer used as mode equivalent for array indexing
  int blinkCount = 0;  // Count the blinks
  
  arrayIndex = 0;                   // Initialize
  arrayIndex = int ( modeNumber );  // convert mode to integer
  
  digitalWrite (rollingPin, LOW);   // Turn off the yellow LED so that it at least blinks
                                    // once when going from one rolling mode to another.
                                    // This behavior gives the user a visual indication
                                    // of a change in mode when going from one rolling
                                    // start mode to another.
  delay (100);                      // Wait long enough for the blink to be evident.
  
  if ( yellowState [arrayIndex] > 0 )
  {
    // Turn on yellow LED
    digitalWrite (rollingPin, HIGH);
    
  }
  else
  {
    // Turn off the yellow LED
    digitalWrite (rollingPin, LOW);
    
  }
  
  blinkCount = redBlinks [ arrayIndex ];
  
  while (blinkCount > 0)
  {
    digitalWrite (modePin, HIGH);  // Red on
    delay ( redblinkDuration );    // Wait
    digitalWrite (modePin, LOW);   // Red off
    delay ( redoffDuration );      // Wait
    
    blinkCount--;
    
  }
  
  delay (oneSecond);               // Wait so that blinks don't happen more often than this
  
}

void BeepEvent (int beepCount)
{
  //
  // Beeps a piezo to mard a coming flag signal event
  // Input:
  //   beepCount = number of times to beep
  // Globals:
  //   beeponDuration = duration for red LED on
  //   beepoffDuration = duration for red LED off
  // Output:
  //   Piezzo buzzer turned on and off for the specified input durations
  //
  int beepLoop = beepCount;
  
  while(beepLoop > 0)
  {
    tone (beepPin, beepFreq);
    delay (beeponDuration);
    noTone (beepPin);
    delay (beepoffDuration);
    beepLoop--;
  }
  
}

void SoundClaxon ( unsigned long int toneDuration, unsigned long int notoneDuration )
{
  //
  // Function to sound the start signal claxon for a given duration, followed by silence for a given duration.
  //
  // Value returned: none
  // Input:
  //   toneDuration = unsigned long integer tone duration in ms
  //   notoneDuration = unsigned long integer silence duration in ms
  // Output:
  //   Audible signal is sounded
  //
  // Globals:
  //   relayPin
  //
  
  
  digitalWrite ( relayPin, HIGH );  // Start the tone
  
  delay ( toneDuration );           // Run the tone as long as needed
  
  digitalWrite ( relayPin, LOW );   // Stop the tone
  
  delay ( notoneDuration );         // Run out the silent period
}

void AppendixS ()
{
  //
  // Function to sound the starting signal according to Racing Rules of Sailing Appendix S.
  // This function will skip the 3-minute signal for rolling starts when the startRoll flag
  // is greater than zero. This means that the start signal for the first fleet is the
  // three minute signal for the second and later fleets. The diaplay is also updated to
  // to cue the race committee on the time of the next signal.
  //
  // Input:
  //   startRoll = 0 - sound 3-minute signal, > 0 - skip 3-minute signal
  //
  
  
  silenceDuration = halfSecond;  // After-tone delay will always be this
  
  //
  // If the rolling start is iterating after the initial fleet start, skip the
  // three tones.
  //
  
  if ( startRoll == 0 )
  {
  
    startClock = millis ();        // Mark the time the first tone starts
    
    SoundClaxon ( claxonlongDuration, silenceDuration );
    SoundClaxon ( claxonlongDuration, silenceDuration );
    SoundClaxon ( claxonlongDuration, silenceDuration );
    
  }
  
  endClock = millis ();                           // Time the signal ended
  waitTime = oneMinute - (endClock - startClock); // Of the minute, time remaining to next signal
  
  delay ( waitTime );                             // Run the minute out
  
  //
  // Two long claxon tones to signal 00:02:00 to the start
  //
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonlongDuration, silenceDuration );
  SoundClaxon ( claxonlongDuration, silenceDuration );
  
  endClock = millis ();                                // Time the signal ended
  waitTime = thirtySeconds - (endClock - startClock);  // Of the minute, time remaining to next signal
  
  delay ( waitTime );                                  // Run out the thirty seconds
  
  //
  // One long and three short with 00:01:30 to go
  //
  
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonlongDuration, silenceDuration );
  
  SoundClaxon ( claxonshortDuration, silenceDuration );
  SoundClaxon ( claxonshortDuration, silenceDuration );
  SoundClaxon ( claxonshortDuration, silenceDuration );
  
  endClock = millis ();                                // Time the signal ended
  waitTime = thirtySeconds - (endClock - startClock);  // Time ramaining to next signal
  
  delay ( waitTime );                                  // Run out the thirty seconds
  
  //
  // One long with 00:01:00 to go
  //
  
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonlongDuration, silenceDuration );
  
  digitalWrite (modePin, HIGH);                    // Turn on the red LED for the final minute
  
  AppendixSMinute ();
  
  digitalWrite (modePin, LOW);     // Turn off the red LED
  
}

void AppendixSMinute ()
{
  //
  // This function runs the final minute of the Appendix S starting sequence. It is used in
  // the 3 x 1 variant implemented for pursuit races. For this signalling, the final minute is repeated
  // until the device is turned off.
  //
  
  endClock = millis ();
  
  waitTime = thirtySeconds - (endClock - startClock);  // Time remaining to next signal
  
  delay ( waitTime );                                  // Run out the thirty seconds
  
  //
  // Three short with 00:00:30 to go
  //
  
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonshortDuration, silenceDuration );
  SoundClaxon ( claxonshortDuration, silenceDuration );
  SoundClaxon ( claxonshortDuration, silenceDuration );
  
  endClock = millis ();                                // Time the signal ended
  waitTime = tenSeconds - (endClock - startClock);     // Time ramaining to next signal
  
  delay ( waitTime );                                  // Run out the ten seconds
  
  //
  // Two short with 00:00:20 to go
  //
  
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonshortDuration, silenceDuration );
  SoundClaxon ( claxonshortDuration, silenceDuration );
  
  endClock = millis ();                                // Time the signal ended
  waitTime = tenSeconds - (endClock - startClock);     // Time ramaining to next signal
  
  delay ( waitTime );                                  // Run out the ten seconds
  
  //
  // One short with 00:00:10 to go
  //
  
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonshortDuration, silenceDuration );
  
  endClock = millis ();                                // Time the signal ended
  waitTime = fiveSeconds - (endClock - startClock);    // Time remaining to next signal
  
  delay ( waitTime );                                  // Run out the five seconds
  
  //
  // Final five short tones at one second intervals
  //
  
  fiveTimes = 5;                  // Loop count
  while (fiveTimes>0)                 
  {
    
    SoundClaxon ( halfSecond, halfSecond);
    
    fiveTimes--;                   // Decrement the loop count
  }
  
  //
  /////////////////////////////////// Start Signal //////////////////////////////////////
  //
  
  //
  // Note: The clock error is not included in the final five second interval because it is
  // in the noise over the entire signal sequence.
  //
  startClock = millis ();          // need start clock time in case of rolling start
  
  SoundClaxon ( claxonstartDuration, silenceDuration);
}

void RuleTwoSix ()
{
  //
  // Function to sound the starting signal according to Racing Rules of Sailing Rule 26.
  // This function will skip the 5-minute signal for rolling starts when the startRoll flag
  // is greater than zero. This means that the start signal for the first fleet is the
  // five minute signal for the second and later fleets. The display is also updated to
  // to cue the race committee on the time of the next signal.
  //
  // Input:
  //   startRoll = 0 - sound 5-minute signal, > 0 - skip 5-minute signal
  //
  
  silenceDuration = halfSecond;  // After-tone delay will always be this
  
  //
  // If the rolling start is iterating after the initial fleet start, skip the
  // five tones.
  //
  
  if ( startRoll == 0 )
  {
  
    startClock = millis ();        // Mark the time the first tone starts
    
    SoundClaxon ( claxonlongDuration, silenceDuration );
    
  }
  
  endClock = millis ();                           // Time the signal ended
  waitTime = fortyfiveSeconds - (endClock - startClock); // Time remaining to event beep
  
  delay ( waitTime );                             // Run the time out
  
  //
  // Beep to let the RC know that an event is coming
  //
  
  startClock = millis ();
  BeepEvent (threeBeeps);
  endClock = millis ();
  waitTime = fifteenSeconds - (endClock - startClock); // Time remaining to signal
  
  delay ( waitTime );
  
  
  
  //
  // Four long claxon tones to signal 00:04:00 to the start
  //
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonlongDuration, silenceDuration );
  
  endClock = millis ();                                // Time the signal ended
  waitTime = twofortyFive - (endClock - startClock);  // Of the 2:45, time remaining to the event beep
  
  delay ( waitTime );                                  // Run out the time
  
  //
  // Beep to let the RC know that an event is coming
  //
  
  startClock = millis ();
  BeepEvent (threeBeeps);
  endClock = millis ();
  waitTime = fifteenSeconds - (endClock - startClock); // Time remaining to signal
  
  delay ( waitTime );
  
  
  //
  // One long claxon tone to signal 00:01:00 to the start
  //
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonlongDuration, silenceDuration );
  
  digitalWrite (modePin, HIGH);                    // Turn the red LED on during the final minute
  
  endClock = millis ();                                    // Time the signal ended
  waitTime = fortyfiveSeconds - (endClock - startClock);   // Time remaining to next event beep
  
  delay ( waitTime );                                  // Run out the time
  
  //
  // Beep to let the RC know that an event is coming
  //
  
  startClock = millis ();
  BeepEvent (threeBeeps);
  endClock = millis ();
  waitTime = fifteenSeconds - (endClock - startClock); // Time remaining to signal
  
  delay ( waitTime );
  
  //
  // One long claxon tone to signal the start
  //
  startClock = millis ();                          // Mark the time the first tone starts
  
  SoundClaxon ( claxonlongDuration, silenceDuration );
  
  digitalWrite (modePin, LOW);                    // Turn the red LED off
  
}

void RuleTwoNineTwo ()
{
  //
  // Function to sound the starting signal according to Racing Rules of Sailing Rule 29.2.
  // This function will skip the 5-minute signal for rolling starts when the startRoll flag
  // is greater than zero. This means that the start signal for the first fleet is the
  // five minute signal for the second and later fleets. The display is also updated to
  // to cue the race committee on the time of the next signal. This rule starts the first
  // signal at six minutes before beginning the Rule 26 signalling.
  //
  // Input:
  //   startRoll = 0 - sound 5-minute signal, > 0 - skip 5-minute signal
  //
  
  silenceDuration = halfSecond;  // After-tone delay will always be this
  
  //
  // If the rolling start is iterating after the initial fleet start, skip the
  // signal at six minutes and the signal of five tones at five minutes.
  //
  
  if ( startRoll == 0 )
  {
    startClock = millis ();        // Mark the time the first tone starts
    
    //
    // Sound the signal starting at 00:06:00
    //
    
    SoundClaxon ( claxonlongDuration, silenceDuration );
    SoundClaxon ( claxonlongDuration, silenceDuration );
    
    endClock = millis ();                           // Time the signal ended
    
    waitTime = fortyfiveSeconds - (endClock - startClock); // Time remaining to next event beep
  
    delay ( waitTime );                             // Run the minute out
    
    //
    // Beep to let the RC know that an event is coming
    //
    
    startClock = millis ();
    BeepEvent (threeBeeps);
    endClock = millis ();
    waitTime = fifteenSeconds - (endClock - startClock); // Time remaining to signal
    
    delay ( waitTime );
      
  }
  
  RuleTwoSix ();    // Run Rule 26

}

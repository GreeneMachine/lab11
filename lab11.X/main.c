//*****************************************************************
// Name:    Dr. Chris Coulston
// Date:    Spring 2020
// Purp:    inLab11 - Hall effect sensor and MIDI
//
// Assisted: The entire EENG 383 class
// Assisted by: Technical documents
//
// Academic Integrity Statement: I certify that, while others may have
// assisted me in brain storming, debugging and validating this program,
// the program itself is my own work. I understand that submitting code
// which is the work of other individuals is a violation of the course
// Academic Integrity Policy and may result in a zero credit for the
// assignment, course failure and a report to the Academic Dishonesty
// Board. I also understand that if I knowingly give my original work to
// another individual that it could also result in a zero credit for the
// assignment, course failure and a report to the Academic Dishonesty
// Board.
//*****************************************************************
#include "mcc_generated_files/mcc.h"
#pragma warning disable 520     
#pragma warning disable 1498

//NOTE FUNCTIONS
#define PLAYNOTE        0x90 
#define NOTEOFF         0x80
#define PLAYINSTRUMENT  0xC0

#define PLAY_DELAY      50000
#define N               64

void    putByteSCI(uint8_t writeByte);
void    noteOn(uint8_t cmd, uint8_t pitch, uint8_t velocity);

//*****************************************************************
//*****************************************************************
void main(void) {
     
    uint8_t i;
    uint8_t pitch = 64;
    uint8_t instrument=0;
    char    cmd;
    uint8_t sample[N];
    uint8_t nominalHallUnPressed = 63;
    uint8_t nominalHallPressed = 29;
    uint8_t delta = 5;
    uint16_t sampleRate = 1000;
    
    
    SYSTEM_Initialize();
    ADC_SelectChannel(HALL_SENSOR);
    
    printf("Development Board\r\n");
    printf("inLab 11 terminal \r\n");
    printf("MIDI + Hall effect \r\n");   
    printf("\r\n> ");                       // print a nice command prompt

    ADC_StartConversion();
    while(!ADC_IsConversionDone());
    nominalHallUnPressed = (ADC_GetConversionResult() >> 8);
                
	for(;;) {
		if (EUSART1_DataReady) {			// wait for incoming data on USART
            cmd = EUSART1_Read();
			switch (cmd) {		// and do what it tells you to do

			//--------------------------------------------
			// Reply with help menu
			//--------------------------------------------
			case '?':
				printf("-------------------------------------------------\r\n");   
                printf("    Nominal %u to %u\r\n",nominalHallUnPressed,  nominalHallPressed);
                printf("    delta = %d\r\n",delta);
                printf("    sampleRate = %d TMR0 counts = %duS\r\n",sampleRate,sampleRate);
				printf("-------------------------------------------------\r\n");
                printf("?: help menu\r\n");
                printf("o: k\r\n");
                printf("Z: Reset processor.\r\n");
                printf("z: Clear the terminal.\r\n");
                printf("d/D: decrement/increment delta\r\n");
                printf("s/S: decrement/increment sampleRate\r\n");
                printf("c/C: calibrate unpressed/pressed hall sensor.\r\n");   
                printf("1: report a single Hall effect sensor reading.\r\n");   
                printf("t: wait for piano keypress and report %d samples, one every %dus.\r\n",N, sampleRate);
                printf("M: enter into Midi mode.\r\n");
                printf("-------------------------------------------------\r\n");
				break;

            //--------------------------------------------
            // Reply with "k", used for PC to PIC test
            //--------------------------------------------
            case 'o':
                printf("o:      ok\r\n");
                break;
                
            //--------------------------------------------
            // Reset the processor after clearing the terminal
            //--------------------------------------------
            case 'Z':
                for (i=0; i<40; i++) printf("\n");
                RESET();
                break;

            //--------------------------------------------
            // Clear the terminal
            //--------------------------------------------
            case 'z':
                for (i=0; i<40; i++) printf("\n");
                break;

            //--------------------------------------------
			// calibrate nominalHallUnPressed
			//--------------------------------------------
            case 'c':
                printf("Hands off the piano, press keyboard key to calibrate nominalHallUnPressed");
                while (!EUSART1_DataReady);
                (void) EUSART1_Read();                
                ADC_StartConversion();
                while(!ADC_IsConversionDone());
                nominalHallUnPressed = ADC_GetConversionResult()>>8;
                printf("New nominalHallUnPressed value = %d\r\n",nominalHallUnPressed);
                break;  
                
            //--------------------------------------------
			// calibrate nominalHallPressed
			//--------------------------------------------
            case 'C':
                printf("Press the piano key, press keyboard key to calibrate nominalHallPressed");
                while (!EUSART1_DataReady);
                (void) EUSART1_Read();                
                ADC_StartConversion();
                while(!ADC_IsConversionDone());
                nominalHallPressed = ADC_GetConversionResult()>>8;
                printf("New nominalHallPressed value = %d\r\n",nominalHallPressed);
                break;                 
                
			//--------------------------------------------
            // Tune noise immunity
			//--------------------------------------------                
            case 'd':
            case 'D':
                if (cmd == 'd') delta -= 1;
                else delta += 1;
                printf("new delta = %d\r\n",delta);
                break;

            //--------------------------------------------
            // Tune sampling rate
			//-------------------------------------------- 
            case 's':
            case 'S':
                if (cmd == 's') sampleRate -= 100;
                else sampleRate += 100;
                printf("new sampleRate = %d TMR0 counts = %duS\r\n",sampleRate, sampleRate);
                break;

                
            //--------------------------------------------
			// Convert hall effect sensor once
			//--------------------------------------------
            case '1':
                TEST_PIN_SetHigh();
                ADC_StartConversion();
                TEST_PIN_SetLow();
                while(!ADC_IsConversionDone());
                printf("Hall sensor = %d\r\n",ADC_GetConversionResult()>>8);
                break;               

            //--------------------------------------------
			// Wait for a keypress and then record samples
			//--------------------------------------------
            case't': 
                printf("Tap a piano key.\r\n");
                
                // Wait for sensor to deviate from the nominal unpressed value
                // This would indicate the begining of a keypress event
                while ( (ADC_GetConversionResult()>>8) > nominalHallUnPressed - delta){
                    TMR0_WriteTimer(TMR0_ReadTimer() + 0xFFFF - sampleRate);
                    INTCONbits.TMR0IF = false;
                    ADC_StartConversion();
                    while (TMR0_HasOverflowOccured() == false);                    
                }
                    
                // Record N samples at sampleRate TMR0 counts
                for (i=0; i<N; i++) {
                    sample[i] = (ADC_GetConversionResult()>>8);
                    TMR0_WriteTimer(TMR0_ReadTimer() + 0xFFFF - sampleRate);  
                    INTCONbits.TMR0IF = false;
                    TEST_PIN_SetHigh();
                    ADC_StartConversion();
                    TEST_PIN_SetLow();
                    while (TMR0_HasOverflowOccured() == false);
                } // end for i
                
                // print the N samples to the terminal 
                for (i=0; i<N; i++) {                    
                    printf("%4d ",sample[i]);
                } // end for i
                printf("\r\n");

                // You are responsible for this section of code.  Compute the
                // key press "velocity".  A surrogate for this value will be
                // the first index of the sample array which is less than the
                // calibrated pressed value plus delta.  If no value of the 
                // sample array is less than this value, then return the length 
                // of the sample array (N) plus the amount the last sample value 
                // is above the calibrated pressed value plus delta.
                i=0;
			// put some code here.  I needed 2 lines of code,

                printf("key velocity = %d\r\n",i);
                break;                               
                
            //--------------------------------------------
			// Midi mode
			//--------------------------------------------
            case 'M':
                printf("Launch hairless-midiserial\r\n");
                printf("In the hairless-midiserial program:\r\n");
                printf("    File -> preferences\r\n");
                printf("    Baud Rate:  9600\r\n");
                printf("    Data Bits:  8\r\n");
                printf("    Parity:     None\r\n");
                printf("    Stop Bit(s):    1\r\n");
                printf("    Flow Control:   None\r\n");
                printf("    Click ""OK""\r\n");
                printf("    Serial Port -> (Not Connected)\r\n");
                printf("    MIDI Out -> (Not Connected)\r\n\n");
                printf("When you have complete this:\r\n");
                printf("    Close out of Putty.\r\n");                
                printf("    In the hairless-midiserial program:\r\n");
                printf("        Serial Port -> USB Serial Port (COMx)\r\n");
                printf("        MIDI Out -> Microsoft GS Wavetable Synth \r\n");                
                printf("    Press upper push button on Dev'18 to start MIDI play sequence\r\n");                
                printf("    Press lower push button on Dev'18 to exit MIDI play sequence\r\n");
                printf("    In the hairless-midiserial program:\r\n");
                printf("        Serial Port -> (Not Connected)\r\n");
                printf("        MIDI Out -> (Not Connected)\r\n");
                printf("        close hairless\r\n");
                printf("    Launch PuTTY and reconnect to the VCOM port\r\n");
                
                pitch=64;
                instrument = 0;
                while(TOP_BUTTON_GetValue() == 1);
                while(BOTTOM_BUTTON_GetValue() != 0) {

                    noteOn(PLAYNOTE, pitch, 105);
                    TMR0_WriteTimer(TMR0_ReadTimer() + 0xFFFF - PLAY_DELAY);
                    INTCONbits.TMR0IF = 0;
                    while (TMR0_HasOverflowOccured() == false);
                    noteOn(NOTEOFF, pitch, 105);

                    pitch = pitch + 1;
                    if (pitch > 76) {    
                        pitch = 64;
                        instrument = (instrument + 8) & 0x7F;
                        putByteSCI(PLAYINSTRUMENT);
                        putByteSCI(instrument);
                    } // end playing up a scale                
                } // end until lower button press
                break;                               
                
			//--------------------------------------------
			// If something unknown is hit, tell user
			//--------------------------------------------
			default:
				printf("Unknown key %c\r\n",cmd);
				break;
			} // end switch
            
		}	// end if
    } // end infinite loop    
} // end main



//*****************************************************************
// Sends a byte to MIDI attached to serial port
//*****************************************************************
void putByteSCI(uint8_t writeByte) {  
        while (PIR1bits.TX1IF == 0);
        PIR1bits.TX1IF = 0;
        TX1REG = writeByte; // send character
} // end putStringSCI 


//Sends the MIDI command over the UART
void noteOn(uint8_t cmd, uint8_t pitch, uint8_t velocity) {
  putByteSCI(cmd);              //First byte "STATUS" byte
  putByteSCI(pitch);            //Second byte note to be played
  putByteSCI(velocity);         //Third byte the 'loudness' of the note (0 turns note off)
}


/**
 End of File
*/
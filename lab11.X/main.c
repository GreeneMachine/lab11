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
#define SONGLENGTH      12

void    putByteSCI(uint8_t writeByte);
void    noteOn(uint8_t cmd, uint8_t pitch, uint8_t velocity);

typedef enum  {UNPRESSED, PRESSED_ACQUIRE, PRESSED_WAIT} state;
state isrState = UNPRESSED;

uint8_t hallSamples[N];

uint8_t songNote[SONGLENGTH] = {0x3C, 0x3C, 0x3E, 0x3C, 0x41, 0x40, 0x3C, 0x3C, 0x3E, 0x3C, 0x43, 0x41};

//*****************************************************************
//*****************************************************************
void main(void) {
     
    uint8_t i;
    uint8_t pitch = 64;
    uint8_t instrument=0;
    char    cmd;
    uint8_t nominalHallUnPressed = 63;
    uint8_t nominalHallPressed = 29;
    uint8_t delta = 5;
    uint16_t sampleRate = 1000;
    uint8_t keyVelocity;
    
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
                printf("c/C: calibrate unpressed/pressed hall sensor.\r\n");   
                printf("1: report a single Hall effect sensor reading.\r\n");   
                printf("t: determine strike time with 64 samples, once every 1000 us.\r\n");
                printf("T: strike indicator and time\r\n");
                printf("i: ISR values\r\n");
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
            case 't': 
                printf("Tap a piano key.\r\n");

                while (isrState != PRESSED_WAIT);
                
                // print the N samples to the terminal 
                for (i=0; i<N; i++) {                    
                    printf("%4d ",hallSamples[i]);
                } // end for i
                printf("\r\n");
                
                keyVelocity = N + hallSamples[N-1];
                for (i=0; i<N; i++) {
                    if (nominalHallPressed + delta > hallSamples[i]) {
                        keyVelocity = 128 - i;
                        break;
                    }    
                }
                
                printf("key velocity = %d\r\n",keyVelocity);
                break; 
                
            
            case 'T': 
                
                printf("Tap piano key and press keyboard key to exit.\r\n");
                
                while (!EUSART1_DataReady) {
                    
                    if (isrState == PRESSED_WAIT){
                        
                        keyVelocity = N + hallSamples[N-1];

                        for (i=0; i<N; i++) {
                            if (nominalHallPressed + delta > hallSamples[i]) {
                                keyVelocity = 128 - i;
                                break;
                            }    
                        }

                        printf("key velocity = %d     ",keyVelocity);
                        
                        while (isrState == PRESSED_WAIT);
                        
                        printf("released.\r\n");
                    
                    }                 
                    
                }
                
                (void) EUSART1_Read();

                break;
                
            case 'i':
                
                for (i=0; i<N; i++) {                    
                    printf("%4d ",hallSamples[i]);
                } // end for i
                printf("\r\n");
                
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
                              
                for (i = 0; i < SONGLENGTH; i++) {
                    
                    putByteSCI(PLAYINSTRUMENT);
                    putByteSCI(0);
                    
                    while (isrState != PRESSED_WAIT);
                    
                    keyVelocity = N + hallSamples[N-1];

                    for (uint8_t j = 0; j < N; j++) {
                        if (nominalHallPressed + delta > hallSamples[j]) {
                            keyVelocity = 128 - j;
                            break;
                        }    
                    }
                    
                    noteOn(PLAYNOTE, songNotes[i], keyVelocity);
                    
                    while (isrState != UNPRESSED);
                    
                    noteOn(NOTEOFF, songNotes[i], keyVelocity);
                    
                }
                
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


void myTMR0ISR(void){
    
    static uint8_t numCollected = 0;

    switch (isrState){
        
        case UNPRESSED:
            
           if ( (ADC_GetConversionResult()>>8) < nominalHallUnPressed - delta) {
               isrState = PRESSED_ACQUIRE;
               numCollected = 0; 
           }
           
           ADC_StartConversion();
           break;
        
        case PRESSED_ACQUIRE:
            
            hallSamples[numCollected] = (ADC_GetConversionResult()>>8);
            
            numCollected++;
            if (numCollected == N) {
                isrState = PRESSED_WAIT;
            }
            
            ADC_StartConversion();
            break;
        
        case PRESSED_WAIT:
            
            if ( (ADC_GetConversionResult()>>8) > nominalHallUnPressed - delta) {
                isrState = UNPRESSED;
            }
            
            ADC_StartConversion();
            break;          

    }
    
    TMR0_WriteTimer(TMR0_ReadTimer() + (0x10000 - sampleCount));
    
    INTCONbits.TMR0IF = 0;
    
}

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

#include <xc.h>

// Standard SG90 Pulse Widths (Calibrated for ~90 degree swing)
#define PULSE_LOCK    1000  // 0 degrees
#define PULSE_UNLOCK  1500  // 90 degrees

// --------- DEBOUNCE FUNCTION ---------
void debounce(void)
{
    while(RB0==0 || RB1==0 || RB2==0 || RB3==0);
    for(unsigned int d=0; d<10000; d++);
    while(RB0==0 || RB1==0 || RB2==0 || RB3==0);
}

// --------- SERVO SIGNAL GENERATOR ---------
// Generates a single PWM pulse and handles the 20ms frame delay
void servo_sg90(unsigned int pulse_us)
{
    unsigned int preload;

    /* ---------- HIGH PULSE (Signal) ---------- */
    RC2 = 1;
    T1CON = 0x01; // Timer1: 1:1 Prescaler (0.2us per tick @ 20MHz)
    TMR1IF = 0;

    // Adjusted preloads to ensure a full 90-degree range
    if(pulse_us == 1000)      preload = 62536;  // Short pulse for 0 deg
    else if(pulse_us == 1500) preload = 55036;  // Long pulse for 90 deg
    else                      preload = 58036;  // Default center

    TMR1 = preload;
    while(!TMR1IF);
    TMR1IF = 0;

    /* ---------- LOW PERIOD (20ms Gap) ---------- */
    RC2 = 0;
    T1CON = 0x11; // Timer1: 1:4 Prescaler (0.8us per tick)
    TMR1 = 40536; // Constant 20ms frame
    while(!TMR1IF);
    TMR1IF = 0;
}

void main()
{
    char password[4] = {'2','3','5','6'};
    char entered[4];
    unsigned char idx = 0;
    unsigned char match;
    unsigned int i;

    // Port Configurations
    TRISB = 0X0F;    // RB0-RB3 Inputs (Keypad), RB4-RB7 Outputs
    nRBPU = 0;       // Enable internal pull-ups for B
    TRISC2 = 0; // RC2 Output (Servo Signal)
    TRISC3 = 0; // LED Signal
    TRISC4 = 0;
    TRISC6 = 0;      // RC6 Output (UART TX)
    RC2 = 0;
    RC3 = 0;
    RC4 = 0;
    // UART Initialization (9600 Baud @ 20MHz)
    SPBRG = 0x81;
    TXSTA = 0x24;    
    RCSTA = 0x80;    
    
    while(1)
    {
        // --- Keypad Scanning Logic ---
        RB7=0; RB6=1; RB5=1; RB4=1;
        if(RB3==0){ entered[idx++]='1'; while(!TRMT); TXREG='1'; debounce(); }
        if(RB2==0){ entered[idx++]='2'; while(!TRMT); TXREG='2'; debounce(); }
        if(RB1==0){ entered[idx++]='3'; while(!TRMT); TXREG='3'; debounce(); }

        RB7=1; RB6=0; RB5=1; RB4=1;
        if(RB3==0){ entered[idx++]='4'; while(!TRMT); TXREG='4'; debounce(); }
        if(RB2==0){ entered[idx++]='5'; while(!TRMT); TXREG='5'; debounce(); }
        if(RB1==0){ entered[idx++]='6'; while(!TRMT); TXREG='6'; debounce(); }

        RB7=1; RB6=1; RB5=0; RB4=1;
        if(RB3==0){ entered[idx++]='7'; while(!TRMT); TXREG='7'; debounce(); }
        if(RB2==0){ entered[idx++]='8'; while(!TRMT); TXREG='8'; debounce(); }
        if(RB1==0){ entered[idx++]='9'; while(!TRMT); TXREG='9'; debounce(); }

        RB7=1; RB6=1; RB5=1; RB4=0;
        if(RB0==0){ entered[idx++]='0'; while(!TRMT); TXREG='0'; debounce(); }

        /* -------- PASSWORD VERIFICATION -------- */
        if(idx >= 4)
        {
            match = 1;
            for(int j=0; j<4; j++)
            {
                if(entered[j] != password[j]) { match = 0; break; }
            }

            if(match)
            {
                // Send "OK" to Serial
                while(!TRMT); TXREG='-'; // New line
                while(!TRMT); TXREG='C'; 
                while(!TRMT); TXREG='O'; 
                while(!TRMT); TXREG='R';
                while(!TRMT); TXREG='R'; 
                while(!TRMT); TXREG='E'; 
                while(!TRMT); TXREG='C';
                while(!TRMT); TXREG='T'; 
                //while(!TRMT); TXREG='E'; 
                //while(!TRMT); TXREG='N'; 
                while(!TRMT); TXREG=' ';
                RC3=1;

                // STEP 1: MOVE TO 90 DEGREES (UNLOCK)
                // Hold for ~2 seconds (100 pulses * 20ms = 2000ms)
                for(i=0; i<500; i++)
                {
                    servo_sg90(PULSE_UNLOCK); 
                }
                

                // STEP 2: MOVE BACK TO 0 DEGREES (LOCK)
                // Send 75 pulses to ensure it fully returns
                for(i=0; i<50; i++)
                {
                    servo_sg90(PULSE_LOCK);
                }
                
                RC3=0;
            }
            else
            {
                // Send "NO" to Serial
                while(!TRMT); TXREG='-'; // New line
                while(!TRMT); TXREG='I'; 
                while(!TRMT); TXREG='N'; 
                while(!TRMT); TXREG='C';
                while(!TRMT); TXREG='O'; 
                while(!TRMT); TXREG='R'; 
                while(!TRMT); TXREG='R';
                while(!TRMT); TXREG='E'; 
                while(!TRMT); TXREG='C'; 
                while(!TRMT); TXREG='T'; 
                //while(!TRMT); TXREG='';
                //while(!TRMT); TXREG='D'; 
                while(!TRMT); TXREG=' ';
                
                
                // Force servo to stay/return to Lock position
            }

            idx = 0; // Clear index for next password attempt
        }
    }
}
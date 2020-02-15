#include <msp430.h>
void Display_Number(long long n);
void LCDInit ();
void LCD_all_off(void);
int flag=0;

void main()
{
  WDTCTL = WDTPW | WDTHOLD;	// Stop watchdog timer
  PM5CTL0 &= ~LOCKLPM5;		// Clear locked IO Pins 		
  P3SEL0 |= BIT5 |BIT4 ;                 	// USCI_A0 UART operation
  P3SEL1 &= ~(BIT4 | BIT5);
  P3DIR |= BIT4;
  
  UCA1CTLW0 = UCSWRST;                 	// Put eUSCI in reset
  UCA1CTLW0 |= UCSSEL__SMCLK;    	// CLK = SMCLK
  UCA1BRW = 6;                              		// 115,200 baud
  UCA1MCTLW = 0x22D1;		// UCBRSx value = 0x08 (See UG)
  UCA1CTL1 &= ~UCSWRST;                	// Initialize eUSCI
  
  UCA1IE |= UCRXIE;                     		// Enable USCI_A0 RX interrupt
  while(!(UCA1IFG&UCTXIFG)); 		// wait for transmitter ready
  UCA1TXBUF = 0x00;			// Transmit!

  TA0CTL = TASSEL__ACLK + MC__UP+ TACLR+ ID__1+ TAIE;	
  TA0CCR0 = 32768;

  LCDInit ();
  __bis_SR_register(LPM3_bits | GIE);       	// Enter LPM3, interrupts enabled
  __no_operation();                         		// For debugger
}
void LCD_all_off(void)
{
	int i;
	char *ptr = 0;
	ptr += 0x0A20;		// LCD memory starts at 0x0A20
	for (i = 0;i<21;i++)
		*ptr++ = 0x00;
}
void LCDInit ()
{
    PJSEL0 = BIT4 | BIT5;                   // For LFXT

    // Initialize LCD segments 0 - 21; 26 - 43
    LCDCPCTL0 = 0xFFFF;
    LCDCPCTL1 = 0xFC3F;
    LCDCPCTL2 = 0x0FFF;

    // Disable the GPIO power-on default high-impedance mode
    // to activate previously configured port settings
    PM5CTL0 &= ~LOCKLPM5;

    // Configure LFXT 32kHz crystal
    CSCTL0_H = CSKEY >> 8;                  // Unlock CS registers
    CSCTL4 &= ~LFXTOFF;                     // Enable LFXT
    do
    {
      CSCTL5 &= ~LFXTOFFG;                  // Clear LFXT fault flag
      SFRIFG1 &= ~OFIFG;
    }while (SFRIFG1 & OFIFG);               // Test oscillator fault flag
    CSCTL0_H = 0;                           // Lock CS registers

    // Initialize LCD_C
    // ACLK, Divider = 1, Pre-divider = 16; 4-pin MUX
    LCDCCTL0 = LCDDIV__1 | LCDPRE__16 | LCD4MUX | LCDLP;

    // VLCD generated internally,
    // V2-V4 generated internally, v5 to ground
    // Set VLCD voltage to 2.60v
    // Enable charge pump and select internal reference for it
    LCDCVCTL = VLCD_1 | VLCDREF_0 | LCDCPEN;

    LCDCCPCTL = LCDCPCLKSYNC;               // Clock synchronization enabled

    LCDCMEMCTL = LCDCLRM;                   // Clear LCD memory
      LCDCCTL0 |= LCDON;
 }
void Display_Number(long long n)
{
  int i=0;
  const unsigned char lcd_num[10] = { 0xFC, 0x60, 0xDB, 0xF3, 0x67, 0xB7, 0xBF, 0xE4,  0xFF, 0xF7};
    char *Ptr2Num[6] = {0};
    Ptr2Num[0] +=0xA29;
    Ptr2Num[1] +=0xA25;
    Ptr2Num[2] +=0xA23;
    Ptr2Num[3] +=0xA32;
    Ptr2Num[4] +=0xA2E;
    Ptr2Num[5] +=0xA27;

  LCD_all_off();
     do{
        *Ptr2Num[5-i] = lcd_num[n%10];
         i++;
         n = n/10;  // wastefull!!
    }while ( n );
}
#pragma vector=USCI_A1_VECTOR
__interrupt void USCI_A1_ISR() {
  char TAV;
  if  (UCA1IFG & UCRXIFG)  {
       	UCA1IFG &= ~ UCRXIFG;  
	while(!(UCA1IFG&UCTXIFG));	// wait for transmitter ready
        TAV=UCA1RXBUF;
        UCA1TXBUF = TAV;
        Display_Number(TAV);
        flag=0;
	__no_operation();
  }	
}
#pragma vector=TIMER0_A1_VECTOR
__interrupt void timer()
{
  int TAV=UCA1RXBUF;
  if((TA0IV == 0x0E)&&(flag=0))
  {
  UCA1TXBUF = TAV;
  flag=1;
  }
}


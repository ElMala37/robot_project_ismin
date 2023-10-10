#include<p18f2520.h>
#include <stdio.h>
#include"MI2C.h"
#pragma config OSC = INTIO67
#pragma config PBADEN = OFF, WDT = OFF, LVP = OFF, DEBUG = ON

#define ADRESSE_TELECOMMANDE 0xA2  // adresse I2C du p�riph�rique � lire
#define ADRESSE_LEDI2C 0x40  // adresse I2C des leds
#define BOUTON_BAS 0x34
#define BOUTON_HAUT 0x32
#define NUM_SAMPLES 4

unsigned volatile int TBAT[NUM_SAMPLES];
unsigned int nBAT = 0;
unsigned int MBAT = 0; // valeur moyenne tension batterie
signed int vitesse = 0; // variable vitesse
char buffer[3];  // tableau pour stocker les donn�es lues
volatile char flag_telecommande =0; // flag telecommande
volatile char flag_batterie =0; // flag batterie
unsigned int count_TMR2 = 0; // compteur TMR2
int nb_led=0;


void HighISR(void);
void avancer(void);
void reculer (void);
void init (void);
void led (void);
void surveillance_batterie (void);

#pragma code HighVector=0x08
void IntHighVector(void)
{
    _asm goto HighISR _endasm
}
#pragma code

#pragma interrupt HighISR
void HighISR(void){
    if(INTCONbits.INT0IF) // est provoqu� par IT t�l�commande
	{
            INTCONbits.INT0IF = 0;
            flag_telecommande = 1;
	}
    if (PIR1bits.TMR2IF)
    {
        PIR1bits.TMR2IF = 0; // Effacer le drapeau d'interruption INT2
        count_TMR2++; 
        if (count_TMR2==200) //
        {
            count_TMR2=0;
            flag_batterie=1;
        }
    }
}

void avancer(void){
    if(vitesse>-1 && vitesse <5){
        if (vitesse==0){
            PORTAbits.RA6 = 0; // met les moteurs dans la direction 1
            PORTAbits.RA7 = 0; // met les moteurs dans la direction 1
        }
        vitesse+=1;
        if (vitesse==1){
            CCPR1L += 45 ; // moteur droit
            CCPR2L += 45 ; // moteur gauche
        }
        nb_led++;
        CCPR1L += 16 ; // moteur droit
	CCPR2L += 16 ; // moteur gauche
        printf("Vitesse : %d\r\n",vitesse);
    }
    else{
        if(vitesse<0){ // arr�t des moteurs
            vitesse=0;
            nb_led=0;
            CCPR1L = 0 ; // moteur droit
            CCPR2L = 0 ; // moteur gauche
            printf("Vitesse : %d (� l'arr�t)\r\n",vitesse);
        }
        else{
            printf("Vitesse maximale\r\n");
        }
    }
}

void reculer(void){
   if(vitesse<1 && vitesse >-5){
        if (vitesse==0){
            PORTAbits.RA6 = 1; // met les moteurs dans la direction 0
            PORTAbits.RA7 = 1; // met les moteurs dans la direction 0
        }
        vitesse-=1;
        if (vitesse==-1){
            CCPR1L += 45 ; // moteur droit
            CCPR2L += 45 ; // moteur gauche
        }
        CCPR1L += 16 ; // moteur droit
	CCPR2L += 16 ; // moteur gauche
        nb_led++;
        printf("Vitesse : %d\r\n",vitesse);
    }
    else{
        if(vitesse>0){ //arr�t des moteurs
            vitesse=0;
            nb_led=0;
            CCPR1L = 0 ; // moteur droit
            CCPR2L = 0 ; // moteur gauche
            printf("Vitesse : %d (� l'arr�t)\r\n",vitesse);
        }
        else{
            printf("Vitesse maximale\r\n");
        }
    }
}

void led()
{
    switch (nb_led) {

    case 0:
        Write_PCF8574(ADRESSE_LEDI2C, 0b11111111);
        break;

    case 1:
        Write_PCF8574(ADRESSE_LEDI2C, 0b11111110);
        break;

    case 2:
        Write_PCF8574(ADRESSE_LEDI2C, 0b11111100);
        break;

    case 3:
        Write_PCF8574(ADRESSE_LEDI2C, 0b11111000);
        break;

    case 4:
        Write_PCF8574(ADRESSE_LEDI2C, 0b11110000);
        break;

    default:
        Write_PCF8574(ADRESSE_LEDI2C, 0b11100000);
        break;
    }
}


void surveillance_batterie()
{
    int i = 0;
    ADCON0bits.GO = 1; // d�marrage ADC
    while (ADCON0bits.GO_DONE) { // temps de la conversion
    }
    TBAT[nBAT] = ADRESH;
    if (nBAT == (NUM_SAMPLES - 1)) {
        nBAT = 0;}
    else {
        nBAT+=1;}
    MBAT = 0;
    for (i; i < NUM_SAMPLES; i++) {
        MBAT += TBAT[i];
    }
    MBAT = MBAT / NUM_SAMPLES;
    if (MBAT < 160) {  //((2**n)-1)*Ana/Vref   255*3,125/5=159,37
        printf("Batterie faible : %d mV\r\n",MBAT*19,53);  //5000/256
        PORTBbits.RB5 = 1;//Met la led de test � 1
    } else {
        printf("Batterie en forme : %d mV\r\n",MBAT*19,53);
        PORTBbits.RB5 = 0;//Met la led de test � 0
    }
}

void init(void){
    //Initialisation valeurs batterie
    int i=0;
    for (i; i < NUM_SAMPLES; i++) {
        TBAT[i] = 255;
    }

    // initialisation de la fr�quence d'horloge � 8 MHz
    OSCCONbits.IRCF0=1;
    OSCCONbits.IRCF1=1;
    OSCCONbits.IRCF2=1;

    //Configuration ADC
    // Configuration ADCON1: on utilise les ports AN0, AN1, AN2
    ADCON1bits.VCFG1 = 0;   // Configuration de la r�f�rence de tension VREF- (VSS)
    ADCON1bits.VCFG0 = 0;   // Configuration de la r�f�rence de tension VREF+ (VDD)
    ADCON1bits.PCFG3 = 1;   // Configuration de l'entr�e AN0 en mode analogique
    ADCON1bits.PCFG2 = 1;   // Configuration de l'entr�e AN1 en mode analogique
    ADCON1bits.PCFG1 = 0;   // Configuration de l'entr�e AN2 en mode analogique
    ADCON1bits.PCFG0 = 0;   // Configuration des autres entr�es en mode num�rique

    // Configuration ADCON2
    ADCON2bits.ADCS2 = 1;   // Configuration du diviseur de l'horloge ADC pour Fosc/32, frequence de fonctionnement de l'ADC
    ADCON2bits.ADCS1 = 0;
    ADCON2bits.ADCS0 = 1;
    ADCON2bits.ADFM = 0;    // R�sultat de l'ADC align� � gauche
    ADCON2bits.ACQT2 = 0;   // Temps d'acquisition TACQ = 6 us
    ADCON2bits.ACQT1 = 1;
    ADCON2bits.ACQT0 = 1;

    // Configuration ADCON0
    ADCON0bits.CHS0 = 0;    // S�lection de l'entr�e AN2 comme canal d'entr�e de l'ADC
    ADCON0bits.CHS1 = 1;
    ADCON0bits.CHS2 = 0;
    ADCON0bits.CHS3 = 0;
    ADCON0bits.ADON = 1;    // Activation de l'ADC

    //initialisation du Timer2
    T2CONbits.T2OUTPS3 = 1;
    T2CONbits.T2OUTPS2 = 1;
    T2CONbits.T2OUTPS1 = 1;
    T2CONbits.T2OUTPS0 = 0;   //postcaler � 16
    //prescaler de 4
    T2CONbits.T2CKPS0 = 1;
    T2CONbits.T2CKPS1 = 0;
    //PR2 = 249
    PR2 = 249;
    // rapport cyclique = 0%
    CCPR1L = 0 ; // moteur droit
    CCPR2L = 0 ; // moteur gauche
    // rapport cyclique DCxB1:DCxB0
    CCP1CONbits.DC1B0 = 0;
    CCP1CONbits.DC1B1 = 0;
    CCP2CONbits.DC2B0 = 0;
    CCP2CONbits.DC2B1 = 0;
    // choix mode PWM
    CCP1CONbits.CCP1M3 = 1; //moteur droit
    CCP1CONbits.CCP1M2 = 1;
    CCP2CONbits.CCP2M3 = 1; // moteur gauche
    CCP2CONbits.CCP2M2 = 1;
    T2CONbits.TMR2ON = 1; // mise en marche du Timer2


    // Initialisation de l'interruption INT0
    INTCON2bits.INTEDG0 = 0;   // D�finit le front descendant de RB0 comme �v�nement d�clencheur de l'interruption INT0
    INTCONbits.INT0IE = 1;     // Active l'interruption externe INT0
    INTCONbits.PEIE = 1;       // Active les interruptions p�riph�riques
    INTCONbits.GIE = 1;        // Active les interruptions globales
    INTCON2bits.INTEDG2 = 1;   // D�finit le front montant de RB2 comme �v�nement d�clencheur de l'interruption INT2
    PIR1bits.TMR2IF = 0;       // Efface le drapeau d'interruption du Timer 2
    PIE1bits.TMR2IE = 1;       // Active l'interruption du Timer 2
    PIE1bits.ADIE = 0;         // D�sactive l'interruption de l'ADC


    //Configuration I2C
    MI2CInit();


   // Configuration UART
    TXSTAbits.SYNC = 0;     // Mode asynchrone
    // Configuration du registre BAUDCON (Baud Rate Control)
    BAUDCONbits.BRG16 = 0;  // Ne pas utiliser le registre SPBRGH pour la communication UART � 16 bits
    TXSTAbits.BRGH = 1;     // S�lection du mode de haute vitesse pour la communication UART
    // Configuration du registre SPBRG (Baud Rate Generator)
    /*
    SPBRG = (Fr�quence d'horloge / (d�bit de bauds * 4)) - 1
    SPBRG = (8000000 / (4800 * 16)) - 1 = 103
    */
    // Configuration du d�bit de bauds (4800 bps pour une fr�quence d'horloge de 8 MHz)
    SPBRGH = 0; //partie haute
    SPBRG = 103; //partie basse
    TXSTAbits.TXEN = 1;     // Activation de la transmission UART
    // Configuration du registre RCSTA (Reception Status and Control)
    RCSTAbits.SPEN = 1;     // Activation du port s�rie
    TRISCbits.RC6=0; // TX en sortie
    TRISCbits.RC7=1; // RX en entr�e - reception



    //Configuration des entr�es/sorties
    TRISCbits.RC1=0; // mise en sortie du moteur gauche
    TRISCbits.RC2=0; // mise en sortie du moteur droit
    TRISBbits.RB0=1; // IT t�l�commande en entr�e
    TRISBbits.RB5 = 0;//Met la led de test en sortie
    PORTBbits.RB5 = 0;//Met la led de test � 0
    //initialisation de DIR D et DIR G
    TRISAbits.RA6=0; // DIR D
    TRISAbits.RA7=0; // DIR G
    // donne une valeur aux sortie DIR D et DIR G
    PORTAbits.RA6 = 0; // met la broche RA6 � l'�tat haut Moteur Droit
    PORTAbits.RA7 = 0; // met la broche RA7 � l'�tat haut Moteur Gauche
           
}


void main(void) {
        init();
        printf("Initialiation termin�e \r\n");
	while(1)
        {
          if (flag_telecommande) //flag t�l�commande
          {
              Lire_i2c_Telecom(ADRESSE_TELECOMMANDE, buffer);
              if (buffer[1]== BOUTON_BAS ){
                reculer();
                led();
              }
              else if (buffer[1]== BOUTON_HAUT ){
                avancer();
                led();
              }
              flag_telecommande=0;
          }
          if (flag_batterie) //flag batterie
          {
              flag_batterie=0;
              surveillance_batterie();              
          }

    }
}
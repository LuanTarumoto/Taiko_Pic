#include <xc.h>
#include "nxlcd.h"

#pragma config PLLDIV = 5
#pragma config CPUDIV = OSC1_PLL2 
#pragma config FOSC = HS
#pragma config WDT = OFF
#pragma config PBADEN = OFF
#pragma config LVP = OFF
#pragma config DEBUG = OFF
#define _XTAL_FREQ 20000000

#define TOTAL_TICKS 62 

char don_cheio[8] = {0x00, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x00, 0x00};
char ka_vazio[8]  = {0x00, 0x0E, 0x11, 0x11, 0x11, 0x0E, 0x00, 0x00};

unsigned char bgm_eva[48] = {
    148, 132, 124, 110, 110, 124, 132, 0,    
    124, 110, 0,   0,   98,  110, 124, 110,  
    124, 132, 0,   148, 148, 0,   0,   0,    
    98,  110, 124, 132, 148, 0,   148, 0,    
    132, 124, 110, 124, 132, 0,   132, 0,    
    148, 132, 124, 110, 98,  82,  73,  0     
};

char hit_map[48] = {
    1, 2, 1, 2, 1, 2, 1, 0, 1, 2, 0, 0,
    1, 1, 2, 2, 1, 2, 0, 1, 1, 0, 0, 0,
    2, 2, 2, 2, 1, 0, 1, 0, 2, 1, 2, 1,
    2, 0, 2, 0, 1, 2, 1, 2, 1, 1, 1, 0
};

unsigned char bgm_real[TOTAL_TICKS];
char beatmap_real[TOTAL_TICKS];

int index_musica = 0;
char pista[16] = {0}; 
char flag_tick = 0;
char flag_placar = 0;
char mensagem_hit = 0; 
unsigned int pontuacao = 0;
unsigned int high_score = 0;
unsigned char carga_timer_h = 0; 

void escrever_EEPROM(unsigned char endereco, unsigned char dado) {
    EEADR = endereco; 
    EEDATA = dado;
    EECON1bits.EEPGD = 0; 
    EECON1bits.CFGS = 0; 
    EECON1bits.WREN = 1;
    char status_gie = INTCONbits.GIE; 
    INTCONbits.GIE = 0;               
    EECON2 = 0x55; 
    EECON2 = 0xAA; 
    EECON1bits.WR = 1;                
    while(EECON1bits.WR);             
    INTCONbits.GIE = status_gie; 
    EECON1bits.WREN = 0;
}

unsigned char ler_EEPROM(unsigned char endereco) {
    EEADR = endereco; 
    EECON1bits.EEPGD = 0; 
    EECON1bits.CFGS = 0; 
    EECON1bits.RD = 1; 
    return EEDATA;
}

void tocar_som(char tipo) {
    unsigned char pr2_antigo = PR2;
    unsigned char ccpr1l_antigo = CCPR1L;
    char timer2_on = T2CONbits.TMR2ON;

    T2CON = 0b00000111; 
    CCP1CON = 0b00001100; 
    
    if(tipo == 1)      { PR2 = 180; CCPR1L = 90;  } 
    else if(tipo == 2) { PR2 = 80;  CCPR1L = 40;  } 
    else               { PR2 = 250; CCPR1L = 125; } 
    
    __delay_ms(50); 
    
    PR2 = pr2_antigo; 
    CCPR1L = ccpr1l_antigo; 
    T2CONbits.TMR2ON = timer2_on; 
}

unsigned int ler_ADC() {
    ADCON0bits.GO = 1; 
    while(ADCON0bits.GO); 
    return ((ADRESH << 8) + ADRESL); 
}

void __interrupt() isr(void) {
    if(INTCONbits.TMR0IF == 1) {
        INTCONbits.TMR0IF = 0; 
        TMR0H = carga_timer_h; 
        TMR0L = 0x00; 
        flag_tick = 1; 
    }
    if(INTCONbits.INT0IF == 1) {
        INTCONbits.INT0IF = 0; 
        if(pista[1] == 1) { 
            pontuacao += 10; 
            pista[1] = 0; 
            mensagem_hit = 1; 
            flag_placar = 1; 
            tocar_som(1); 
        } 
        else { 
            mensagem_hit = 2; 
            flag_placar = 1; 
            tocar_som(0); 
        }
    }
    if(INTCON3bits.INT1IF == 1) {
        INTCON3bits.INT1IF = 0; 
        if(pista[1] == 2) { 
            pontuacao += 10; 
            pista[1] = 0; 
            mensagem_hit = 1; 
            flag_placar = 1; 
            tocar_som(2); 
        } 
        else { 
            mensagem_hit = 2; 
            flag_placar = 1; 
            tocar_som(0); 
        }
    }
}

void main(void) {
    TRISB = 0xFF; 
    TRISCbits.TRISC2 = 0; 
    TRISD = 0x00; 
    TRISE = 0x00;         
    ADCON1 = 0x0E; 
    ADCON2 = 0b10001010; 
    ADCON0 = 0b00000001; 
    INTCON2bits.INTEDG0 = 0; 
    INTCON2bits.INTEDG1 = 0; 

    for(int i=0; i<TOTAL_TICKS; i++) {
        if(i < 14) bgm_real[i] = 0; 
        else bgm_real[i] = bgm_eva[i - 14];
        
        if(i < 48) beatmap_real[i] = hit_map[i]; 
        else beatmap_real[i] = 0;
    }

    OpenXLCD(FOUR_BIT & LINES_5X7); 
    WriteCmdXLCD(0x01); 
    __delay_ms(2); 
    WriteCmdXLCD(0x0C); 

    WriteCmdXLCD(0x40); 
    for(int i = 0; i < 8; i++) WriteDataXLCD(don_cheio[i]); 
    for(int i = 0; i < 8; i++) WriteDataXLCD(ka_vazio[i]); 
    WriteCmdXLCD(0x80); 

    high_score = (ler_EEPROM(1) << 8) | ler_EEPROM(0);
    if(high_score == 0xFFFF) high_score = 0; 

    while(1) {
        while(PORTBbits.RB0 == 0); __delay_ms(50); 
        
        index_musica = 0; 
        pontuacao = 0; 
        flag_tick = 0; 
        flag_placar = 0; 
        mensagem_hit = 0;
        for(int i = 0; i < 16; i++) pista[i] = 0; 
        
        WriteCmdXLCD(0x01); 
        __delay_ms(2); 
        WriteCmdXLCD(0xC0); 
        putsXLCD("Aperte O (Start)"); 
        
        while(PORTBbits.RB0 == 1) {
            unsigned int valor_adc = ler_ADC();
            carga_timer_h = 190 + (((unsigned long)valor_adc * 40) / 1023); 
            int porcentagem = ((unsigned long)valor_adc * 99) / 1023;
            
            WriteCmdXLCD(0x80); putsXLCD("Velocidade: "); 
            putcXLCD((porcentagem / 10) % 10 + '0'); 
            putcXLCD((porcentagem % 10) + '0'); 
            putsXLCD("% ");
            __delay_ms(100); 
        }
        
        tocar_som(1); 
        __delay_ms(500);

        WriteCmdXLCD(0x01); 
        __delay_ms(2); 
        WriteCmdXLCD(0x80); 
        putsXLCD("Pts:0000        "); 
        
        INTCONbits.INT0IF = 0; 
        INTCON3bits.INT1IF = 0; 
        INTCONbits.TMR0IF = 0;
        
        RCONbits.IPEN = 0; 
        INTCONbits.TMR0IE = 1; 
        INTCONbits.INT0IE = 1; 
        INTCON3bits.INT1IE = 1;
        INTCONbits.PEIE = 1; 
        INTCONbits.GIE = 1; 
        T0CON = 0b00000111; 
        T0CONbits.TMR0ON = 1; 

        while(index_musica < TOTAL_TICKS) {
            if(flag_placar == 1) {
                flag_placar = 0; WriteCmdXLCD(0x84); 
                putcXLCD((pontuacao / 1000) % 10 + '0'); 
                putcXLCD((pontuacao / 100) % 10 + '0');
                putcXLCD((pontuacao / 10) % 10 + '0'); 
                putcXLCD((pontuacao % 10) + '0');
                
                WriteCmdXLCD(0x89); 
                if(mensagem_hit == 1) putsXLCD("ACERTO!"); 
                else if(mensagem_hit == 2) putsXLCD(" ERRO!");
            }
            
            if(flag_tick == 1) {
                flag_tick = 0; 
                if(bgm_real[index_musica] != 0) {
                    T2CON = 0b00000111; 
                    CCP1CON = 0b00001100; 
                    PR2 = bgm_real[index_musica]; 
                    CCPR1L = PR2 / 2; 
                } else { CCPR1L = 0; }

                for(int i = 1; i < 15; i++) pista[i] = pista[i+1];
                pista[15] = beatmap_real[index_musica]; 
                index_musica++;
                
                WriteCmdXLCD(0xC0); 
                putcXLCD('>'); 
                for(int i = 1; i < 16; i++) {
                    if(pista[i] == 1) putcXLCD(0); 
                    else if(pista[i] == 2) putcXLCD(1); 
                    else putcXLCD(' ');
                }
            }
        } 

        T0CONbits.TMR0ON = 0; 
        INTCONbits.GIE = 0; 
        CCPR1L = 0; 
        WriteCmdXLCD(0x01); 
        __delay_ms(2);
        
        if(pontuacao > high_score) {
            high_score = pontuacao; 
            escrever_EEPROM(0, (pontuacao & 0xFF)); 
            escrever_EEPROM(1, (pontuacao >> 8));   
            WriteCmdXLCD(0x80); 
            putsXLCD("NOVO RECORDE!   "); 
        } else {
            WriteCmdXLCD(0x80); putsXLCD("Pts: "); 
            putcXLCD((pontuacao / 1000) % 10 + '0'); 
            putcXLCD((pontuacao / 100) % 10 + '0');
            putcXLCD((pontuacao / 10) % 10 + '0'); 
            putcXLCD((pontuacao % 10) + '0'); putsXLCD(" FIM!"); 
        }
        
        WriteCmdXLCD(0xC0); 
        putsXLCD("Max: "); 
        putcXLCD((high_score / 1000) % 10 + '0'); 
        putcXLCD((high_score / 100) % 10 + '0');
        putcXLCD((high_score / 10) % 10 + '0'); 
        putcXLCD((high_score % 10) + '0'); 
        putsXLCD(" ->(O)"); 
        
        while(PORTBbits.RB0 == 0); 
        __delay_ms(50); 
        while(PORTBbits.RB0 == 1); 
    }
}
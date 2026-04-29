#include <xc.h>
#include <stdio.h>

#pragma config FOSC = HS, WDTE = OFF, PWRTE = OFF, BOREN = OFF, LVP = OFF
#define _XTAL_FREQ 20000000

// LCD Control Pins
#define RS RD0
#define EN RD1

// L298 Motor Driver Connections
#define MOTOR_IN1 RB1    // IN1 connected to RB1

// LEDs
#define DRY_LED RC0
#define WET_LED RC1
#define MOTOR_LED RC2

// Input
#define RAIN RB0

// ADC for soil moisture on RA0 and LM35 on RA1
void ADC_Init(){
    ADCON0 = 0x41;
    ADCON1 = 0x80;
}

int ADC_Read(int channel){
    ADCON0 &= 0xC5;
    ADCON0 |= (channel << 3);
    __delay_ms(10);
    GO_nDONE = 1;
    while(GO_nDONE);
    return ((ADRESH << 8) + ADRESL);
}

// LCD Functions
void LCD_Pulse(){
    EN = 1;
    __delay_us(100);
    EN = 0;
    __delay_us(100);
}

void LCD_Send4Bit(unsigned char data){
    PORTD &= 0x0F;
    PORTD |= (data & 0xF0);
    LCD_Pulse();
}

void LCD_Command(unsigned char cmd){
    RS = 0;
    LCD_Send4Bit(cmd);
    LCD_Send4Bit(cmd << 4);
    __delay_ms(5);
}

void LCD_Char(unsigned char data){
    RS = 1;
    LCD_Send4Bit(data);
    LCD_Send4Bit(data << 4);
    __delay_ms(5);
}

void LCD_String(const char *str){
    while(*str) LCD_Char(*str++);
}

void LCD_SetCursor(unsigned char row, unsigned char col){
    if(row == 1) LCD_Command(0x80 + col);
    else LCD_Command(0xC0 + col);
}

void LCD_Clear(){
    LCD_Command(0x01);
    __delay_ms(5);
}

void LCD_Init(){
    TRISD = 0x00;
    PORTD = 0x00;
    __delay_ms(20);
    
    RS = 0;
    LCD_Send4Bit(0x30);
    __delay_ms(10);
    LCD_Send4Bit(0x30);
    __delay_ms(5);
    LCD_Send4Bit(0x30);
    __delay_ms(5);
    LCD_Send4Bit(0x20);
    __delay_ms(5);
    
    LCD_Command(0x28);
    LCD_Command(0x0C);
    LCD_Command(0x06);
    LCD_Command(0x01);
    __delay_ms(10);
}

int GetTemperature(int adc_value){
    return (adc_value * 500) / 1023;
}

void main(){
    int soil_moisture, temp_adc, temperature;
    char line1[17], line2[17];
    
    // Configure ports
    TRISA = 0xFF;        // RA0, RA1 as analog inputs
    TRISB = 0x01;        // RB0 input (rain), RB1-RB7 outputs
    TRISC = 0x00;        
    TRISD = 0x00;        // LCD outputs
    
    // Initialize all outputs to 0
    PORTB = 0x00;
    PORTC = 0x00;
    PORTD = 0x00;
    
    // Initialize peripherals
    ADC_Init();
    LCD_Init();
    
    // Display startup message
    LCD_SetCursor(1,0);
    LCD_String("System Ready");
    LCD_SetCursor(2,0);
    LCD_String("Monitoring...");
    __delay_ms(2000);
    LCD_Clear();
    
    while(1){
        // Read sensors
        soil_moisture = ADC_Read(0);
        temp_adc = ADC_Read(1);
        temperature = GetTemperature(temp_adc);
        
        // -------- CONTROL LOGIC --------
        if(RAIN == 1){  
            // Rain detected - Wet LED ON, everything else OFF
            MOTOR_IN1 = 0;      // Motor OFF
            MOTOR_LED = 0;      // Motor LED OFF
            DRY_LED = 0;        // Dry LED OFF
            WET_LED = 1;        // Wet LED ON
        }
        else if(temperature > 30){  
            // Temperature above 30°C - Motor ON, Dry LED ON, Motor LED ON
            MOTOR_IN1 = 1;      // Motor ON
            MOTOR_LED = 1;      // Motor LED ON (motor working)
            DRY_LED = 1;        // Dry LED ON
            WET_LED = 0;        // Wet LED OFF
        }
        else if(soil_moisture < 250){  
            // Soil too dry - Motor ON, Dry LED ON
            MOTOR_IN1 = 1;      // Motor ON
            MOTOR_LED = 1;      // Motor LED ON (motor working)
            DRY_LED = 1;        // Dry LED ON
            WET_LED = 0;        // Wet LED OFF
        }
        else if(soil_moisture >= 250 && soil_moisture < 400){
            // Soil slightly dry - Motor OFF, Dry LED ON (warning)
            MOTOR_IN1 = 0;      // Motor OFF
            MOTOR_LED = 0;      // Motor LED OFF
            DRY_LED = 1;        // Dry LED ON (warning - getting dry)
            WET_LED = 0;        // Wet LED OFF
        }
        else{  
            // Soil moisture high - Normal conditions
            MOTOR_IN1 = 0;      // Motor OFF
            MOTOR_LED = 0;      // Motor LED OFF
            DRY_LED = 0;        // Dry LED OFF
            WET_LED = 1;        // Wet LED ON (soil is wet)
        }
        
        // Update LCD display
        LCD_SetCursor(1,0);
        sprintf(line1, "S:%4d T:%d", soil_moisture, temperature);
        LCD_String(line1);
        
        LCD_SetCursor(2,0);
        sprintf(line2, "R:%s M:%s", 
                (RAIN ? "YES" : "NO"), 
                (MOTOR_IN1 ? "ON" : "OFF"));
        LCD_String(line2);
        
        __delay_ms(500);
    }
}

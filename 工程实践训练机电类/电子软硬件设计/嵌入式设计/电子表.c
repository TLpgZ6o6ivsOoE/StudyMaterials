#include <STC8A.h>

typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;

#define DIGIT_SELECT_PORT P1
#define DIGIT_DATA_PORT   P0
#define BEEP_PORT         P4

sbit KEY_HOUR_UP    = P3^2;
sbit KEY_HOUR_DOWN  = P3^3;
sbit KEY_MINUTE_UP  = P3^4;
sbit KEY_MINUTE_DOWN= P3^5;
sbit KEY_MODE       = P4^0;
sbit BEEP           = P4^2;

const uint8_t DIGIT_ENCODING[] = {
    0xC0, 0xF9, 0xA4, 0xB0, 0x99, 
    0x92, 0x82, 0xF8, 0x80, 0x90, 0xBF
};

const uint8_t DIGIT_SELECT[] = {
    0x7F, 0xBF, 0xDF, 0xEF, 0xF7, 0xFB, 0xFD, 0xFE
};

typedef enum {
    MODE_TIME,    
    MODE_DATE,    
    MODE_ALARM      
} DisplayMode;

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} Time;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
} Date;

typedef struct {
    uint8_t hour;
    uint8_t minute;
    uint8_t active;
} Alarm;

volatile Time current_time = {0, 0, 0};
volatile Date current_date = {2023, 9, 22};
volatile Alarm alarm = {0, 0, 0};
volatile DisplayMode display_mode = MODE_TIME;
volatile uint8_t timer_20ms_count = 0;
volatile uint8_t beep_counter = 0;
volatile uint8_t beep_state = 0;
volatile uint8_t i = 0;
volatile uint8_t prev_days = 0;
volatile uint8_t Days_of_month = 0;

uint8_t key_state[5] = {0};
uint8_t key_pressed[5] = {0};

const uint8_t DAYS_IN_MONTH[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void System_Init(void);
void Timer0_Init(void);
void GPIO_Init(void);
void Display_Time(void);
void Display_Date(void);
void Display_Alarm(void);
void Process_Keys(void);
void Update_Time(void);
void Update_Date(void);
uint8_t Is_Leap_Year(uint16_t year);
uint8_t Get_Days_In_Month(uint16_t year, uint8_t month);
void Beep_Control(uint8_t state);

void Delay_ms(uint16_t ms) {
    uint16_t i, j;
    for(i = 0; i < ms; i++)
        for(j = 0; j < 1000; j++);
}

void System_Init(void) {
    GPIO_Init();
    Timer0_Init();
    EA = 1;
}

void GPIO_Init(void) {

    P0M1 = 0x00;
    P0M0 = 0xFF;
    P1M1 = 0x00;
    P1M0 = 0xFF;
    P2M1 = 0x00;
    P2M0 = 0xFF;
    
    DIGIT_DATA_PORT = 0xFF;
    DIGIT_SELECT_PORT = 0xFF;
    BEEP = 0;  // ?????
}

void Timer0_Init(void) {
    AUXR 	&= 	0x7F;   
    TMOD 	&= 	0xF0;   
    TL0 	= 	0xC0;     
    TH0 	= 	0x63;     
    TR0 	= 	1;        
    ET0 	= 	1;        
}

uint8_t Is_Leap_Year(uint16_t year) {
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0))
        return 1;
    return 0;
}

uint8_t Get_Days_In_Month(uint16_t year, uint8_t month) {
    if (month < 1 || month > 12) return 31;
    
    if (month == 2)
        return Is_Leap_Year(year) ? 29 : 28;
    
    return DAYS_IN_MONTH[month - 1];
}

void Update_Time(void) {
    current_time.second++;
    
    if (current_time.second >= 60) {
        current_time.second = 0;
        current_time.minute++;
        
        if (current_time.minute >= 60) {
            current_time.minute = 0;
            current_time.hour++;
            
            if (current_time.hour >= 24) {
                current_time.hour = 0;
                Update_Date();
            }
        }
    }
    
    if (alarm.active && current_time.hour == alarm.hour && 
        current_time.minute == alarm.minute && current_time.second == 0) {
        beep_state = 1;
        beep_counter = 0;
    }
}

void Update_Date(void) {
	

    if (current_date.month < 1) {
        current_date.month = 12;
        if (current_date.year > 0) {
            current_date.year--;
        }
    } else if (current_date.month > 12) {
        current_date.month = 1;
        current_date.year++;
    }
    
    Days_of_month = Get_Days_In_Month(current_date.year, current_date.month);
    
    if (current_date.day < 1) {
			
        current_date.month--;
        
        if (current_date.month < 1) {
            current_date.month = 12;
            if (current_date.year > 0) {
                current_date.year--;
            }
        }
        
        prev_days = Get_Days_In_Month(current_date.year, current_date.month);
        current_date.day = prev_days;
    } else if (current_date.day > Days_of_month) {
			
        current_date.month++;
        current_date.day = 1;
        
        if (current_date.month > 12) {
            current_date.month = 1;
            current_date.year++;
        }
    }
}

void Beep_Control(uint8_t state) {
    static uint8_t beep_pattern = 0;
    
    if (state) {
        if (beep_counter < 5) {
            BEEP = ~BEEP; 
        } else if (beep_counter < 10) {
            BEEP = 1;  
        } else {
            beep_counter = 0;
            beep_pattern++;
            
            if (beep_pattern >= 6) {
                beep_pattern = 0;
                beep_state = 0;
            }
        }
    } else {
        BEEP = 1;
    }
}

void Display_Time(void) {

		// Ð¡Ê±
    DIGIT_SELECT_PORT = DIGIT_SELECT[0];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_time.hour / 10];
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[1];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_time.hour % 10];
    Delay_ms(2);
    
		// ·Ö¸ô·û
    DIGIT_SELECT_PORT = DIGIT_SELECT[2];
    DIGIT_DATA_PORT = DIGIT_ENCODING[10];  
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[3];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_time.minute / 10];
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[4];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_time.minute % 10];
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[5];
    DIGIT_DATA_PORT = DIGIT_ENCODING[10];  // ??"-"
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[6];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_time.second / 10];
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[7];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_time.second % 10];
    Delay_ms(2);
}

void Display_Date(void) {
    DIGIT_SELECT_PORT = DIGIT_SELECT[0];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_date.year / 1000];
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[1];
    DIGIT_DATA_PORT = DIGIT_ENCODING[(current_date.year % 1000) / 100];
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[2];
    DIGIT_DATA_PORT = DIGIT_ENCODING[(current_date.year % 100) / 10];
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[3];
    DIGIT_DATA_PORT = DIGIT_ENCODING[(current_date.year % 10)] & 0x7f;
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[4];
    DIGIT_DATA_PORT = current_date.month < 10 ? 0xFF : DIGIT_ENCODING[current_date.month / 10];
    Delay_ms(2);
	
		DIGIT_SELECT_PORT = DIGIT_SELECT[5];
    DIGIT_DATA_PORT = DIGIT_ENCODING[(current_date.month % 10)] & 0x7f;
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[6];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_date.day / 10];
    Delay_ms(2);
		
		DIGIT_SELECT_PORT = DIGIT_SELECT[7];
    DIGIT_DATA_PORT = DIGIT_ENCODING[current_date.day % 10];
    Delay_ms(2);
}

void Display_Alarm(void) {

    DIGIT_SELECT_PORT = DIGIT_SELECT[0];
    DIGIT_DATA_PORT = DIGIT_ENCODING[alarm.hour / 10];
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[1];
    DIGIT_DATA_PORT = DIGIT_ENCODING[alarm.hour % 10];
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[2];
    DIGIT_DATA_PORT = DIGIT_ENCODING[10];  // ??"-"
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[3];
    DIGIT_DATA_PORT = DIGIT_ENCODING[alarm.minute / 10];
    Delay_ms(2);

    DIGIT_SELECT_PORT = DIGIT_SELECT[4];
    DIGIT_DATA_PORT = DIGIT_ENCODING[alarm.minute % 10];
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[5];
    DIGIT_DATA_PORT = 0xFF;
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[6];
    DIGIT_DATA_PORT = 0xFF;
    Delay_ms(2);
    
    DIGIT_SELECT_PORT = DIGIT_SELECT[7];
    DIGIT_DATA_PORT = 0xFF;
    Delay_ms(2);
}

void Process_Keys(void) {
    
    if (key_pressed[4]) {
        key_pressed[4] = 0;
        display_mode = (display_mode + 1) % 3;
    }
    
    switch (display_mode) {
        case MODE_TIME:
            if (key_pressed[0]) {
                key_pressed[0] = 0;
                current_time.hour = (current_time.hour + 1) % 24;
            }
            if (key_pressed[1]) {
                key_pressed[1] = 0;
                current_time.hour = (current_time.hour + 23) % 24;
            }
            if (key_pressed[2]) {
                key_pressed[2] = 0;
                current_time.minute = (current_time.minute + 1) % 60;
            }
            if (key_pressed[3]) {
                key_pressed[3] = 0;
                current_time.minute = (current_time.minute + 59) % 60;
            }
            break;
            
        case MODE_DATE:
						
            if (key_pressed[0]) {
                key_pressed[0] = 0;
                current_date.month += 1;
            }
						
            if (key_pressed[1]) {
                key_pressed[1] = 0;
                current_date.month -= 1;
            }
						
            if (key_pressed[2]) {
                key_pressed[2] = 0;
                current_date.day += 1;
            }
						
            if (key_pressed[3]) {
                key_pressed[3] = 0;

                current_date.day -= 1;
            }
						
						Update_Date();
						
            break;
            
        case MODE_ALARM:
            if (key_pressed[0]) {
                key_pressed[0] = 0;
                alarm.hour = (alarm.hour + 1) % 24;
                alarm.active = 1;
            }
            if (key_pressed[1]) {
                key_pressed[1] = 0;
                alarm.hour = (alarm.hour + 23) % 24;
                alarm.active = 1;
            }
            if (key_pressed[2]) {
                key_pressed[2] = 0;
                alarm.minute = (alarm.minute + 1) % 60;
                alarm.active = 1;
            }
            if (key_pressed[3]) {
                key_pressed[3] = 0;
                alarm.minute = (alarm.minute + 59) % 60;
                alarm.active = 1;
            }
            break;
    }
}

void main(void) {
    System_Init();
    
    while (1) {

        switch (display_mode) {
            case MODE_TIME:
                Display_Time();
                break;
            case MODE_DATE:
                Display_Date();
                break;
            case MODE_ALARM:
                Display_Alarm();
                break;
        }
        
        Process_Keys();
        Beep_Control(beep_state);
    }
}

void Timer0_ISR(void) interrupt 1 {
	
    TL0 = 0xC0;
    TH0 = 0x63;
    
    timer_20ms_count++;
    
    key_state[0] = (key_state[0] << 1) | KEY_HOUR_UP;
    key_state[1] = (key_state[1] << 1) | KEY_HOUR_DOWN;
    key_state[2] = (key_state[2] << 1) | KEY_MINUTE_UP;
    key_state[3] = (key_state[3] << 1) | KEY_MINUTE_DOWN;
    key_state[4] = (key_state[4] << 1) | KEY_MODE;
		
    for (i = 0; i < 5; i++) {
        if (key_state[i] == 0xF0) {
            key_pressed[i] = 1;
            key_state[i] = 0xFF; 
        }
    }
    
    if (timer_20ms_count >= 50) {
        timer_20ms_count = 0;
        Update_Time();
    }
    
    if (beep_state) {
        beep_counter++;
    }
}
#include <avr/io.h>
 
#include <avr/interrupt.h>
 
#include <util/delay.h>
 
#include "lcd.h"
 
 
 
void init_serial(void) ;  //  Serial 통신포트 초기화
 
void SerialPutChar(char ch);
 
void SerialPutString(char str[]);
 
 
 
#define  Avg_Num     4         //  이동 평균 갯수
 
#define  Amp_Gain   11         //  증폭기 이득
 
#define  GAS_Mode   1          //  GAS 센서 디스플레이 모드
 
 
void sound( void );        // 부저 호출 함수
void bluetooth ( void );   // 블루투스 경고 함수 호출
 
 
 
void HexToDec( unsigned short num, unsigned short radix);
 
char NumToAsc( unsigned char Num );				// 숫자를 문자(ASCII 코드)로 변환하는 함수
 
unsigned char AscToNum( char Asc ) ;      // 문자(ASCII 코드)를 숫자로 변환하는 함수
 
 
 
static volatile unsigned char cnumber[5] = {0, 0, 0, 0, 0};
 
 
 
void Display_Number_LCD( unsigned int num, unsigned char digit ) ;    // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이
 
void Display_TMP_LCD( unsigned int tp  )  ;                           // 온도를 10진수 형태로 LCD 에 디스플레이
 
 
 
void msec_delay(unsigned int n);
 
void usec_delay(unsigned int n);
 
void DC_FAN_Run_Fwd( short duty );    // FAN 정회전(PWM구동) 함수
 
 
 
static volatile unsigned short GAS_sensor_ouput= 10,  GAS_sensor_ouput_avg = 10 ;
 
static volatile unsigned char int_num = 0,  Sensor_Flag = GAS_Mode ;
 
static volatile  char  recv_cnt = 0, rdata=0, new_recv_flag = 0  ; //블루투스 통신에 필요
 
short duty = 0;
 
static volatile short  Vmax = 0 ;
 
int Flag;
 
 
 
static volatile  char  recv_data[30] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
 
 
 
 
 
 
int main()
{
 
//	unsigned char cmd_data = 0xFF  ;
 
 
 
	DDRB |= 0x10;     // LED (PB4 : 출력설정 )
 
	PORTB |= 0x10;    // PB4  : High ( LED OFF)
 
 
 
	DDRB |= 0x08;     // LED (PB3 : 출력설정 )
 
	PORTB |= 0x08;    // PB3  : High ( LED OFF)
 
 
 
	DDRB |= 0x04 ;    // 능동버저(Buzzer) ( PB2 : 출력 )
 
	PORTB |= 0x04 ;   // PB2 : Low (버저 OFF)
 
 
 
	DDRB |= 0x20;   // FAN구동신호 + 단자:  PWM 포트( pin: OC1A(PB5) )   --> 출력 설정
 
	DDRA |= 0x01;   // FAN구동신호 - 단자 : 범용 입/출력포트(pin : PA0 ) --> 출력 설정
 
 
 
	LcdInit();          // LCD 초기화
 
	init_serial() ;     // Serial Port (USART1) 초기화
 
	UCSR1B |=  0x80  ;  // UART1 송신(RX) 완료 인터럽트 허용
 
	sei() ;
 
 
 
////// FAN 구동신호 ( pin: OC1A(PB5) ),   Timer1, PWM signal (period= 200 usec )
 
 
 
	TCCR1A = 0x82;    // OC1A(PB5)) :  PWM 포트 설정,   Fast PWM ( mode 14 )
 
	TCCR1B = 0x1b;    // 64 분주 타이머 1 시작 (내부클럭 주기 =  64/(16*10^6) = 4 usec ),  Fast PWM ( mode 14 )
 
	ICR1 = 50;        // PWM 주기 = 50 * 4 usec = 200 usec (  PWM 주파수 = 1/200usec = 5 kHz )
 
 
 
  Vmax = ICR1;
 
 
 
	OCR1A = duty;      //  OC1A(PB5) PWM duty = 0 설정 : 모터 정지
 
////////////////////////////////////////////////////////////////////////////////
 
 
 
/*****   AD Converter **********/
 
 
 
	ADMUX &= ~0xE0;    //  ADC 기준전압 = AREF ,   ADC 결과 오른쪽정렬
 
	ADCSRA |= 0x87;    // ADC enable, Prescaler = 128
 
 
 
/**** Timer0 Overflow Interrupt  ******/
 
/**************************************/
 
	TCCR0 = 0x00;
 
  TCNT0 = 256 - 156;  //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,
 
                      //  오버플로인터럽트 주기 = 10msec
 
                      //  156 = 10msec/ 64use
 
 
 
	TIMSK = 0x01;  // Timer0 overflow interrupt enable
 
	sei();         // Global Interrupt Enable

  TCCR0 |= 0x07; // Clock Prescaler N=1024 (Timer 0 Start)
 
 
    while(1)
    {
 
 
	   if( GAS_sensor_ouput_avg <= 100 ) // 평상시
	   {
	    	LcdCommand(ALLCLR);
		 		LcdMove(0,0);
		 		LcdPuts("GAS Clean!!");

		 		LcdMove(1,0);
	   	 	LcdPuts("GAS = ");

		   	LcdMove(1,6);
       	Display_Number_LCD( GAS_sensor_ouput_avg, 4 );
 
		 		PORTB |= 0x10;   // PB4  : High ( LED OFF )
 
			 	PORTB &= ~0x08;   // PB3  : Low ( LED ON )
 
			 	PORTB |= 0x04;    // PB2  : High ( 부저 OFF )
 
		 		DC_FAN_Run_Fwd( 0 );     // FAN 정회전 정지
 
			 	Flag = 1;
     }
 
	   else if( GAS_sensor_ouput_avg > 100 ) // 가스 감지시
	   {
 
 		  	LcdCommand(ALLCLR);
		  	LcdMove(0,0);
	   		LcdPuts("GAS Detecting!!");
 
		  	LcdMove(1,0);
	   		LcdPuts("GAS = ");

		  	LcdMove(1,6);
      	Display_Number_LCD( GAS_sensor_ouput, 4 );
 
		  	PORTB &= ~0x10;   // PB4  : Low ( LED ON )
		  	PORTB |= 0x08;   // PB3  : High ( LED OFF )
 
		  	DC_FAN_Run_Fwd( 50 );     // FAN 정회전 스피드 50
		  	sound(); //부저 함수 호출
		  	bluetooth(); //블루투스 함수 호출
 
      }
 
   }
 
}
 
////////////////////////////////////////
////////////////////////////////////////
 
 
 
 
// UART1 수신 인터럽트 서비스 프로그램
 
ISR(  USART1_RX_vect )
{
 
    static unsigned char r_cnt = 0 ;
 
 
    rdata = UDR1;
 
 
    if( rdata != '.' )                   // 수신된 데이터가 마지막 문자를 나타내는 데이터(마침표)가 아니면
    {
        SerialPutChar( rdata);            // Echo  수신된 데이터를 바로 송신하여 수신된 데이터가 정확한지 확인
 
        recv_data[r_cnt] = rdata;         //  수신된 문자 저장
 
    		r_cnt++;                          //  수신 문자 갯수 증가
 
				new_recv_flag = 0;
 
    }
 
    else if(  rdata == '.' )                // 수신된데이터가 마지막 문자를 나타내는 데이터(마침표) 이면
    {
 
        SerialPutChar('\n');                // 휴대폰으로 데이터 전송시 Line Feed('\n')를 항상 끝에 전송해야함
 
        recv_cnt = r_cnt ;                  // 수신된 데이터 바이트수 저장
 
        r_cnt = 0;
 
				new_recv_flag = 1;
 
    }
 
}
 
 
 
void init_serial(void)
{
 
    UCSR1A=0x00;                    //초기화
 
    UCSR1B = 0x18  ;                //송수신허용,  송수신 인터럽트 금지
 
    UCSR1C=0x06;                    //데이터 전송비트 수 8비트로 설정.
 
 
 
    UBRR1H=0x00;
 
    UBRR1L=103;                     //Baud Rate 9600
 
}
 
 
 
//======================================
 
// 한 문자를 송신한다.
 
//======================================
 
 
 
void SerialPutChar(char ch)
{
 
	 while(!(UCSR1A & (1<<UDRE)));			// 버퍼가 빌 때를 기다림
 
  	 UDR1 = ch;												// 버퍼에 문자를 쓴다
 
}
 
 
 
//=============================================
 
// 문자열을 송신한다.
 
// 입력   : str - 송신한 문자열을 저장할 버퍼의 주소
 
//=============================================
 
 
 
void SerialPutString(char *str)
{
 
	 while(*str != '\0')
 
    {
 
      	SerialPutChar(*str++);
 
    }
 
}
 
 
 
 
 
ISR(TIMER0_OVF_vect)   // Timer0 overflow interrupt( 10 msec)  service routine
{
 
    static unsigned short  time_index = 0,  count1 = 0, GAS_Sum = 0 ;
 
    static unsigned short  GAS_sensor_ouput_buf[Avg_Num]   ;
 
 
    unsigned char i = 0 ;
 
 
    TCNT0 = 256 - 156;       //  내부클럭주기 = 1024/ (16x10^6) = 64 usec,
 
                             //  오버플로인터럽트 주기 = 10msec
 
                             //  156 = 10msec/ 64usec
 
 
    time_index++ ;
 
 
    if( time_index == 25 )    // 샘플링주기 =  250 msec = 10msec x 25
    {
 
        time_index = 0;
 
   /**************   GAS Sensor signal detection(AD 변환) ************/
 
 
 
	   	ADMUX &= ~0x1F;    //  ADC Chanel 0 : ADC0 선택
 
	   	ADMUX |= 0x01;
 
 
	   	ADCSRA |= 0x40;   // ADC start
 
 
 
	   	while( ( ADCSRA & 0x10 ) == 0x00  ) ;  // Check if ADC Conversion is completed
 
	    ADCSRA |= 0x10;  // 변환완료 비트 리셋
 
 
 
	   	GAS_sensor_ouput = ADC;
 
 
 
     /******************************************************/
 
 
   ////////////////////////////////////////////////////////////////////
 
   //////////                                               ///////////
 
   //////////  Avg_Num(4개) 개씩 이동 평균(Moving Average)  ///////////
 
   //////////                                               ///////////
 
   ////////////////////////////////////////////////////////////////////
 
 
 
	   if( count1 <= ( Avg_Num -1 ) )
	   {
       GAS_sensor_ouput_buf[ count1 ] = GAS_sensor_ouput ;
 
			 GAS_Sum +=  GAS_sensor_ouput_buf[ count1 ] ;
 

          ////////////////////////////////////////////////////
 

      //  TMP_sensor_ouput_buf[ count1 ] = TMP_sensor_ouput ;
 
			// TMP_Sum +=  TMP_sensor_ouput_buf[ count1 ] ;
 
 
	      count1++ ;
	   }
 
	   else
	   {
        GAS_Sum +=  GAS_sensor_ouput  ;	       // 가장 최근 값 더하고
 
        GAS_Sum -=  GAS_sensor_ouput_buf[ 0 ] ;   // 가장 오랜된 값 빼고

        GAS_sensor_ouput_avg = GAS_Sum / Avg_Num ;     // 4개 이동 평균
 
 
		   for( i = 0; i <= (Avg_Num - 2) ; i++ )
		   {
 
           GAS_sensor_ouput_buf[ i ]  = GAS_sensor_ouput_buf[ i+1 ] ;
 
		   }
 
           GAS_sensor_ouput_buf[ Avg_Num - 1 ]  = GAS_sensor_ouput ;
 
	   }

   }
 
}
 
 
 
void DC_FAN_Run_Fwd( short duty )   // FAN 정회전 함수
{
 
    if( duty > Vmax )     duty = Vmax ;
 
    PORTA &= ~0x01;     //  FAN구동신호 - 단자 : 0 V 인가( PA0 = 0 );
 
		OCR1A = duty;       //  FAN구동신호 + 단자 : OC1A(PB5) PWM duty 설정
 
}
 
 
 
 
void bluetooth ( void ) // 블루투스 경고 함수 호출
{
 
	if(Flag == 1)
	{
 
		SerialPutString( "Warning! Fire detected!\n" );
 
		Flag = 0;
 
	}
 
}
 
 
 
void sound( void ) // 부저 호출 함수
{
 
	PORTB &= ~0x04;    // PB2  : High ( 부저 ON )
 
	msec_delay(150);
 
	PORTB |= 0x04;    // PB2  : High ( 부저 OFF )
 
	msec_delay(150);
 
}
 
 
 
void Display_Number_LCD( unsigned int num, unsigned char digit )       // 부호없는 정수형 변수를 10진수 형태로 LCD 에 디스플레이
{
 
	HexToDec( num, 10); //10진수로 변환
 
 
 
	if( digit == 0 )     digit = 1 ;
 
	if( digit > 5 )      digit = 5 ;
 
  if( digit >= 5 )     LcdPutchar( NumToAsc(cnumber[4]) );  // 10000자리 디스플레이
 
	if( digit >= 4 )     LcdPutchar(NumToAsc(cnumber[3]));    // 1000자리 디스플레이

	if( digit >= 3 )     LcdPutchar(NumToAsc(cnumber[2]));    // 100자리 디스플레이
 
	if( digit >= 2 )     LcdPutchar(NumToAsc(cnumber[1]));    // 10자리 디스플레이
 
	if( digit >= 1 )     LcdPutchar(NumToAsc(cnumber[0]));    //  1자리 디스플레이
 
}
 
 
 
void Display_TMP_LCD( unsigned int tp  )       // 온도를 10진수 형태로 LCD 에 디스플레이
{
 
	HexToDec( tp, 10); //10진수로 변환
 
  LcdPutchar(NumToAsc(cnumber[2]) );   // 10자리 디스플레이
 
  LcdPutchar(NumToAsc(cnumber[1]));    // 1자리 디스플레이
 
  LcdPuts( ".");                       // 소숫점(.) 디스플레이

  LcdPutchar(NumToAsc(cnumber[0]));    // 0.1 자리 디스플레이
 
}
 
 
 
 
 
void HexToDec( unsigned short num, unsigned short radix)
{
 
	int j ;

	for(j=0; j<5 ; j++) cnumber[j] = 0 ;
 
	j=0;
 
	do
	{
		cnumber[j++] = num % radix ;
 
		num /= radix;
 
	} while(num);
 
}
 
 
 
char NumToAsc( unsigned char Num )
{
 
	if( Num <10 ) Num += 0x30;
 
	else          Num += 0x37;

	return Num ;
 
}
 
 
 
void msec_delay(unsigned int n)
{
 
	for(; n>0; n--)		// 1msec 시간 지연을 n회 반복
 
	_delay_ms(1);		// 1msec 시간 지연
 
}
 
 
 
void usec_delay(unsigned int n)
{
 
	for(; n>0; n--)		// 1usec 시간 지연을 n회 반복
 
	_delay_us(1);		// 1usec 시간 지연
 
}

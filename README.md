

## Atmega128을 이용한 가스감지기

![trace](/img/gas1.png)

## 기능
- 연기 감지기로 가스를 감지하여 FAN의 작동으로 인해 내부 공기를 순환한다.
- 부저를 통해 사람들에게 청각적으로 경고한다.
- 전달 블루투스 신호를 보내 관리자의 단말기에 보다 빠르게 경고를 알린다.

  #### 가스 감지시
  1. 경고음 발생
  2. LCD에서 현재 가스 농도 및 GAS Detection 문구 출력
  3. 붉은색 LED 점등
  4. 내부 공기 순환을 위한 FAN 회전
  5. 사용자 또는 관리자의 모바일 기기로 알림

  #### 평상시
  1. LCD에서 현재 가스 농도 및 GAS Clean 문구 출력
  2. 초록색 LED 점등
  3. FAN 정지

## 사용 부품
![trace](/img/gas3.png)

## 구현도
![trace](/img/gas2.png)

## 회로도
![trace](/img/gas4.png)

- 지금 생각해보면 회로도를 좀 더 깔끔하게 표현했어야 하는 아쉬움이 있다.

## 주요 코드
```C
if( GAS_sensor_ouput_avg <= 100 ) { // 평상시
   
      LcdCommand(ALLCLR);
      LcdMove(0,0);
      LcdPuts("GAS Clean!!");

      LcdMove(1,0);
      LcdPuts("GAS = ");

      LcdMove(1,6);
      Display_Number_LCD( GAS_sensor_ouput_avg, 4 );
 
      PORTB |= 0x10;    // PB4  : High ( LED OFF )
      PORTB &= ~0x08;   // PB3  : Low ( LED ON )
      PORTB |= 0x04;    // PB2  : High ( 부저 OFF )
 
      DC_FAN_Run_Fwd( 0 );     // FAN 정회전 정지
 
      Flag = 1;
    }
 
   else if( GAS_sensor_ouput_avg > 100 ) { // 가스 감지시
   
      LcdCommand(ALLCLR);
      LcdMove(0,0);
      LcdPuts("GAS Detecting!!");
 
      LcdMove(1,0);
      LcdPuts("GAS = ");

      LcdMove(1,6);
      Display_Number_LCD( GAS_sensor_ouput, 4 );
 
      PORTB &= ~0x10;     // PB4  : Low ( LED ON )
      PORTB |= 0x08;      // PB3  : High ( LED OFF )
 
      DC_FAN_Run_Fwd( 50 );     // FAN 정회전 스피드 50
      sound(); //부저 함수 호출
      bluetooth(); //블루투스 함수 호출
   }
```

## 제작 과정
![trace](/img/gas5.png)

## 시연영상  

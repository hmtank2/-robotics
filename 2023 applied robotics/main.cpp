/*
 * 0525 1 16bit struct3.cpp
 *
 * Created: 2023-05-24 오후 7:32:10
 * Author : hmtan
 */ 
/*
 * 0520 usart2 pwm 1 16bit.cpp
 *
 * 0520 USART2 printf.cpp
 * 유아트 응용? 통신 prinf 함수 사용?
 * Created: 2023-05-21 오전 2:40 
 * isr 내부 index 초기화 오류 수정.
 결과: 안 작동함.정확히는 리셋을 한 번 하면 딱 한번 전송가능함.
 * 3:06 프린트 스트링 제외하고 전부 비활성화 시키니 현재 상태로도 잘 작동함. 계속.
 //memmove 활성화 시키면 단 한번만 작동함.
 *3:23
 무한루프를 돌아서 그럼. i++ 위치 틀려서. 현재는 잘작동하나, rx_debug_num가 에러임.


*	5 23 14:30 main 함수 폴링 방식으로 받는건 기존보다 더 uart 누락이 심함. 
*	5 24 12:30
타이머를 하나만 쓰니까 훨씬 안정적임 뭐지
그리고 출력을 보면 일정한 패턴으로 별출력하듯이 usart가 누락되거나 추가되면서 나오는데
주기적으로 뭔가 에러를 유발하는게 여기 있나봄. 우선 구조체를 다시 도입하였음

 * Author : hmtan
 */ 

#include <avr/io.h>

#define F_CPU 16000000UL
#define TERMINATOR '\r' //문자열의 끝을 알리는 문자,'\n'이나 '\r'을 사용

#include <avr/iom128a.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include <stdio.h>
#include <string.h>
#include <stdint.h>


#define PWM_PERIOD 40000  // PWM 주기 설정 (20ms)
#define MIN_DUTY 2000     // 최소 듀티비 설정 (1ms)
#define MAX_DUTY 4000     // 최대 듀티비 설정 (2ms)
//#include "0520 USART1_ehcoing"



//char receive_buf=0;//유아트 인아웃풋 데이터 레지스터

//일단 이 둘은 전역변수로




float motor_y_deg=0;//초기값도 10이므로 처음 10이 아닌 값이 나올 때까지는if문 갔다와도 10
float motor_x_deg=0;

uint8_t index=0;

bool process;

class Uart
{//클래스 선언부
public://인터럽트 서비스 루틴때문에 이걸 전부 static으로 해야하나싶네. 인스턴스 하나밖에 없긴 한데
    char data[20]="";
    char outData[4]={0,0,0,0};
    int len=0;
    int idx=0;
    int lngth=0;
    bool complete= false;
	
	int motor_Y =10;
	int motor_X = 10;
    
	/*
	void Uart{//void 없나?
        len = 0;
        idx = 0;
        lngth = 0;
        complete = false;
		
		data = 0;
		outData[0] = {0,0,0,0};
    };*/
    void reset(){
        len = 0;
        idx = 0;
        lngth = 0;
        complete = false;
    };
    void asciiToInt();

    
};

//멤버함수 구현부
void Uart :: asciiToInt()
{

	if(this -> complete){
		int digit =0;
		this -> idx =0;
		int num_Y = 0;
		int num_X = 0;

		int flag =0; //0 y 1 x
		for(int i =0; i< this->lngth; i++) //uart1.lngth 
		{
			//digit++;
			int num = 0;//이
			if(this->data[i] ==' '){
				this -> motor_Y = num_Y;
				flag =1;
			}
			if(this -> data[i]=='\n'){this -> motor_X = num_X;}

			num = this ->data[i]-48;
			if(num>=0&&num<=9){
				if(flag ==0){
					num_Y *=10;
					num_Y += num;
				}
				else{
					num_X *=10;
					num_X += num;
				}
			}
		}
		this -> len = 0;
		this -> complete = false;
		//return this -> outData;

	}
}

Uart usart1;


void usart0_init()
{
	UCSR0A=0x00;
	UCSR0B=0x98;
	UCSR0C=0x06;
	UBRR0H=0;
	UBRR0L=103;
}


void USART0_tx(char data)//trasmit
{
	while(!(UCSR0A&(1<<UDRE0)));
	UDR0=data;
}

ISR(USART0_RX_vect){//루틴 너무 길어지는데 스트링 출력은 메인함수로 빠질수도 있음.
    
    while(!(UCSR0A&(1<<UDRE0)));
	//if((UCSR0A&(1<<DOR0)==true)){USART0_tx(50);}
    
	usart1.data[index++]= UDR0;
	if(usart1.data[index-1] == 10){
		//USART0_print_string(usart1.data);
        usart1.lngth = index; // 안쓰던데
        usart1.len = index;
        usart1.complete = true;
		//process = true;
        //index = 0;//이걸 안해주면 계속 쌓임. 아진짜? 그래서 루프를 돌면서 계속 쌓이는건가? 누락이 쌓이는 느낌이라면? 충분히 될거같음.

    }
	
}
//그러네 기존에는 인덱스를 재활용하니까 여기까지 와서야 초기화를 했구나.


void USART0_print_string(char *str)//print string
{
    for(int i=0;str[i] ;i++)
    {
        USART0_tx(str[i]);
    }
}

void USART0_print_1_byte_number(uint8_t n)//이제부터 이 함수에 패러미터는 반드시 사전에 아스키로 변환되어있어야.
{
    char numString[4] = "0";
    int i, index = 0;
    
    if(n > 0){
        for(i=0; n != 0 ; i++)
        {
            numString[i] = n % 10 + '0';//1의 자리 10의 자리 100의 자리 이렇게 감 빼면 특수기호, 더하면 0113 지옥이네.
            n = n / 10;//10으로 나눈 몫. 나머지 버리고 숫자 열배 작아지는 셈
        }
        numString[i] = '\0';
        
        index = i - 1;
    }
    for(i = index; i>=0; i--)
    {
        USART0_tx(numString[i]);
    }
    
    //else USART0_tx('0');
}



float pwm(float _deg)//pwm값을 받아서 실제 pwm값으로 변환
{
	//return 39999.0*(_deg/180);//4만이 20ms 1ms 2000
	return 2000 + (2000*(_deg/180)); // 이따가 180으로
}



void servo_init() {
	TIMSK |= (1 << OCIE1A);
	//ETIMSK |= (1 << OCIE3A);//마크스들의 경우는 재설정이 필요할 수 있음.
	// Timer/Counter1 설정 (OC1A 핀 사용)
	TCCR1A |= (1 << COM1A1) | (1 << COM1B1)| (1 << WGM11);  // 비교 일치 시 OC1A 핀 출력 클리어, 10비트 Fast PWM 모드
	//TCCR1A = 0xaa;
	
	TCCR1B |= (1 << WGM12) | (1 << WGM13) | (1 << CS11);  // Fast PWM 모드, 분주비 8 설정
	ICR1 = PWM_PERIOD - 1;  // PWM 주기 설정
	DDRB |= (1 << DDB5)|(1 << DDB6);  // OC1A 핀(PB5)을 출력으로 설정 + pb6, oc1b

	// Timer/Counter3 설정 (OC3A 핀 사용)
	//TCCR3A |= (1 << COM3A1) | (1 << WGM31);  // 비교 일치 시 OC3A 핀 출력 클리어, 10비트 Fast PWM 모드
	//TCCR3B |= (1 << WGM32) | (1 << WGM33) | (1 << CS31);  // Fast PWM 모드, 분주비 8 설정
	//ICR3 = PWM_PERIOD - 1;  // PWM 주기 설정
	//DDRE |= (1 << DDE3);  // OC3A 핀(PE3)을 출력으로 설정
	
	//OCR1A = 2000;
	//OCR1B = 2000;
}

void set_servo_position(uint16_t position_X, uint16_t position_Y) {
	OCR1A = position_X;  // Timer/Counter1의 출력 비교 값을 설정하여 듀티비 조정
	OCR1B = position_Y;  // Timer/Counter3의 출력 비교 값을 설정하여 듀티비 조정
}

int main(void){

    usart0_init();
	servo_init();
	sei();
    
	//Uart usart1;

	while(1)
	{

		if(usart1.complete == true)
		{//여기 고쳐야
			//USART0_print_string(usart1.data);
			usart1.asciiToInt();
			//USART0_tx(10);
			
			//USART0_print_1_byte_number(usart1.motor_Y);
			//USART0_print_1_byte_number(usart1.motor_X);
			
			//USART0_tx(10);
			//if(usart1.outData[0]==0){usart1.outData[0]=motor_y_deg;}
			//if(usart1.outData[1]==0){usart1.outData[1]=motor_x_deg;}
			
			motor_x_deg=usart1.motor_X;
			motor_y_deg=usart1.motor_Y;
			set_servo_position(pwm(motor_x_deg), pwm(motor_y_deg));//led용, 반드시 모터를 돌리기 전에 변환해야함
			//pb5 x축 큰거, pb6 y축 작은거.
			usart1.motor_X =0;
			usart1.motor_Y =0;//char 리셋을 안해선가.
		}
		/*
		if(process == true){
			USART0_print_string(usart1.data);
			//USART0_tx('32');
			process =false;
		}*/
		
	}
    return 0;
}




#include "stdafx.h"

volatile Point SelfPointArr[Max_Storage];
volatile Point BallPointArr[Max_Storage];
volatile s16 SelfAngleArr[Max_Storage];		//С������,absolute angle
volatile s16 TargetAngleArr[Max_Storage];	//�����С���ĽǶ�,absolute angle
volatile u8 moveState;				//0 for stop, 1 for rotate, 2 for straightfoward
volatile s8 currentIndex;									//current index in the array
volatile s8 rotateStartIndex; 							//index when starting rotate;
volatile uint8_t countNewPoint;

volatile MatchInfo info;

void Encoder_Init(void)
{		
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef   TIM_TimeBaseStructure;
  TIM_ICInitTypeDef  TIM_ICInitStructure;
	
	//motor feedback input
	//motor0 - PA0,PA1(TIM2 CH1,CH2)
	//motor1 - PA6,PA7(TIM3 CH1,CH2)
	//motor2 - PB6,PB7(TIM4 CH1,CH2)
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_0 | GPIO_Pin_1 |
																 GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOA, &GPIO_InitStructure);	
	
	GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_Init(GPIOB, &GPIO_InitStructure);
	
	//TIM_DeInit(TIM2);		//TIM2��λ
	//TIM_DeInit(TIM3);		//TIM3��λ
	//TIM_DeInit(TIM4);		//TIM4��λ
	
	TIM_TimeBaseStructure.TIM_Prescaler = 0x0;  // No prescaling 
  TIM_TimeBaseStructure.TIM_Period = 0xffff;  
  TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
  TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;   
  TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);
	
	TIM_EncoderInterfaceConfig(TIM2, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	TIM_EncoderInterfaceConfig(TIM3, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Rising);
	
  TIM_ICStructInit(&TIM_ICInitStructure); 
  TIM_ICInitStructure.TIM_ICFilter = 6;
  TIM_ICInit(TIM2, &TIM_ICInitStructure);
	TIM_ICInit(TIM3, &TIM_ICInitStructure);
	TIM_ICInit(TIM4, &TIM_ICInitStructure);
   
  //Reset counter
  TIM2 -> CNT = 0;
	TIM3 -> CNT = 0;
	TIM4 -> CNT = 0;
  TIM_Cmd(TIM2, ENABLE);
	TIM_Cmd(TIM3, ENABLE);
	TIM_Cmd(TIM4, ENABLE);
}

//�����Ĵ�����ֵ
void Encoder_Reset(void)
{
  TIM2 -> CNT = 0;
	TIM3 -> CNT = 0;
	TIM4 -> CNT = 0;
}

//incompleted
//�Ĵ���ֵ��ȡ
int Encoder_Read(int motornum)
{
	int count;
	switch(motornum){
		case 0:
			count = TIM2->CNT;break;
		case 1:
			count = TIM3->CNT;break;
		case 2:
			count = TIM4->CNT;break;
		default:
			count = -314;break;
	}
	return (s16)count;
}

//rotate is from -314 to 314

void RotateStop(int16_t rotateAngle){
	rotateAngle *= 11;
	rotateAngle /= 2;
	if((Encoder_Read(0) > rotateAngle || (-Encoder_Read(0)) > rotateAngle)){
			Motor_Speed_Control(0, 0);
			Motor_Speed_Control(0, 1);
			Motor_Speed_Control(0, 2);
			Encoder_Reset();
	}
}

void Rotate(s16 angle){
	moveState = 1;
	rotateStartIndex = currentIndex;
	if(angle > 0){
		Motor_Speed_Control(20,0);
		Motor_Speed_Control(20,1);
		Motor_Speed_Control(20,2);
	}
	else if(angle < 0){
			Motor_Speed_Control(-20,0);
			Motor_Speed_Control(-20,1);
			Motor_Speed_Control(-20,2);
		}
	else{
		Motor_Speed_Control(0,0);
		Motor_Speed_Control(0,1);
		Motor_Speed_Control(0,2);
	}
	Encoder_Reset();
}

void straightfoward(){
	moveState = 2;
	Motor_Speed_Control(-50,0);
	Motor_Speed_Control(50,2);
	Motor_Speed_Control(0,1);
}


int relaAngle(Point self, Point target){
	double theta = atan2(target.Y - self.Y, target.X - self.X);
	return (s16) (100*theta);
}

int moveAngle(Point current, Point prev){
	return relaAngle(prev, current);
}

void getSelfAngle(void)
{
	volatile float angle = -112 * AngYaw;
	if(angle > 314) angle -= 628;
	else
		if(angle < -314) angle += 628;
	printf("self:%f\n",angle);
	SelfAngleArr[currentIndex] = (int)angle;
}	

void Stop(void){
	moveState = 0;
	Motor_Speed_Control(0,0);
	Motor_Speed_Control(0,1);
	Motor_Speed_Control(0,2);
}

void move(void){
	//
	s16 relativeAngle = TargetAngleArr[currentIndex] - SelfAngleArr[currentIndex];
	if(relativeAngle > 314) {
		relativeAngle += 314;
		relativeAngle %= 628;
		relativeAngle -= 314;
	}
	else
		if(relativeAngle < -314) {
			relativeAngle *= -1;
			relativeAngle += 314;
			relativeAngle %= 628;
			relativeAngle -= 314;
			relativeAngle *= -1;
		}
	
	if(relativeAngle < 30 && relativeAngle > -30)
	{
		Stop();
	}
	else
	{
		Rotate(relativeAngle);
		printf("ball:%d\n",TargetAngleArr[currentIndex]);
	}
}

int Arrived(void)
{
	if(GetDistanceSquare(SelfPointArr[currentIndex],BallPointArr[currentIndex]) <= 100)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}


void addNewPoint(Point selfPoint, Point ballPoint)
{
	currentIndex++;
	currentIndex %= Max_Storage;
	SelfPointArr[currentIndex] = selfPoint;
	BallPointArr[currentIndex] = ballPoint;
	countNewPoint++;
}

uint32_t GetDistanceSquare(Point p1, Point p2)
{
	int32_t DeltaX, DeltaY;
	DeltaX = p1.X - p2.X;
	DeltaY = p1.Y - p2.Y;
	return DeltaX * DeltaX + DeltaY * DeltaY;
}
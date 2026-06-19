
//*********************韦恩斯塔克科技***********************//
//电机驱动例程参考，应用时需要实现底层代码。
//实现电机循环正反转运行，匀加减速。
//*********************************************************//


 void  PWM_out((unsigned char duty)
{
	
	//pwm占空比输出到P引脚
	
} 
void motor_stop(void)
{
	INA = 1;	//IO引脚操作
	INB = 1;		
	PWM_out(0);	
}
 
void motor_forward(void)
{
	INA = 0;
	INB = 1;	
}

void motor_backward(void)
{
	INA = 1;
	INB = 0;	
}
 
void  delay_ms(unsigned char ms)
{
	
	//毫秒延时
	
}

/******************** 主函数**************************/
void main(void)
{
    uint8_t i;
	
	sys_init();		//系统初始化
	IO_init();		//控制引脚初始化
	timer_init();	//配置定时器，pwm模式
	
	motor_stop();
	delay_ms(100);
	
    while (1)
    {
		motor_forward();	//电机正转设定
		for(i=0;i<100;i++)	//正向加速
		{
			PWM_out(i);
			delay_ms(2);
		}
		
		delay_ms(2000);		//转速保持
		
		for(i=100;i>0;i--)	//正向减速
		{
			PWM_out(i);
			delay_ms(2);
		}
		
		motor_backward();	//电机反转设定
		for(i=0;i<100;i++)	//反向加速
		{
			PWM_out(i);
			delay_ms(2);
		}
		
		delay_ms(2000);		//转速保持
		
		for(i=100;i>0;i--)	//反向减速
		{
			PWM_out(i);
			delay_ms(2);
		} 
    }
}
 

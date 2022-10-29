#include <reg52.h>

sbit PIN_RXD = P3^0;
sbit PIN_TXD = P3^1;

bit RxdEnd = 0;
bit TxdEnd = 0; 
bit RxdOrTxd = 0;
unsigned char RxdBuf = 0;
unsigned char TxdBuf = 0;

void ConfigUART(unsigned int baud);
void StartRXD();
void StartTXD(unsigned char dat);


/**
在信号接收这个程序里面，定时器并不是全程开启的：

而是在开始有输入信号后，开启定时器，
设置定时器计数间隔和传输的波特率相等，
这样在定时器每个中断里面去读取传输信号是0还是1，保存到buf，
最后检测结束位，读取完毕后，关闭定时器。

在发送信号的时候，开启定时器，设置定时器计数间隔和传输的波特率相等，
然后依次将起始位、数据位、结束位的 0|1 发送到传输引脚，
最后停止定时器。
*/
void main() {
	
	// 打开全局中断使能，需要用到定时器 
	EA = 1;
	ConfigUART(9600);
	
	while (1) {
		while (PIN_RXD); // 等待一个低电平的输入信号的起始符
		StartRXD(); // 开始接收输入信号
		while (!RxdEnd); // 在输入信号的终止符来临之前，一直接收
		
		StartTXD(RxdBuf+1); // 开始输出信号，发生的信号是输入信号+1后的值
		while(!TxdEnd); // 等待信号发送完毕
		
		// 其中 RxdEnd, TxdEnd 终止符的更新是在定时器中断里面进行检测的
	}

}

// 配置波特率等，常用的有 9600
// 波特率对应到定时器的计时间隔时间
void ConfigUART(unsigned int baud) {
	// 使用 Timer0
	TMOD &= 0xF0;
	// 8位自动重装模式，定时器溢出后THn自动重装到TLn，
	// 可以不进入中断即可重载TH0,TL0
	TMOD |= 0x02; 
	TH0 = 256 - (11059200/12)/baud;
}

void StartRXD() {
	// 在读取每个周期的时候，防止读取到周期切换间隔的误差，
	// 所以延时到单次周期的一半，才去读一次数据，
	// 第一次开始读的时刻是 256 - 波特率时间的一半
	TL0 = 256 - ((256-TH0) >> 1);
	
	ET0 = 1; // enable T0
	TR0 = 1; // start T0
	RxdEnd = 0;
	RxdOrTxd = 0; // 设置当前状态为读取
}

void StartTXD(unsigned char dat) {
	TxdBuf = dat;
	TL0 = TH0;
	ET0 = 1; // enable T0
	TR0 = 1; // start T0
	PIN_TXD = 0; // set start bit
	TxdEnd = 0;
	RxdOrTxd = 1; // 设置当前状态为发送
}

void InterruptTimer0() interrupt 1 {
	// signal [1 8 1]
	static unsigned char cnt = 0;
	if (RxdOrTxd) { // 发送状态
		if (cnt == 0) {
			PIN_TXD = 0; // send start bit
			cnt++;
		} else if (cnt <= 8) { // set data bits
			PIN_TXD = TxdBuf & 0x01; // send last bit of TxdBuf
			TxdBuf >>= 1;
		} else if (cnt == 9) {
			PIN_TXD = 1; // send finish bit
		} else {
			cnt = 0;
			TR0 = 0;
			TxdEnd = 1; // 设置发送结束标识符
		}
	} else { // 接收状态
		if (cnt == 0) { // 处理输入信号的第一位
			if (!PIN_RXD) { // 确认一下输入信号的起始位为0
				RxdBuf = 0;
				cnt++;
			} else { 
				// 在错位了半个周期后读取到的起始信号不为0，
				// 说明是干扰信号，不进行读取
				TR0 = 0;
			}
		} else if (cnt <= 8) { // 读取输入信号的连续8位
			RxdBuf >>= 1; // 0xxx xxxx
			if (PIN_RXD) {
				RxdBuf |= 0x80; // 1xxx xxxx 
			}
			cnt++;
		} else { // 处理接收停止位
			cnt = 0;
			TR0 = 0;
			if (PIN_RXD) { // 判断是否是停止位，设置标识符
				RxdEnd = 1;
			}
		}
	}
}










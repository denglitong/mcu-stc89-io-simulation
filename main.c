#include <reg52.h>

sbit PIN_RXD = P3^0;
sbit PIN_TXD = P3^1;

bit RxdOrTxd = 0;
bit RxdEnd = 0;
bit TxdEnd = 0; 
unsigned char RxdBuf = 0;
unsigned char TxdBuf = 0;

void ConfigUART(unsigned int baud);
void StartTXD(unsigned char dat);
void StartRXD();


/**
���źŽ�������������棬��ʱ��������ȫ�̿����ģ�

�����ڿ�ʼ�������źź󣬿�����ʱ����
���ö�ʱ����������ʹ���Ĳ�������ȣ�
�����ڶ�ʱ��ÿ���ж�����ȥ��ȡ�����ź���0����1�����浽buf��
��������λ����ȡ��Ϻ󣬹رն�ʱ����

�ڷ����źŵ�ʱ�򣬿�����ʱ�������ö�ʱ����������ʹ���Ĳ�������ȣ�
Ȼ�����ν���ʼλ������λ������λ�� 0|1 ���͵��������ţ�
���ֹͣ��ʱ����

Keil 4 ������ע��ʹ�� GB2312 ������Դ�
*/
void main() {
	
	// ��ȫ���ж�ʹ�ܣ���Ҫ�õ���ʱ�� 
	EA = 1;
	ConfigUART(9600);
	
	while (1) {
		while (PIN_RXD); // �ȴ�һ���͵�ƽ�������źŵ���ʼ��
		StartRXD(); // ��ʼ���������ź�
		while (!RxdEnd); // �������źŵ���ֹ������֮ǰ��һֱ����
		
		StartTXD(RxdBuf+1); // ��ʼ����źţ��������ź��������ź�+1���ֵ
		while(!TxdEnd); // �ȴ��źŷ������
		
		// ���� RxdEnd, TxdEnd ��ֹ���ĸ������ڶ�ʱ���ж�������м���
	}

}

// ���ò����ʵȣ����õ��� 9600
// �����ʶ�Ӧ����ʱ���ļ�ʱ���ʱ��
void ConfigUART(unsigned int baud) {
	// ʹ�� Timer0
	TMOD &= 0xF0;
	// 8λ�Զ���װģʽ����ʱ�������THn�Զ���װ��TLn��
	// ���Բ������жϼ�������TH0,TL0
	TMOD |= 0x02; 
	TH0 = 256 - (11059200/12)/baud;
}

void StartRXD() {
	// �ڶ�ȡÿ�����ڵ�ʱ�򣬷�ֹ��ȡ�������л��������
	// ������ʱ���������ڵ�һ�룬��ȥ��һ�����ݣ�
	// ��һ�ο�ʼ����ʱ���� 256 - ������ʱ���һ��
	TL0 = 256 - ((256-TH0)>>1);
	
	ET0 = 1; // enable T0
	TR0 = 1; // start T0
	RxdEnd = 0;
	RxdOrTxd = 0; // ���õ�ǰ״̬Ϊ��ȡ
}

void StartTXD(unsigned char dat) {
	TxdBuf = dat;
	TL0 = TH0;
	ET0 = 1; // enable T0
	TR0 = 1; // start T0
	PIN_TXD = 0; // set start bit
	TxdEnd = 0;
	RxdOrTxd = 1; // ���õ�ǰ״̬Ϊ����
}

void InterruptTimer0() interrupt 1 {
	// signal [1 8 1]
	static unsigned char cnt = 0;
	if (RxdOrTxd) { // ����״̬
		cnt++;
		if (cnt <= 8) { // set data bits
			PIN_TXD = TxdBuf & 0x01; // send last bit of TxdBuf
			TxdBuf >>= 1;
		} else if (cnt == 9) {
			PIN_TXD = 1; // send finish bit
		} else {
			cnt = 0;
			TR0 = 0;
			TxdEnd = 1; // ���÷��ͽ�����ʶ��
		}
	} else { // ����״̬
		if (cnt == 0) { // ���������źŵĵ�һλ
			if (!PIN_RXD) { // ȷ��һ�������źŵ���ʼλΪ0
				RxdBuf = 0;
				cnt++;
			} else { 
				// �ڴ�λ�˰�����ں��ȡ������ʼ�źŲ�Ϊ0��
				// ˵���Ǹ����źţ������ж�ȡ
				TR0 = 0;
			}
		} else if (cnt <= 8) { // ��ȡ�����źŵ�����8λ
			RxdBuf >>= 1; // 0xxx xxxx
			if (PIN_RXD) {
				RxdBuf |= 0x80; // 1xxx xxxx 
			}
			cnt++;
		} else { // �������ֹͣλ
			cnt = 0;
			TR0 = 0;
			if (PIN_RXD) { // �ж��Ƿ���ֹͣλ�����ñ�ʶ��
				RxdEnd = 1;
			}
		}
	}
}

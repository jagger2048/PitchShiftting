// wavread.cpp: 定义控制台应用程序的入口点
#include <stdafx.h>
#include <iostream>
#include <fstream>
#include <typeinfo.h>
#include "fftw3.h"
#include <math.h>
#include <vector>
#define PI 3.1415926535897
using namespace std;

class wavfile
{
public:
unsigned short cahnnel;			// 声道数
unsigned int frequency;			// 采样频率
unsigned int byte_rate;			// 比特率，frequency * cahnnel * bit_per_sample /8
unsigned short bit_per_sample;	// 采样位数 8位或者16 24等
unsigned int file_size;			// 文件大小
unsigned long data_size;		// 实际数据文件大小（字节形式）
unsigned char *original_data;	// 实际存储的数据 （从字节形式进行转化)
double *norm_data;				// 归一化数据
double *data;					// default output is mono data
unsigned long len;				// 实际点数,data_size/(bit_per_sample/8) /channel
double duration;				// 持续时间(second)
bool is_open;
wavfile();
~wavfile();
int read(string file_neme);		// read the wav file
void show();					// show wav file message
int write(string file_name);	// write data to wav file
private:

};

wavfile::wavfile()
{
	// 构造函数 相当于初始化

}

wavfile::~wavfile()
{
	// 析构函数 释放资源
	this->is_open = false;
}

int wavfile::read(string file_neme)
{	
	ifstream fp;																	// 输入时 ifstream
	fp.open(file_neme, ios::in | ios::binary);										// 读取文件 try except
	if (!fp.is_open()) {
		cout << "Can not open it." << endl;
		fp.close();
		return false;
	}
	fp.seekg(0, ios::end);						// seek get
	this->file_size = fp.tellg();					// 从后往前，输出的是整个文件的大小(字节形式)
	fp.seekg(0x16);
	fp.read((char*)&this->cahnnel, 2);		// 声道
	fp.seekg(0x18);								// offset 24 is num channels
	fp.read((char *)&this->frequency, 4);			// 采样频率
	fp.read((char*)&this->byte_rate, 4);			// 比特率
	fp.seekg(0x22);
	fp.read((char*)&this->bit_per_sample, 2);		// 采样位数	
	fp.seekg(0x28);
	fp.read((char*)&this->data_size, 4);

	this->original_data = new unsigned  char[this->data_size];// 开辟数据空间接收

	fp.seekg(0x2c);								// 44开始是data域的东西
	fp.read((char*)this->original_data, this->data_size);

	this->norm_data = new double[this->data_size / 2];
	for (unsigned long win_len = 0; win_len < this->data_size; win_len = win_len + 2) {
		long temp_value = 0;
		if ((this->original_data[win_len + 1] >> 7) == 1) {								// 判断正负数,x86系统中是小端模式，低位低地址，高位高地址，两字节
			temp_value = this->original_data[win_len] | (this->original_data[win_len + 1] << 8) - 65536;						
		}
		else {
			temp_value = this->original_data[win_len] | this->original_data[win_len + 1] << 8;		// 正数
		}
		this->norm_data[win_len / 2] = (double)temp_value / 32768.0;							// 归一化

	}

	// output data

	if (this->cahnnel > 1) {
		this->data = new double[this->data_size / 4];
		long count = 0;
		for (long win_len = 0; win_len < this->data_size / 4; win_len += 2) {
			this->data[count++] = (this->norm_data[win_len] + this->norm_data[win_len + 1]) / 2.0;
		}
	}
	else
		this->data = this->norm_data;
	fp.close();
	this->len = this->data_size / 4;
	this->duration = this->len / this->frequency;
	this->is_open = true;
	return true;
}

void wavfile::show()
{
	if (this->is_open) {
		cout << "The Wav file message:" << endl
			<< "Frequency	: " << this->frequency <<" Hz"<<endl
			<< "Bit_per_sample	: " << this->bit_per_sample <<" bps"<<endl
			<< "Size	: "<<this->len<<" samples"<<endl
			<< "Duration: " << this->duration << " seconds" << endl;
	}
	else
		cout << "Have not open any wav file." << endl;
}

int wavfile::write(string file_name)
{	
	ofstream fp;
	fp.open(file_name, ios::out || ios::binary);
	//fp.write()
	return 0;
}

int main(void)
{
	// 换电脑时需要更新解决方案(清理+重定向项目SDK) 
	const string file_name = "D:\\Project\\effect_c\\audioData\\Long Live.wav";				// 需要使用绝对地址 work-pc
	//const string file_name = "D:\\WorkSpace\\Matlab\\GPE\\Audio Data\\Long Live.wav";		// 需要使用绝对地址 home-pc
	wavfile wav;
	wav.read(file_name);
	wav.show();
	// fftw test


	const int win_len = 256;								//win_len ,fft number
	fftw_complex *fft_in, *fft_out,*ifft_in,*ifft_out;
	fftw_plan fft,ifft;
	fft_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*win_len);
	fft_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*win_len);
	ifft_in = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*win_len);
	ifft_out = (fftw_complex*)fftw_malloc(sizeof(fftw_complex)*win_len);
	fft = fftw_plan_dft_1d(win_len, fft_in, fft_out, FFTW_FORWARD, FFTW_ESTIMATE);// FFT
	ifft = fftw_plan_dft_1d(win_len, ifft_in, ifft_out, FFTW_BACKWARD, FFTW_ESTIMATE);// IFFT


	double input[win_len];
	const unsigned int ana_len = 64;					// analysis length
	const unsigned int syn_len = 90;					// synthesis length
	unsigned int ana_count = ( wav.len - (win_len-ana_len) ) / ana_len;
	// fft 相关
	double real[win_len];// 实部
	double imag[win_len];// 虚部
	double mag[win_len]; // 幅值
	double angle[win_len] = {0};// 相角
	double angle_pre[win_len];
	bool is_firstTime = true;
	double angle_syn[win_len];

	double omega[win_len];
	for (int n = 0; n < win_len; n++) {
		omega[n] = 2 * PI*double(n) / double(win_len);
	}

	double hanning_win[win_len];							// hanning window
	for (int n = 0; n < win_len; n++) {
		hanning_win[n] = 0.5*(1 - cos(2 * PI* n / win_len));
	}

	double overlap_buff[256] = {0};
	// 分帧
	ana_count = 44100*120/ana_len;// test 5000-10 seconds/ 29400-40s
	// test 
	ofstream audio_out;// 输出值txt进行查看 test
	audio_out.open("audio6.txt", ios::out | ios::ate);//  最终输出的音频
	//ofstream original_data;
	//original_data.open("original_data.txt", ios::out | ios::ate); // 输入的测试音频
	//ofstream ifft_check;
	//ifft_check.open("ifft_check.txt", ios::out | ios::ate);// ifft output to check
	//ofstream ifft_data;
	//ifft_data.open("ifft.txt", ios::out | ios::ate); // real and imag data to ifft
	//ofstream real_imag;
	//real_imag.open("real_imag.txt", ios::out | ios::ate); // real and imag

	vector<double> original_input;
	vector<double> syn_angle;

	for (int i=0,offset=0; i < ana_count; i++,offset++) {			//	以ana_len 为间隔向前移动，取出win_len点数据
		for (int n = 0; n < win_len; n++) {
			input[n] = wav.data[offset*ana_len +n];
			fft_in[n][0] = input[n] * hanning_win[n]; // 加窗
			fft_in[n][1] = 0;
			original_input.push_back(input[n]);// test 记录即将进行处理的原始数据
			//original_data << input[n]<<endl;// test original data input 
		}

		fftw_execute(fft);// 对输入信号使用fft
		for (int n = 0; n < win_len; n++) {
			real[n] = fft_out[n][0];// 保存实部
			imag[n] = fft_out[n][1];// 虚部
			angle_pre[n] = angle[n];
			// Note: notic the difference between real() and abs()
			mag[n] = sqrt(real[n] * real[n] + imag[n] * imag[n]);// 重新加上了根号

			//real_imag << real[n]<<" "<< imag[n]<< endl;// output for test

			// Note：angle(x+yj) = actan2(y,x);
			angle[n] = atan2(imag[n],real[n]);// 求出相位 之前real imag的方向反了
			// phase unwrap
			//angle_out << angle[n]<<endl;// 通过
			double delta = (angle[n] - angle_pre[n]) - omega[n] * ana_len; // phi 数组！
			double delta_phi = delta - round( delta / (2 * PI) ) * (2 * PI);// phi 
			double y_unwrap = delta_phi/ana_len + omega[n];// w
			//delta_phi_out << delta_phi<<endl;// delta_phi output for test 未通过

			if (is_firstTime) {
				angle_syn[n] = angle[n];// 第一帧时进行赋值
				
			}
			else {
				angle_syn[n] = angle_syn[n] + y_unwrap * syn_len;// phi ,angle shifting
			}


			syn_angle.push_back(angle_syn[n]);// test 记录同步相位
			// prepare for ifft
			ifft_in[n][0] = mag[n] * cos(angle_syn[n]);// 还原实部
			ifft_in[n][1] = mag[n] * sin(angle_syn[n]);// 还原虚部

			//ifft_data << ifft_in[n][0] << " " << ifft_in[n][1] <<endl;// test
		}
		is_firstTime = false;

		fftw_execute(ifft);// fftw ifft输出的值没有进行归一化，需要除以ifft_points 才与matlab中的结果相等
		//确认加入ifft之后的数据是否一致
		// 使用ifft_out中的实数部分进行synthesis and overlap add
 
		double toSOA[win_len];
		double overlap[win_len - syn_len];
		for (int n = 0; n < win_len; n++) {
			toSOA[n]=hanning_win[n]*ifft_out[n][0] / double(win_len);// 归一化 加窗

			//ifft_check << ifft_out[n][0] << " " << ifft_out[n][1] << endl;// check the ifft data

			if (n < win_len - syn_len) {
				overlap[n] = overlap_buff[syn_len + n] + toSOA[n];// overlap add
				overlap_buff[n] = overlap[n];
			}
			else
			{
				overlap_buff[n] = toSOA[n];
			}
		}
		for (int output_n = 0; output_n < syn_len; output_n++) {
			audio_out << overlap_buff[output_n] << endl;// test
		}
		
		// output is in overlap_buff[0:syn_len]
	}
	//int nsize = original_input.size();
	//for (int n = 0; n < nsize; n++) {
	//	fo << original_input[n] << " " << syn_angle[n] << endl;
	//}
	// 释放资源
	fftw_free(fft_in);
	fftw_free(fft_out);
	fftw_free(ifft_in);
	fftw_free(ifft_out);
	fftw_destroy_plan(fft);
	fftw_destroy_plan(ifft);
	//cout << "end here" << endl;
	//system("pause");
	return 0;
}
/*
TODO：
	-1. 封装成类，方便调用
	2. 优化双声道数据读取
	3. 使用向量的形式进行操作

	已知BUG：
		1. 所换算出来的值比matlab中的值(浮点)少一半，不知为何，需要重新校验matlab中的解码方式，或者
			自己编写的解码方式，就目前来说影响不大。
	
*/
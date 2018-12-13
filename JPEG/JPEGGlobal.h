#ifndef JPEGGLOBAL_H
#define JPEGGLOBAL_H
#include<iostream>
#include<fstream>
#include<exception>
#include<cstdlib>
#include<cmath>
#include<string>
#include<cstring>
#include<vector>
#include<set>
#include<iterator>
#include<thread>
#include<atomic>
#include"define.h"
#define PI 3.1415926535897932f

typedef unsigned char BYTE;	//1byte
typedef signed char SBYTE;	//1byte signed
typedef unsigned short WORD;//2byte
typedef short SWORD;        //2byte signed
typedef unsigned int DWORD;//4byte
typedef int SDWORD;		//4byte signed

#pragma pack(push)
#pragma pack(1)

class APP0 {
public:
	//	BYTE sign;//段标记0xFF
	//	BYTE type;//段类型0xE0
	BYTE lengthH;//段长度高字节
	BYTE lengthL;//段长度低字节
	BYTE jFIF4;//‘J'
	BYTE jFIF3;//‘F'
	BYTE jFIF2;//‘I'
	BYTE jFIF1;//‘F'
	BYTE jFIF0;//NULL
	BYTE edition;//主版本号
	BYTE subEdition;//次版本号
	BYTE density;//密度单位
	BYTE xDensityH;//X像素密度高字节
	BYTE xDensityL;//X像素密度低字节
	BYTE yDensityH;//y像素密度高字节
	BYTE yDensityL;//y像素密度低字节
	BYTE xThumbnail;//缩率图X像素数
	BYTE yThumbnail;//缩略图y像素数
	WORD length() { return lengthH * 0x100 + lengthL; }//求段长度
														 //求长度仅供程序检查，不用于写入文件
	DWORD jFIF() { return jFIF4 * 0x1000000 + jFIF3 * 0x10000 + jFIF2 * 0x100 + jFIF1; }//求“jFIF”
	WORD xDensity() { return xDensityH * 0x100 + xDensityL; }//求X像素密度
	WORD yDensity() { return yDensityH * 0x100 + yDensityL; }//求y像素密度
																  //大多数文件没有缩略图
	APP0(BYTE lengthH, BYTE lengthL, BYTE jFIF4, BYTE jFIF3, BYTE jFIF2, BYTE jFIF1, BYTE jFIF0, BYTE edition, BYTE subEdition,
			BYTE density, BYTE xDensityH, BYTE xDensityL, BYTE yDensityH, BYTE yDensityL, BYTE xThumbnail, BYTE yThumbnail){
		this->lengthH = lengthH; this->lengthL = lengthL;
		this->jFIF4 = jFIF4; this->jFIF3 = jFIF3; this->jFIF2 = jFIF2; this->jFIF1 = jFIF1; this->jFIF0 = jFIF0;
		this->edition = edition; this->subEdition = subEdition;
		this->density = density; this->xDensityH = xDensityH; this->xDensityL = xDensityL; this->yDensityH = yDensityH; this->yDensityL = yDensityL;
		this->xThumbnail = xThumbnail; this->yThumbnail = yThumbnail;
	}
	APP0(){
	/*	this->lengthH = 0; this->lengthL = 0;
		this->jFIF4 = 0; this->jFIF3 = 0; this->jFIF2 = 0; this->jFIF1 = 0; this->jFIF0 = 0;
		this->edition = 0; this->subEdition = 0;
		this->density = 0; this->xDensityH = 0; this->xDensityL = 0; this->yDensityH = 0; this->yDensityL = 0;
		this->xThumbnail = 0; this->yThumbnail = 0;*/
	}
};
class DQT {
public:
	//	BYTE sign;//段标记0xFF
	//	BYTE type;//段类型0xdb
	//BYTE lengthH;//段长度高字节
	//BYTE lengthL;//段长度低字节
	//WORD length() { return lengthH * 0x100 + lengthL; }//求段长度
	BYTE qtInfo;//QT信息 0-3位QT号 4-7位QT精度
	BYTE qtNumber() { return qtInfo - 16 * qtPrecision(); }//低位
	BYTE qtPrecision() { return qtInfo / 16; }//高位
	//	QT 64*QT精度
	DQT(BYTE qtInfo){this->qtInfo = qtInfo;}
	DQT(){/*this->qtInfo = 0;*/}
};
class SOF {
public:
	//0xffc0
	BYTE lengthH;//段长度高字节
	BYTE lengthL;//段长度低字节
	WORD length() { return lengthH * 0x100 + lengthL; }//求段长度
	BYTE samplePrecise;
	BYTE heightH;
	BYTE heightL;
	WORD height() { return  heightH * 0x100 + heightL; }//图像高度
	BYTE widthH;
	BYTE widthL;
	WORD width() { return  widthH * 0x100 + widthL; }//图像宽度
	BYTE numberOfColor;//颜色分量数量
	//class SOFColour
	SOF(BYTE lengthH, BYTE lengthL, BYTE samplePrecise, BYTE heightH, BYTE heightL, BYTE widthH, BYTE widthL, BYTE numberOfColor){
		this->lengthH = lengthH; this->lengthL = lengthL; this->samplePrecise = samplePrecise;
		this->heightH = heightH; this->heightL = heightL; this->widthH = widthH; this->widthL = widthL; this->numberOfColor = numberOfColor;
	}
	SOF(){
	/*	this->lengthH = 0; this->lengthL = 0; this->samplePrecise = 0;
		this->heightH = 0; this->heightL = 0; this->widthH = 0; this->widthL = 0; this->numberOfColor = 0;*/
	}
};
class SOFColour {//颜色分量信息
public:
	BYTE colorID;
	BYTE samplingCoefficient;//高4位：水平采样因子 低4位：垂直采样因子
	BYTE qtUsed;//当前分量使用的量化表的ID
	BYTE verticalSamplingCoefficient() { return samplingCoefficient & 0xff; }
	BYTE horizontalSamplingCoefficient() { return samplingCoefficient >> 4; }
	SOFColour(BYTE colorID, BYTE samplingCoefficient, BYTE qtUsed){
		this->colorID = colorID; this->samplingCoefficient = samplingCoefficient; this->qtUsed = qtUsed;
	}
	SOFColour(){
	/*	this->colorID = 0; this->samplingCoefficient = 0; this->qtUsed = 0;*/
	}
};
class DHT {
public:
	//0xffc4
	//BYTE lengthH;//段长度高字节
	//BYTE lengthL;//段长度低字节
	//WORD length() { return lengthH * 0x100 + lengthL; }//求段长度
	BYTE htInfo;//ht信息 0~3位ht号 4位ht类型0=DC表1=AC表 5~7位0
	BYTE htNumber() { return htInfo & 0xff; }//0~3位ht号
	BYTE htType() { return htInfo >> 4; }//4位ht类型0=DC表1=AC表
	BYTE htBitTable[16];//ht位表
	WORD htBitTableSum() {
		WORD temp = 0;
		for (int i = 0; i < 16; i++)
			temp += htBitTable[i];
		return temp;
	};//16个数求和 n<=256
	//n ht值表
	DHT(BYTE htInfo, const BYTE HT_BIT_TABLE[16]){
		this->htInfo = htInfo;
		for(int i=0;i<16;i++){
			this->htBitTable[i] = HT_BIT_TABLE[i];
		}
	}
	DHT(){
	/*	this->htInfo = htInfo;
		for(int i=0;i<16;i++){
			this->htBitTable[i] = 0;
		}*/
	}
};
class SOS {
public:
	//0xffda
	BYTE lengthH;//段长度高字节
	BYTE lengthL;//段长度低字节
	WORD length() { return lengthH * 0x100 + lengthL; }//求段长度
	BYTE numberOfColor;//颜色分量数量
	SOS(BYTE lengthH, BYTE lengthL, BYTE numberOfColor){
		this->lengthH = lengthH; this->lengthL = lengthL; this->numberOfColor = numberOfColor;
	}
	SOS(){}
};
class SOSColour {//颜色分量信息
public:
	BYTE colorID;
	BYTE htTableUsed;// 高4位：直流分量使用的哈夫曼树编号  低4位：交流分量使用的哈夫曼树编号
	BYTE acHtTableUsed() { return htTableUsed & 0xff; }//低位
	BYTE dcHtTableUsed() { return htTableUsed >> 4; }//高位
	SOSColour(BYTE colorID, BYTE htTableUsed){
		this->colorID = colorID; this->htTableUsed = htTableUsed;
	}
	SOSColour(){}
};
class DRI {
public:
	BYTE lengthH;//段长度高字节
	BYTE lengthL;//段长度低字节
	WORD length() { return lengthH * 0x100 + lengthL; }//求段长度
	BYTE stepH;//段长度高字节
	BYTE stepL;//段长度低字节
	WORD step() { return stepH * 0x100 + stepL; }//间隔长度
	DRI(BYTE lengthH, BYTE lengthL, BYTE stepH, BYTE stepL){
		this->lengthH = lengthH; this->lengthL = lengthL;
		this->stepH = stepH; this->stepL = stepL;
	}
	DRI(){}
};//RST表示将哈夫曼解码数据流复位，DC从0开始。RST标记共有8个：RST0~RST7
#pragma pack(pop)

  /*
  y = 0.299R + 0.587G + 0.114B
  U = -0.168736R - 0.331264G + 0.5B(+128)
  V = 0.5R - 0.418688G - 0.081312B(+128)
  */
const BYTE stdLuminanceQT[64] =
{
	16,  11,  10,  16,  24,  40,  51,  61,
	12,  12,  14,  19,  26,  58,  60,  55,
	14,  13,  16,  24,  40,  57,  69,  56,
	14,  17,  22,  29,  51,  87,  80,  62,
	18,  22,  37,  56,  68, 109, 103,  77,
	24,  35,  55,  64,  81, 104, 113,  92,
	49,  64,  78,  87, 103, 121, 120, 101,
	72,  92,  95,  98, 112, 100, 103,  99
};
const BYTE stdChrominanceQT[64] =
{
	17,  18,  24,  47,  99,  99,  99,  99,
	18,  21,  26,  66,  99,  99,  99,  99,
	24,  26,  56,  99,  99,  99,  99,  99,
	47,  66,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99,
	99,  99,  99,  99,  99,  99,  99,  99
};
const BYTE stdDcLuminanceLength[16] = { 0,1,5,1,1,1,1,1,1,0,0,0,0,0,0,0 };
const BYTE stdDcLuminanceValue[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

const BYTE stdDcChrominanceLength[16] = { 0,3,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };
const BYTE stdDcChrominanceValue[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

const BYTE stdAcLuminanceLength[16] = { 0,2,1,3,3,2,4,3,5,5,4,4,0,0,1,0x7d };
const BYTE stdAcLuminanceValue[162] =
{
	0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12,
	0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07,
	0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08,
	0x23, 0x42, 0xb1, 0xc1, 0x15, 0x52, 0xd1, 0xf0,
	0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16,
	0x17, 0x18, 0x19, 0x1a, 0x25, 0x26, 0x27, 0x28,
	0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98,
	0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
	0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6,
	0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3, 0xc4, 0xc5,
	0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4,
	0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xe1, 0xe2,
	0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea,
	0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};

const BYTE stdAcChrominanceLength[16] = { 0,2,1,2,4,4,3,4,7,5,4,4,0,1,2,0x77 };
const BYTE stdAcChrominanceValue[162] =
{
	0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21,
	0x31, 0x06, 0x12, 0x41, 0x51, 0x07, 0x61, 0x71,
	0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91,
	0xa1, 0xb1, 0xc1, 0x09, 0x23, 0x33, 0x52, 0xf0,
	0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34,
	0xe1, 0x25, 0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26,
	0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
	0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48,
	0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58,
	0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
	0x69, 0x6a, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78,
	0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87,
	0x88, 0x89, 0x8a, 0x92, 0x93, 0x94, 0x95, 0x96,
	0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
	0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4,
	0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
	0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2,
	0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda,
	0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9,
	0xea, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8,
	0xf9, 0xfa
};
const BYTE zigzag[64] = {
	0, 1, 5, 6,14,15,27,28,
	2, 4, 7,13,16,26,29,42,
	3, 8,12,17,25,30,41,43,
	9,11,18,24,31,40,44,53,
	10,19,23,32,39,45,52,54,
	20,22,33,38,46,51,55,60,
	21,34,37,47,50,56,59,61,
	35,36,48,49,57,58,62,63
};//zig-zag变换用

const float aa = 0.5f*cosf(PI / 4), bb = 0.5f*cosf(PI / 16), cc = 0.5f*cosf(PI / 8), dd = 0.5f*cosf(3 * PI / 16);
const float ee = 0.5f*cosf(5 * PI / 16), ff = 0.5f*cosf(3 * PI / 8), gg = 0.5f*cosf(7 * PI / 16);
const float A[64] = {
	aa, aa, aa, aa, aa, aa, aa, aa,
	bb, dd, ee, gg,-gg,-ee,-dd,-bb,
	cc, ff,-ff,-cc,-cc,-ff, ff, cc,
	dd,-gg,-bb,-ee, ee, bb, gg,-dd,
	aa,-aa,-aa, aa, aa,-aa,-aa, aa,
	ee,-bb, gg, dd,-dd,-gg, bb,-ee,
	ff,-cc, cc,-ff,-ff, cc,-cc, ff,
	gg,-ee, dd,-bb, bb,-dd, ee,-gg
};//dct用

const WORD binaryMask[16] = { 1,2,4,8,16,32,64,128,256,512,1024,2048,4096,8192,16384,32768 };//通过逻辑计算读取文件

class Huffmantree {
public:
	short lChild;
	short rChild;
	unsigned short lCode;
	unsigned short rCode;
	unsigned char lLength;
	unsigned char rLength;
	unsigned char lValue;
	unsigned char rValue;
	Huffmantree() {
		lChild = 0;	rChild = 0;
		lLength = 0; rLength = 0;
		lValue = 0; rValue = 0;
		lCode = 0; rCode = 0;
	}
	void mutate() {
		short Childtemp;
		unsigned char Valuetemp, Lengthtemp;
		Childtemp = lChild;	lChild = rChild; rChild = Childtemp;
		Lengthtemp = lLength; lLength = rLength; rLength = Lengthtemp;
		Valuetemp = lValue; lValue = rValue; rValue = Valuetemp;
	}
};
class Matrix {
public:
	template<typename T1, typename T2, typename T3> void mat_mul(T1*a, T2*b, T3*result, int aTotalRow, int bTotalColumn, int aTotalColumnBTotalRow) {
		int i, j, k;
		memset(result, 0, sizeof(T3)*aTotalRow*bTotalColumn);
		for (i = 0; i < aTotalRow; i++) {
			for (k = 0; k < aTotalColumnBTotalRow; k++) {
				for (j = 0; j < bTotalColumn; j++) {
					result[i * bTotalColumn + j] += a[i * aTotalColumnBTotalRow + k] * b[bTotalColumn * k + j];//行数+列数
				}
			}
		}
	}
	template<typename T> void mat_Transpose(T * a, T * result, int aTotalRow, int aTotalColumn) {
		for (int y = 0; y < aTotalColumn; y++) {
			for (int x = 0; x < aTotalRow; x++) {
				result[x * aTotalColumn + y] = a[x + aTotalColumn * y];
			}
		}
	}
	Matrix() {}
};

class ChaoticMap {
public:
	virtual double keySequence() = 0;
};
class LogisticMap : public ChaoticMap {
	double x;
	double mu;
public:
	double keySequence() {
		x = mu * x * (1 - x);
		if (x == 0.5)x += 0.001;
		if (x == 0.75)x += 0.001;
		return x;
	}
	void init(double x) {
		this->x = x;
		for (int i = 0; i < 200; i++) {
			keySequence();
		}
	}
	LogisticMap(double x, double mu = 4) {
		this->mu = mu;
		this->x = x;
		init(this->x);
	}
};
class HyperChaoticLvSystem : public ChaoticMap {
	/**
	 * where a, b, c are the constants of Lü system and d is a control parameter.
	 * When a = 36, b = 3, c = 20, and -0.35 < d ≤ 1.30,
	 * the system exhibits a hyperchaotic behavior.
	 */
	int i;
	double ele_val[4];
	double lv_x, lv_y, lv_z, lv_u;
	double lv_a, lv_b, lv_c, lv_d;
	void hylvRungeKutta() {	// 龙格库塔法
		double h = 0.005;   // 步长
		double K1, L1, M1, K2, L2, M2, K3, L3, M3, K4, L4, M4;
		double N1, N2, N3, N4;

		K1 = lv_a * (lv_y - lv_x) + lv_u;
		L1 = -lv_x * lv_z + lv_c * lv_y;
		M1 = lv_x * lv_y - lv_b * lv_z;
		N1 = lv_x * lv_z + lv_d * lv_u;

		K2 = lv_a * ((lv_y + h / 2 * L1) - (lv_x + h / 2 * K1))
			+ (lv_u + h / 2 * N1);
		L2 = -(lv_x + h / 2 * K1) * (lv_z + h / 2 * M1) 
			+ lv_c * (lv_y + h / 2 * L1);
		M2 = (lv_x + h / 2 * K1) * (lv_y + h / 2 * L1) 
			- lv_b * (lv_z + h / 2 * M1);
		N2 = (lv_x + h / 2 * K1) * (lv_z + h / 2 * M1)
			+ lv_d * (lv_u + h / 2 * N1);

		K3 = lv_a * ((lv_y + h / 2 * L2) - (lv_x + h / 2 * K2))
			+ (lv_u + h / 2 * N2);
		L3 = -(lv_x + h / 2 * K2) * (lv_z + h / 2 * M2)
			+ lv_c * (lv_y + h / 2 * L2);
		M3 = (lv_x + h / 2 * K2) * (lv_y + h / 2 * L2)
			- lv_b * (lv_z + h / 2 * M2);
		N3 = (lv_x + h / 2 * K2) * (lv_z + h / 2 * M2)
			+ lv_d * (lv_u + h / 2 * N2);

		K4 = lv_a * ((lv_y + h * L3) - (lv_x + h * K3))
			+ (lv_u + h * N3);
		L4 = -(lv_x + h * K3) * (lv_z + h * M3) + lv_c * (lv_y + h * L3);
		M4 = (lv_x + h * K3) * (lv_y + h * L3) - lv_b * (lv_z + h * M3);
		N4 = (lv_x + h * K3) * (lv_z + h * M3) + lv_d * (lv_u + h * N3);

		lv_x = lv_x + h / 6 * (K1 + 2 * K2 + 2 * K3 + K4);
		lv_y = lv_y + h / 6 * (L1 + 2 * L2 + 2 * L3 + L4);
		lv_z = lv_z + h / 6 * (M1 + 2 * M2 + 2 * M3 + M4);
		lv_u = lv_u + h / 6 * (N1 + 2 * N2 + 2 * N3 + N4);
	}
public:
	double keySequence() {
		if (i >= 4) {
			hylvRungeKutta();
			ele_val[0] = lv_x;
			ele_val[1] = lv_y;
			ele_val[2] = lv_z;
			ele_val[3] = lv_u;
			i = 0;
		}
		double data = ele_val[i++];
		data = abs(data);   // Lv 系统有负值
		while (data >= 1) data /= 10;
		return data;
	}
	void init(double lv_x = 1, double lv_y = 1, double lv_z = 1, double lv_u = 1, double lv_d = 1) {
		this->lv_x = lv_x;
		this->lv_y = lv_y;
		this->lv_z = lv_z;
		this->lv_u = lv_u;
		this->lv_d = lv_d;
		for (int i = 0; i < 200; i++) {
			hylvRungeKutta();
		}
		i = 10;
	}
	HyperChaoticLvSystem(double lv_x = 1, double lv_y = 1, double lv_z = 1, double lv_u = 1, double lv_d = 1) {
		this->lv_a = 36;
		this->lv_b = 3;
		this->lv_c = 20;
		init(lv_x, lv_y, lv_z, lv_u, lv_d);
	}
	~HyperChaoticLvSystem(){}
};
#ifdef ENCRYPT
struct PartData {//避免stl嵌套
	SWORD data[16];
	int size;//size 1~16
	PartData(){
		memset(data, 0, 32/*sizeof(SWORD)*16*/);
		size = 0;
	}
//	PartData& operator=(const PartData& src) {
//		this->size = src.size;
//		memcpy(this->data, src.data, 32/*sizeof(SWORD)*16*/);
//		return *this;
//	}
	bool isZRL() {
		if (size == 16 && data[15] == 0)
			return true;
		else return false;
	}
	void clear() {
		memset(data, 0, 32/*sizeof(SWORD) * 16*/);
		size = 0;
	}
};
class ObjectPosition {
public:
	std::string type;
	int x, y;
	int width, height;
	ObjectPosition(std::ifstream &file) {
		file >> type >> x >> y >> width >> height;
	}
};
class Object {
	int colBeginMCU, colEndMCU;
	int rowBeginMCU, rowEndMCU;
	int originalColMCU;
public:
	int numMCU;
	std::string type;
	SWORD (*yData)[64], (*cbData)[64], (*crData)[64];
	void writeBack(SWORD(*oriYData)[64], SWORD(*oriCbData)[64], SWORD(*oriCrData)[64]) {
		SWORD(*yPointer)[64] = this->yData;
		SWORD(*cbPointer)[64] = this->cbData;
		SWORD(*crPointer)[64] = this->crData;
		int sizeTemp = sizeof(SWORD)*(colEndMCU - colBeginMCU + 1) * 64;
		for (int i = this->originalColMCU*rowBeginMCU + colBeginMCU;
			i <= this->originalColMCU*rowEndMCU + colBeginMCU;
			i += this->originalColMCU)
		{
			memcpy(&oriYData[i], yPointer, sizeTemp);
			memcpy(&oriCbData[i], cbPointer, sizeTemp);
			memcpy(&oriCrData[i], crPointer, sizeTemp);
			yPointer += (colEndMCU - colBeginMCU + 1);
			cbPointer += (colEndMCU - colBeginMCU + 1);
			crPointer += (colEndMCU - colBeginMCU + 1);
		}
	}
	Object(ObjectPosition position, int originalColMCU, SWORD(*oriYData)[64],SWORD(*oriCbData)[64],SWORD(*oriCrData)[64]) {
		rowBeginMCU = position.y / 8;
		rowEndMCU = (position.y - 1 + position.height) / 8;
		colBeginMCU = position.x / 8;
		colEndMCU = (position.x - 1 + position.width) / 8;
		int sizeTemp = (rowEndMCU - rowBeginMCU + 1)*(colEndMCU - colBeginMCU + 1);
		numMCU = sizeTemp;
		this->yData = new SWORD[sizeTemp][64];
		this->cbData = new SWORD[sizeTemp][64];
		this->crData = new SWORD[sizeTemp][64];
		this->originalColMCU = originalColMCU;
		SWORD(*yPointer)[64] = this->yData;
		SWORD(*cbPointer)[64] = this->cbData;
		SWORD(*crPointer)[64] = this->crData;
		sizeTemp = sizeof(SWORD)*(colEndMCU - colBeginMCU + 1) * 64;
		for (int i = this->originalColMCU*rowBeginMCU + colBeginMCU;
			i <= this->originalColMCU*rowEndMCU + colBeginMCU;
			i += this->originalColMCU)
		{		
			memcpy(yPointer, &oriYData[i], sizeTemp);
			memcpy(cbPointer, &oriCbData[i], sizeTemp);
			memcpy(crPointer, &oriCrData[i], sizeTemp);
			yPointer += (colEndMCU - colBeginMCU + 1);
			cbPointer += (colEndMCU - colBeginMCU + 1);
			crPointer += (colEndMCU - colBeginMCU + 1);
		}
	}
	Object(const Object&object) {
		this->type = object.type;
		this->colBeginMCU = object.colBeginMCU; this->colEndMCU = object.colEndMCU;
		this->rowBeginMCU = object.rowBeginMCU; this->rowEndMCU = object.rowEndMCU;
		this->originalColMCU = object.originalColMCU;
		this->numMCU = object.numMCU;
		int sizeTemp = this->numMCU;
		this->yData = new SWORD[sizeTemp][64];
		this->cbData = new SWORD[sizeTemp][64];
		this->crData = new SWORD[sizeTemp][64];
		sizeTemp = sizeof(SWORD)*(rowEndMCU - rowBeginMCU + 1)*(colEndMCU - colBeginMCU + 1) * 64;
		memcpy(this->yData, object.yData, sizeTemp);
		memcpy(this->cbData, object.cbData, sizeTemp);
		memcpy(this->crData, object.crData, sizeTemp);
	}
	~Object() {
		delete[] yData;
		delete[] cbData;
		delete[] crData;
	}
};

#endif

#endif

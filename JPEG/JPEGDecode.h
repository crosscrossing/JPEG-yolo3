#ifndef JPEGDECODE_H
#define JPEGDECODE_H
#include "JPEGGlobal.h"
class JPEGDecode : protected Matrix{
protected:
	struct DEHUFF {
		BYTE length;
		BYTE value;
		DEHUFF() { length = 0; value = 0; }
	};//用code作为索引，遇到size=0跳过
	struct DEHUFFEncrypted {
		BYTE length;
		WORD code;
		DEHUFFEncrypted() { length = 0; code = 0; }
	};//循环检索，效率较低，哈夫曼表加密用
	unsigned char sequentialOrProgressive;
	std::fstream file;
	std::string fileName;
	APP0 app0;
	DQT dqt[2];
	BYTE*yqt;
	BYTE*cqt;
	SOF sof;
	SOFColour*sofColor;
	DRI dri;
	SOS sos;
	SOSColour*sosColor;
	BYTE ss, se, sA;
	BYTE *r, *g, *b, *y;//图像数据
	int width, height;

	short dqtcount, dhtcount;//qt ht 有多少段
	DHT dht[4];
	BYTE *yDcHuffLength, *cDcHuffLength, *yAcHuffLength, *cAcHuffLength;
	WORD *yDcHuffCode, *cDcHuffCode, *yAcHuffCode, *cAcHuffCode;
	BYTE *yDcHuffVal, *cDcHuffVal, *yAcHuffVal, *cAcHuffVal;
	DEHUFF *yDcDehuff, *yAcDehuff, *cDcDehuff, *cAcDehuff;

	DHT dhtAc2, dhtAc3;
	BYTE *ac2HuffLength, *ac3HuffLength;
	WORD *ac2HuffCode, *ac3HuffCode;
	BYTE *ac2HuffValue, *ac3HuffValue;
	bool dht0Generated, dht1Generated, dht2Generated, 
		 dht3Generated, ac2Generated, ac3Generated;
	//	SWORD (*Y_P)[64], (*Cb_P)[64], (*Cr_P)[64];//Progressive


	int preRead();//预读取 测试用
	void show(std::fstream&file);//临时显示文件接下来的内容
	void showFileQtHt();
	//显示文件QT和HT表

	//解码主函数
	int decodeMain(std::string fileName);
	virtual int decodeSequentialMain();
	int decodeProgressiveMain();
	virtual int decodeDataStream();

	int readSegments();//读数据前各个段
	int readHuffmanTable();//读取哈夫曼表
	int readQuantizationTable();//读取量化表
	int otherSegments();//其他段的处理
	int readDC(DEHUFF * deHuff, SWORD & diff, SWORD & pred, SWORD * result);
	int readAC(DEHUFF * deHuff, SWORD & diff, SWORD * result);
	int readDCProgressiveFirstscan(DEHUFF*DEHUFF, SWORD&diff, SWORD&pred, const BYTE Ah, const BYTE Al, SWORD*result);
	int readACProgressiveFirstscan(DEHUFF * DEHUFF, const BYTE ss, const BYTE se, const BYTE Al, int & EOBRUN, SWORD * result);
	int readDCProgressiveSubsequentScans(const BYTE Ah, const BYTE Al, SWORD(*y)[64], SWORD(*Cb)[64], SWORD(*Cr)[64], int MCU_sum);
	int readDCProgressiveSubsequentScans(const BYTE Ah, const BYTE Al, SWORD(*y)[64], int MCU_sum);
	int readACProgressiveSubsequentScans(DEHUFF * DEHUFF, const BYTE ss, const BYTE se, const BYTE Ah, const BYTE Al, SWORD(*result)[64], int MCU_sum);

	SWORD yDiff, cbDiff, crDiff;
	SWORD yPred, cbPred, crPred;
	bool restartInterval;
	int restartCount;//DRI
	BYTE byteNew; //将要读取的byte
	SBYTE bytePos;
	//读取byte当前第几个bit 应该<=7且>=0
	BYTE readOneBit();//读一个bit 返回1和0

	int generateHuffmanTable(DHT dht, BYTE*huffLength, WORD*huffCode, BYTE*huffValue, DEHUFF*DEHUFF);//生成哈夫曼表
	void generateHuffmanTable();
	void convertToRgb(BYTE * rMCU, BYTE * gMCU, BYTE * bMCU, float * yMCU, float * cbMCU, float * crMCU);
	void idct(SWORD * idct, float * mcu);
	void dequantization(SWORD * deQT, SWORD * result, BYTE * QT);
	void reverseZZ(SWORD * ZZ, SWORD * result);

public:
	int getWidth() { return width; }
	int getHeight() { return height; }
	const BYTE* getR() { return r; }
	const BYTE* getG() { return g; }
	const BYTE* getB() { return b; }
	const BYTE* getY() { return y; }
	BYTE getNumberOfColor() { return sof.numberOfColor; }
	JPEGDecode(){}
	JPEGDecode(std::string fileName) {
		decodeMain(fileName);
	}
	virtual ~JPEGDecode() {
		//通用部分内存清理
		delete[] yDcHuffLength; delete[] yDcHuffCode; delete[] yDcHuffVal;
		delete[] yAcHuffLength; delete[] yAcHuffCode; delete[] yAcHuffVal;
		delete[] yqt;
		delete[] sofColor;
		delete[] yDcDehuff; delete[] yAcDehuff;
		if (sof.numberOfColor == 3) {//RGB
			delete[] cDcHuffLength; delete[] cDcHuffCode; delete[] cDcHuffVal;
			delete[] cAcHuffLength; delete[] cAcHuffCode; delete[] cAcHuffVal;
			delete[] cqt;
			delete[] cDcDehuff; delete[] cAcDehuff;
			delete[] r; delete[] g; delete[] b;
		}
		else {
			delete[] y;
			if (sequentialOrProgressive == 0xc2) {
				delete[] cAcHuffLength; delete[] cAcHuffCode; delete[] cAcHuffVal;//ac有多张表，实际使用了该表，只是表的变量名字没改
			}
		}
		//清理连续JPEG模块内存
		if (sequentialOrProgressive == 0xc0) {
			delete[] sosColor;
		}
		//清理累进JPEG模块内存已在对应函数中执行
	}
};
class JPEGDecodeDecrypt : public JPEGDecode {
protected:
	//override
	virtual int decodeSequentialMain();
	virtual int decodeDataStream();
#ifdef ENCRYPT_HUFF
	int decodeCipherStream();
	DEHUFFEncrypted *dehuffEncrypt[4];
	int readDcCipher(DEHUFFEncrypted* dehuffEncrypt, SWORD & diff, SWORD & pred, SWORD * result);
	int readAcCipher(DEHUFFEncrypted* dehuffEncrypt, SWORD & diff, SWORD * result);
	int generateEncryptedHuffmanTable(DHT dht, BYTE * huffLength, WORD * huffCode, BYTE * huffValue, DEHUFFEncrypted * dehuffEncrypt);
	void generateEncryptedHuffmanTable();
#endif
#ifdef ENCRYPT	
	void decrypt(SWORD(*YMcu)[64], SWORD(*CbMcu)[64], SWORD(*CrMcu)[64], int numMCU);
	//void decryptComputeDiff(SWORD(*mcu)[64], int numMCU);
	//void decryptReComputeDiff(SWORD(*mcu)[64], int numMCU);
	void decryptBlockScrambling(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation);
	void decryptDCScramble(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap &substitution);
	void decryptAC(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap &substitution);
#endif
public:
	JPEGDecodeDecrypt(std::string fileName){
		decodeMain(fileName);
	}
};
#endif

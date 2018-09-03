#ifndef JPEGENCODE_H
#define JPEGENCODE_H
#include "JPEGGlobal.h"

class JPEGEncode : protected Matrix {
protected:
//	char Encrypt_HUFF_CMD;
	std::fstream file;
	std::string fileName;
	struct ENHUFF {
		BYTE length;
		WORD code;
		ENHUFF() { length = 0; code = 0; }
	};//��value��Ϊ����������size=0����
	APP0 app0;
	DQT dqt[2];
	const BYTE*yQtBasicTable;
	const BYTE*cQtBasicTable;
	BYTE*yqt;
	BYTE*cqt;
	SOF sof;
	SOFColour sofColor[3];
	DHT dht[4];
	BYTE*yDcHuffVal, *cDcHuffVal, *yAcHuffVal, *cAcHuffVal;//temp
	BYTE*yDcHuffLength, *cDcHuffLength, *yAcHuffLength, *cAcHuffLength;//temp
	WORD*yDcHuffCode, *cDcHuffCode, *yAcHuffCode, *cAcHuffCode;//temp
	ENHUFF *yDcEnHuff, *yAcEnHuff, *cDcEnHuff, *cAcEnHuff;
	SOS sos;
	SOSColour sosColor[3];
	DRI dri;
	int numMCU;//mcu����
	int width, height;
	const BYTE *r, *g, *b;//ͼ������

	void encodeMain(std::string fileName, int width, int height, const BYTE *r, const BYTE *g, const BYTE *b);
	virtual void encode(int scalefactor);
	int writeFileHeader();
	int generateHuffmanTable(DHT dht, BYTE * huffLength, WORD * huffCode, BYTE * huffValue, ENHUFF * enhuff);
	void convertToYCbCr(BYTE * rMCU, BYTE * gMCU, BYTE * bMCU, float * yMCU, float * cbMCU, float * crMCU);
	void dct(float*mcu, float*dct);
	void zz(float * ZZ, float * result);
	void set_quant_table(const BYTE *basicTable, int scalefactor, BYTE *newTable);
	void quantization(float * QT, SWORD * result, BYTE * table);
	int writeDC(ENHUFF*ENHUFF, SWORD*afterQT, SWORD&pred);
	int writeAC(ENHUFF*ENHUFF, SWORD*afterQT);

	BYTE byteNew; //��Ҫ��ȡ��byte
	SBYTE bytePos; //��ȡbyte��ǰ�ڼ���bit Ӧ��<=7��>=0
	void writeOneBit(WORD w);//дһ��bit
	void showFileQtHt();

public:
	JPEGEncode(){}
	JPEGEncode(std::string fileName,int width,int height, const BYTE *r, const BYTE *g, const BYTE *b) { //ʹ��std��������Ĺ��캯��
		encodeMain(fileName, width, height, r, g, b);//ִ��
	}
	virtual ~JPEGEncode() {
		delete[] yqt;
		delete[] cqt;
	}
};

class JPEGEncodeEncrypt : public JPEGEncode {
protected:
	//override
	virtual void encode(int scalefactor);
#ifdef ENCRYPT	
	void encrypt(SWORD(*YMcu)[64], SWORD(*CbMcu)[64], SWORD(*CrMcu)[64], int numMCU);
	//void encryptComputeDiff(SWORD(*mcu)[64], int numMCU);
	//void encryptReComputeDiff(SWORD(*mcu)[64], int numMCU);
	void encryptBlockScrambling(SWORD(*mcu)[64], int numMCU, ChaoticMap &chaoticMap);
	void encryptDCScramble(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap&substitution);
	void encryptAC(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap &substitution);
#endif
#ifdef ENCRYPTHUFF
	void generateEncryptedHuffmanTable();
	int generateEncryptedHuffmanTable(DHT dht, BYTE * huffLength, WORD * huffCode, BYTE * huffValue, ENHUFF * ENHUFF);
#endif

public:
	JPEGEncodeEncrypt(std::string fileName, int width, int height, const BYTE *r, const BYTE *g, const BYTE *b) : JPEGEncode() {
		encodeMain(fileName, width, height, r, g, b);//ִ��
	}
};

#endif

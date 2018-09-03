#ifndef BMPHEADER_H
#define BMPHEADER_H
#include<iostream>
#include<fstream>
#include<exception>
#include<cmath>
#include<string>
typedef unsigned char BYTE;	//1byte
typedef signed char SBYTE;	//1byte signed
typedef unsigned short WORD;//2byte
typedef short SWORD;        //2byte signed
typedef unsigned int DWORD;//4byte
typedef int SDWORD;		//4byte signed

#pragma pack(push)//�������״̬
#pragma pack(2)//�趨Ϊ2�ֽڶ���
typedef struct tagBITMAPFILEHEADER {
	WORD bfType;//�ļ����ͣ�������0x4d42�����ַ���BM��  
	DWORD bfSize;//�ļ���С  
	WORD bfReserved1;//������  
	WORD bfReserved2;//������  
	DWORD bfOffBits;//���ļ�ͷ��ʵ��λͼ���ݵ�ƫ���ֽ���  
	void showBmpHead() {
		std::cout << "λͼ�ļ�ͷ:" << std::endl;
		std::cout << "�ļ���С:" << bfSize << std::endl;
		std::cout << "������_1:" << bfReserved1 << std::endl;
		std::cout << "������_2:" << bfReserved2 << std::endl;
		std::cout << "ʵ��λͼ���ݵ�ƫ���ֽ���:" << bfOffBits << std::endl << std::endl;
	}
}BITMAPFILEHEADER;//λͼ�ļ�ͷ����; 
typedef struct tagBITMAPINFOHEADER {
	DWORD biSize;//��Ϣͷ��С  
	DWORD biWidth;//ͼ����  
	DWORD biHeight;//ͼ��߶�  
	WORD biPlanes;//λƽ����������Ϊ1  
	WORD biBitCount;//ÿ����λ��  
	DWORD  biCompression; //ѹ������  
	DWORD  biSizeImage; //ѹ��ͼ���С�ֽ���  
	DWORD  biXPelsPerMeter; //ˮƽ�ֱ���  
	DWORD  biYPelsPerMeter; //��ֱ�ֱ���  
	DWORD  biClrUsed; //λͼʵ���õ���ɫ����  
	DWORD  biClrImportant; //��λͼ����Ҫ��ɫ����  
	void showBmpInforHead() {
		std::cout << "λͼ��Ϣͷ:" << std::endl;
		std::cout << "�ṹ��ĳ���:" << biSize << std::endl;
		std::cout << "λͼ��:" << biWidth << std::endl;
		std::cout << "λͼ��:" << biHeight << std::endl;
		std::cout << "ƽ����:" << biPlanes << std::endl;
		std::cout << "������ɫλ��:" << biBitCount << std::endl;
		std::cout << "ѹ����ʽ:" << biCompression << std::endl;
		std::cout << "ʵ��λͼ����ռ�õ��ֽ���:" << biSizeImage << std::endl;
		std::cout << "X����ֱ���:" << biXPelsPerMeter << std::endl;
		std::cout << "y����ֱ���:" << biYPelsPerMeter << std::endl;
		std::cout << "ʹ�õ���ɫ��:" << biClrUsed << std::endl;
		std::cout << "��Ҫ��ɫ��:" << biClrImportant << std::endl << std::endl;
	}
}BITMAPINFOHEADER; //λͼ��Ϣͷ����  
typedef struct tagRGBQUAD {
	BYTE rgbBlue; //����ɫ����ɫ����  
	BYTE rgbGreen; //����ɫ����ɫ����  
	BYTE rgbRed; //����ɫ�ĺ�ɫ����  
	BYTE rgbReserved; //����ֵ  
}RGBQUAD;//��ɫ�嶨��  
struct COLOURIMAGEDATA {
public:
	BYTE blue;
	BYTE green;
	BYTE red;
	COLOURIMAGEDATA& operator= (COLOURIMAGEDATA& a) {
		this->blue = a.blue; this->green = a.green; this->red = a.red;
		return *this;
	}
}; //������Ϣ  
#pragma pack(pop)//�ָ�����״̬

class BMPDecode {
protected:
	int width, height;
	int padding;//padding for 4 byte alignment
	std::string fileName;
	BITMAPFILEHEADER head;
	BITMAPINFOHEADER info;
	RGBQUAD*quad;
	BYTE*y;
	BYTE*r, *g, *b;
	int decode() {
		std::ifstream rfile(fileName.c_str(), std::ios::in | std::ios::binary);
		if (!rfile) {
			throw"bmp open error";
		}
		rfile.read((char*)&head, sizeof(BITMAPFILEHEADER));
		rfile.read((char*)&info, sizeof(BITMAPINFOHEADER));
		if (head.bfType != 0x4d42) {
			throw "not windows bmp file";
		}
		//	head.showBmpHead();
		//	info.showBmpInforHead();
		width = info.biWidth; height = info.biHeight;
		
		switch (info.biBitCount) {
		case 24: {
			padding = 4 - (width * 3) % 4;
			if (padding == 4)padding = 0;
			r = new BYTE[2*width*height];
			g = new BYTE[2*width*height];
			b = new BYTE[2*width*height];
			COLOURIMAGEDATA temp;
			int pos;
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					rfile.read((char*)&temp, 3);
					pos = (height - j - 1)*width + i;
					r[pos] = temp.red;
					g[pos] = temp.green;
					b[pos] = temp.blue;
				}
				rfile.seekg(padding, std::ios::cur);
			}
			break;
		}
		case 8: {
			padding = 4 - width % 4;
			if (padding == 4)padding = 0;
			quad = new RGBQUAD[256];
			rfile.read((char*)quad, 1024);//256 * sizeof(RGBQUAD)
			BYTE temp;
			y = new BYTE[width*height];
			int pos;
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					rfile.read((char*)&temp, 1);
					pos = (height - j - 1)*width + i;
					y[pos] = temp;
				}
				rfile.seekg(padding, std::ios::cur);
			}
			break;
		}
		case 4: {
			int _width = width / 2;
			if (width % 8 != 0)_width++;
			padding = 4 - _width % 4;
			if (padding == 4)padding = 0;
			quad = new RGBQUAD[16];
			rfile.read((char*)quad, 64);//16 * sizeof(RGBQUAD);
			BYTE temp;
			y = new BYTE[width*height];
			int pos;
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < _width; i++) {
					rfile.read((char*)&temp, 1);
					pos = (height - j - 1)*_width + i * 2;
					y[pos] = temp / 16;
					y[pos] *= 16;
					pos++;
					if (i == _width - 1 && width % 2 != 0)break;
					y[pos] = temp % 16;
					y[pos] *= 16;
				}
				rfile.seekg(padding, std::ios::cur);
			}
			break;
		}
		case 1: {
			int _width = width / 8;
			if (width % 8 != 0)_width++;
			padding = 4 - _width % 4;
			if (padding == 4)padding = 0;
			quad = new RGBQUAD[2];
			rfile.read((char*)quad, 8);// 2 * sizeof(RGBQUAD)
			BYTE temp;
			y = new BYTE[width*height];
			int pos;
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < _width; i++) {
					rfile.read((char*)&temp, 1);
					pos = (height - j - 1)*_width + i * 8;
					int width_temp = width;
					int maskArray[] = { 128,64,32,16,8,4,2,1 };
					for (int k = 7; k >= 0 && width_temp>0; k--) {
						y[pos] = temp & maskArray[k];
						y[pos] *= 255;
						pos++; 
						width_temp--;
					}
				}
				rfile.seekg(padding, std::ios::cur);
			}
			break;
		}
		default:{
			throw "info.biBitCount unsupported";

		}
		}
		rfile.close();
		return 0;
	}
	/*
	bool resize(unsigned char * dstBuf, unsigned char * srcBuf, int srcW, int srcH, int dstW, int dstH)
	{
		if (!dstBuf || !srcBuf)
			return false;
		if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0)
			return false;

		int nSrcWidthStep = srcW;//Դͼ��ÿ����ռ�ֽ���
		int nDstWidthStep = dstW;//Ŀ��ͼ��ÿ����ռ�ֽ���

		for (int i = 0; i < dstH; ++i)
		{
			//ԭͼ�ж�Ӧ������
			double y = (static_cast<double>(i) + 0.5)*static_cast<double>(srcH) / static_cast<double>(dstH) - 0.5;
			//ָ��Ŀ��ͼ���������
			unsigned char * pDstLine = dstBuf + i * nDstWidthStep;
			for (int j = 0; j <dstW; ++j)
			{
				double x = (static_cast<double>(j) + 0.5)*static_cast<double>(srcW) / static_cast<double>(dstW) - 0.5;//ԭͼ�ж�Ӧ������
																													  //u��vΪx,y��С������,���Ŵ�2048���Թ�ܸ�������
				int u = static_cast<int> ((y - static_cast<int>(y)) * 2048);
				int v = static_cast<int> ((x - static_cast<int>(x)) * 2048);

				//ix,iy�ֱ�洢x,y����������
				int ix = (int)x;
				int iy = (int)y;
				pDstLine[j]
					= (srcBuf[iy*nSrcWidthStep + ix] * (2048 - u)*(2048 - v)
						+ srcBuf[iy*nSrcWidthStep + (ix + 1)] * (2048 - u)*v
						+ srcBuf[(iy + 1)*nSrcWidthStep + ix] * u*(2048 - v)
						+ srcBuf[(iy + 1)*nSrcWidthStep + (ix + 1)] * u*v) >> 22;
			}
		}
		return true;
	}
	void resize(int reWidth, int reHeight) {
		if (info.biBitCount == 24) {
			BYTE *reR = new BYTE[reWidth*reHeight];
			BYTE *reG = new BYTE[reWidth*reHeight];
			BYTE *reB = new BYTE[reWidth*reHeight];
			resize(reR, r, width, height, reWidth, reHeight);
			resize(reG, g, width, height, reWidth, reHeight);
			resize(reB, b, width, height, reWidth, reHeight);
			delete[] r;
			delete[] g;
			delete[] b;
			r = new BYTE[reWidth*reHeight];
			g = new BYTE[reWidth*reHeight];
			b = new BYTE[reWidth*reHeight];
			width = reWidth;
			height = reHeight;
			memcpy(r, reR, width*height);
			memcpy(g, reG, width*height);
			memcpy(b, reB, width*height);
			delete[] reR;
			delete[] reG;
			delete[] reB;
		}
		else {
			BYTE *reY = new BYTE[reWidth*reHeight];
			resize(reY, y, width, height, reWidth, reHeight);
			delete[] y;
			y = new BYTE[reWidth*reHeight];
			width = reWidth;
			height = reHeight;
			memcpy(y, reY, width*height);
			delete[] reY;
		}
	}
*/
public:
	int getWidth() { return width; }
	int getHeight() { return height; }
	const BYTE* getR() { return r; }
	const BYTE* getG() { return g; }
	const BYTE* getB() { return b; }
	const BYTE* getY() { return y; }
	BYTE getNumberOfColor() { return info.biBitCount >= 24 ? 3 : 1; }
	BMPDecode(std::string fileName) {
		this->fileName = fileName;
		decode();//ִ��
	}
	~BMPDecode() {
		if (info.biBitCount==24) {
			delete[] r;
			delete[] g;
			delete[] b;
		}
		else {
			delete[] y;
			delete[] quad;
		}
	}
};
class BMPEncode {
protected:
	int width,height;
	int padding;//ÿ��4λ����
	int filePadding;//�ļ�4λ����
	std::string fileName;
	BITMAPFILEHEADER head;
	BITMAPINFOHEADER info;
	RGBQUAD*quad;
	COLOURIMAGEDATA*colorImageData;
	BYTE*grayImageData;
	int encode() {
		std::fstream wfile(fileName.c_str(), std::ios::out | std::ios::binary);
		if (!wfile)return -1;
		wfile.write((char*)&head, sizeof(BITMAPFILEHEADER));
		wfile.write((char*)&info, sizeof(BITMAPINFOHEADER));
		if (info.biBitCount != 24) {
			int size;
			switch (info.biBitCount) {
			case 8:
				size = 256 * sizeof(RGBQUAD);
				break;
			case 4:
				size = 16 * sizeof(RGBQUAD);
				break;
			case 1:
				size = 2 * sizeof(RGBQUAD);
				break;
			}
			wfile.write((char*)quad, size);
		}
		switch (info.biBitCount) {
		case 24:
			for (int j = 0; j < height; j++) {
				int offset = j * width;
				COLOURIMAGEDATA *Data_pointer;
				Data_pointer = colorImageData + offset;
				wfile.write((char*)Data_pointer, sizeof(COLOURIMAGEDATA)*width);
				for (int i = padding; i > 0; i--)
					wfile.put(0);
			}
			break;
		case 8:
			for (int j = 0; j < height; j++) {
				int offset = j * width;
				BYTE *Data_pointer;
				Data_pointer = grayImageData + offset;
				wfile.write((char*)Data_pointer, width);
				for (int i = padding; i > 0; i--)
					wfile.put(0);
			}
			break;
		case 4:
			wfile.write((char*)grayImageData, width*height / 2);
			break;
		case 1:
			wfile.write((char*)grayImageData, width*height / 8);
			break;
		}
		for (int i = 0; i < filePadding; i++) {
			wfile.put(0);
		}
		wfile.close();
		return 0;
	}
public:
	BMPEncode(std::string _file_name, int _width, int _height, BYTE biBitCount, const BYTE*r, const BYTE*g, const BYTE*b) {
		width = _width; height = _height;
		fileName = _file_name;

		head.bfType = 0x4d42;
		switch (biBitCount) {
		case 24:
			padding = 4 - (width * 3) % 4;
			if (padding == 4)padding = 0;
			filePadding = 4 - ((width + padding)* height * 3) % 4;
			if (filePadding == 4)filePadding = 0;
			head.bfSize = 54 + ((sizeof(COLOURIMAGEDATA))*width + padding)*height + filePadding;
			break;
		default:
			throw"info.biBitCount unsupported";
		}
		head.bfReserved2 = head.bfReserved1 = 0;
		head.bfOffBits = 54;
		info.biBitCount = biBitCount;
		info.biClrImportant = 0;
		info.biClrUsed = 0;
		info.biCompression = 0;
		info.biHeight = height;
		info.biWidth = width;
		info.biPlanes = 1;
		info.biSize = 40;
		info.biSizeImage = head.bfSize - 54;
		info.biXPelsPerMeter = 0;
		info.biYPelsPerMeter = 0;

		switch (info.biBitCount) {//switchΪ��δ����32bit
		case 24: {
			colorImageData = new COLOURIMAGEDATA[width*height];
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					int bmppos = (height - j - 1)*width + i;
					int pos = i + j * width;
					colorImageData[bmppos].red = r[pos];
					colorImageData[bmppos].green = g[pos];
					colorImageData[bmppos].blue = b[pos];
				}
			}
			break;
		}
		}
		//Execute
		encode();
	}
	BMPEncode(std::string _file_name, int _width, int _height, BYTE biBitCount, const BYTE*y) {
		width = _width; height = _height;
		fileName = _file_name;
		int Quad_size = 0;

		switch (biBitCount) {
		case 8: {
			padding = 4 - width % 4;
			if (padding == 4)padding = 0;
			filePadding = 4 - ((width + padding)* height) % 4;
			if (filePadding == 4)filePadding = 0;
			Quad_size = 256 * sizeof(RGBQUAD);
			quad = new RGBQUAD[256];
			for (int i = 0; i < 256; i++) {
				quad[i].rgbBlue = i;
				quad[i].rgbGreen = i;
				quad[i].rgbRed = i;
				quad[i].rgbReserved = 0;
			}
			break;
		}
		case 4: {
			int _width = width / 2;
			if (width % 8 != 0)_width++;
			padding = 4 - _width % 4;
			if (padding == 4)padding = 0;
			filePadding = 4 - ((_width + padding)* height) % 4;
			if (filePadding == 4)filePadding = 0;
			Quad_size = 16 * sizeof(RGBQUAD);
			quad = new RGBQUAD[16];
			for (int i = 0; i < 16; i++) {
				quad[i].rgbBlue = i * 17;
				quad[i].rgbGreen = i * 17;
				quad[i].rgbRed = i * 17;
				quad[i].rgbReserved = 0;
			}
			break;
		}
		case 1: {
			int _width = width / 8;
			if (width % 8 != 0)_width++;
			padding = 4 - _width % 4;
			if (padding == 4)padding = 0;
			filePadding = 4 - ((_width + padding)* height) % 4 + 2;
			if (filePadding == 4)filePadding = 0;
			Quad_size = 2 * sizeof(RGBQUAD);
			quad = new RGBQUAD[2];
			quad[0].rgbBlue = quad[0].rgbGreen = quad[0].rgbRed = 0;
			quad[1].rgbBlue = quad[1].rgbGreen = quad[1].rgbRed = 255;
			quad[0].rgbReserved = quad[1].rgbReserved = 0;
			break;
		}
		default: {
			throw"info.biBitCount unsupported";
		}
		}

		head.bfType = 0x4d42;
		switch (biBitCount) {
		case 8:
			head.bfSize = 54 + Quad_size + (width + padding)*height + filePadding;
			break;
		case 4:
			head.bfSize = 54 + Quad_size + width*height / 2 + padding*height + filePadding;
			break;
		case 1:
			head.bfSize = 54 + Quad_size + width*height / 8 + padding*height + filePadding;
			break;
		}
		head.bfReserved2 = head.bfReserved1 = 0;
		head.bfOffBits = 54 + Quad_size;
		info.biBitCount = biBitCount;
		info.biClrImportant = 0;
		info.biClrUsed = 0;
		info.biCompression = 0;
		info.biHeight = height;
		info.biWidth = width;
		info.biPlanes = 1;
		info.biSize = 40;
		info.biSizeImage = head.bfSize - 54 - Quad_size;
		info.biXPelsPerMeter = 0;
		info.biYPelsPerMeter = 0;

		switch (info.biBitCount) {
		case 8: {
			grayImageData = new BYTE[width*height];
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					int bmppos = (height - j - 1)*width + i;
					int pos = i + j * width;
					grayImageData[bmppos] = y[pos];
				}
			}
			break;
		}
		case 4: {
			grayImageData = new BYTE[width*height / 2];
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					int bmppos = (height - j - 1)*width + i;
					int pos = i + j * width;
					grayImageData[bmppos] = 0;
					if (width % 2 == 0) {
						bmppos = (height - j - 1)*width + i / 2;
						grayImageData[bmppos] += (BYTE)round(y[pos] / 16.0) * 16;
					}
					else {
						bmppos = (height - j - 1)*(int)floor((width + 1) / 2.0) + i / 2;
						grayImageData[bmppos] += (BYTE)round(y[pos] / 16.0);
					}
				}
			}
			break;
		}
		case 1: {
			grayImageData = new BYTE[width*height / 8];
			BYTE byteNew = 0;
			SBYTE bytePos = 7;
			int Data_pos = 0;
			int maskArray[] = { 128,64,32,16,8,4,2,1 };
			for (int j = 0; j < height; j++) {
				for (int i = 0; i < width; i++) {
					//int bmppos = (height - j - 1)*width + i;
					int pos = i + j * width;
					if (y[pos] > 127)
						byteNew = byteNew | maskArray[bytePos];
					bytePos--;
					if (bytePos < 0) {
						grayImageData[Data_pos] = byteNew;
						Data_pos++;
						bytePos = 7;
						byteNew = 0;
					}
				}
				if (bytePos != 7) {
					grayImageData[Data_pos] = byteNew;
					Data_pos++;
					bytePos = 7;
					byteNew = 0;
				}
			}
			break;
		}
		}
		encode();//Execute
	}

	~BMPEncode() {
		if (info.biBitCount == 24){
			delete[] colorImageData;
		}
		else{
			delete[] quad;delete[] grayImageData;
		}
	}
};
#endif
/*	*/

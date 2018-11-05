#include"JPEGDecode.h"
using namespace std;
int JPEGDecode::decodeMain(string fileName) {
//	preRead();//预读取 测试用
	restartInterval = false;
	yqt = NULL;
	cqt = NULL;
	sofColor = NULL;
	sosColor = NULL;
	dqtcount = dhtcount = 0;
	byteNew = 0;
	bytePos = 7;
	yDcDehuff = new DEHUFF[65536];
	yAcDehuff = new DEHUFF[65536];
	cDcDehuff = new DEHUFF[65536];
	cAcDehuff = new DEHUFF[65536];
	yDiff = cbDiff = crDiff = 0;
	yPred = cbPred = crPred = 0;
	dht0Generated = dht1Generated = dht2Generated = dht3Generated = ac2Generated = ac3Generated = false;//progressive
	this->fileName = fileName;
	file.open(this->fileName.c_str(), ios::in | ios::binary);
	if (!file) {
		throw "JPG文件打开失败";
	}
	readSegments();
	switch (sequentialOrProgressive) {
	case 0xc0:
		decodeSequentialMain();
		//	show(file);//检查是否完成解码
		break;
	case 0xc2:
		decodeProgressiveMain(); break;
	}
	file.close();

	return 0;
}
int JPEGDecode::decodeSequentialMain() {
	generateHuffmanTable();
	//	showFileQtHt();//仅支持非加密//临时显示文件用
	decodeDataStream();
	return 0;
}
int JPEGDecodeDecrypt::decodeSequentialMain() {
#ifndef ENCRYPT_HUFF
	generateHuffmanTable();
	//	showFileQtHt();//仅支持非加密//临时显示文件用
	decodeDataStream();
#else
	mu = 4.0;
	x = 0.2865612612905;//seed
	for (int i = 0; i < 200; i++) logistic();
	for (int i = 0; i < 4; i++) {
		dehuffEncrypt[i] = new DEHUFFEncrypted[256];
	}//?????
	generateEncryptedHuffmanTable();
	decodeCipherStream();
#endif
	return 0;
}

int JPEGDecode::decodeDataStream() {
	width = sof.width();
	height = sof.height();
	if (sof.numberOfColor == 3) {
		r = new BYTE[width*height];
		g = new BYTE[width*height];
		b = new BYTE[width*height];
	}
	else {
		y = new BYTE[width*height];
	}
	int x_pos_max, y_pos_max;
	int numMCU; 
	SWORD(*yDeQT)[64], (*cbDeQT)[64], (*crDeQT)[64];
	float(*yMCU)[64];
	BYTE(*rMCU)[64], (*gMCU)[64], (*bMCU)[64];

	switch (sofColor[0].samplingCoefficient) {
	case 17: {
		x_pos_max = (int)ceil((float)width / 8);
		y_pos_max = (int)ceil((float)height / 8);
		numMCU = (x_pos_max)*(y_pos_max);//mcu总数
		if (sof.numberOfColor == 3) {
			yDeQT = new SWORD[numMCU][64];
			cbDeQT = new SWORD[numMCU][64];
			crDeQT = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				if (restartInterval) {
					if (restartCount == 0) {
						restartCount = dri.step();
						bytePos = 7;
					}
					restartCount--;
				}	
				readDC(yDcDehuff, yDiff, yPred, yDeQT[i]);
				readAC(yAcDehuff, yDiff, yDeQT[i]);
				readDC(cDcDehuff, cbDiff, cbPred, cbDeQT[i]);
				readAC(cAcDehuff, cbDiff, cbDeQT[i]);
				readDC(cDcDehuff, crDiff, crPred, crDeQT[i]);
				readAC(cAcDehuff, crDiff, crDeQT[i]);
			}
			rMCU = new BYTE[numMCU][64];
			gMCU = new BYTE[numMCU][64];
			bMCU = new BYTE[numMCU][64];

			int threadsNum = thread::hardware_concurrency() - 1;
			vector<thread> th;
			std::atomic<int> curMCU(0);

			auto process = [&](/*std::atomic<int> &curMCU*/) {
				int num;
				while ((num = curMCU++) < numMCU) {
					//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
					//{
					//	std::lock_guard<std::mutex> lock_guard(mtx);
					//	num = curMCU;
					//	if (curMCU >= numMCU)break;
					//	++curMCU;
					//	std::cout << "thread " << std::this_thread::get_id() << " has got permission to solve " << num << std::endl;
					//}
					SWORD tempA[64], tempB[64];
					float yTempC[64], cbTempC[64], crTempC[64];
					dequantization(yDeQT[num], tempA, yqt);
					reverseZZ(tempA, tempB);
					idct(tempB, yTempC);
					dequantization(cbDeQT[num], tempA, cqt);
					reverseZZ(tempA, tempB);
					idct(tempB, cbTempC);
					dequantization(crDeQT[num], tempA, cqt);
					reverseZZ(tempA, tempB);
					idct(tempB, crTempC);
					convertToRgb(rMCU[num], gMCU[num], bMCU[num], yTempC, cbTempC, crTempC);
				}
			};
			for (int j = 0; j < threadsNum; j++) {	//multi threads
				th.push_back(thread(process/*, std::ref(curMCU)*/));
			}
			process();//main thread
			for (auto &ths : th)ths.join();
			th.clear();
			delete[] yDeQT; delete[] cbDeQT; delete[] crDeQT;
		}//end if
		else {// if (sof.numberOfColor == 1) {
			yDeQT = new SWORD[numMCU][64];
			yMCU = new float[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				if (restartInterval) {
					if (restartCount == 0) {
						restartCount = dri.step();
						bytePos = 7;
					}
					restartCount--;
				}
				readDC(yDcDehuff, yDiff, yPred, yDeQT[i]);
				readAC(yAcDehuff, yDiff, yDeQT[i]);
			}
			int threadsNum = thread::hardware_concurrency() - 1;
			vector<thread> th;
			std::atomic<int> curMCU(0);
			auto process = [&](/*int &curMCU*/) {
				int num;
				SWORD tempA[64];
				SWORD tempB[64];
				while ((num = curMCU++) < numMCU) {
					dequantization(yDeQT[num], tempA, yqt);
					reverseZZ(tempA, tempB);
					idct(tempB, yMCU[num]);
				}
			};
			for (int j = 0; j < threadsNum; j++) {	//multi threads
				th.push_back(thread(process/*, std::ref(curMCU)*/));
			}
			process();//main thread
			for (auto &ths : th)ths.join();
			th.clear();
			delete[] yDeQT; 
		}//end else
		break;
	}//end case 17
	case 34: {
		x_pos_max = (int)ceil((float)width / 8);
		y_pos_max = (int)ceil((float)height / 8);
		numMCU = (x_pos_max)*(y_pos_max);//mcu总数
		rMCU = new BYTE[numMCU][64];
		gMCU = new BYTE[numMCU][64];
		bMCU = new BYTE[numMCU][64];
		x_pos_max = (int)ceil((float)width / 16);
		y_pos_max = (int)ceil((float)height / 16);
		numMCU = (x_pos_max)*(y_pos_max);//实际分块总数
		yDeQT = new SWORD[numMCU * 4][64];
		cbDeQT = new SWORD[numMCU][64];
		crDeQT = new SWORD[numMCU][64];
		for (int i = 0; i < numMCU; i++) {
			if (restartInterval) {
				if (restartCount == 0) {
					restartCount = dri.step();
					bytePos = 7;
				}
				restartCount--;
			}
			readDC(yDcDehuff, yDiff, yPred, yDeQT[4 * i]);
			readAC(yAcDehuff, yDiff, yDeQT[4 * i]);
			readDC(yDcDehuff, yDiff, yPred, yDeQT[4 * i + 1]);
			readAC(yAcDehuff, yDiff, yDeQT[4 * i + 1]);
			readDC(yDcDehuff, yDiff, yPred, yDeQT[4 * i + 2]);
			readAC(yAcDehuff, yDiff, yDeQT[4 * i + 2]);
			readDC(yDcDehuff, yDiff, yPred, yDeQT[4 * i + 3]);
			readAC(yAcDehuff, yDiff, yDeQT[4 * i + 3]);
			readDC(cDcDehuff, cbDiff, cbPred, cbDeQT[i]);
			readAC(cAcDehuff, cbDiff, cbDeQT[i]);
			readDC(cDcDehuff, crDiff, crPred, crDeQT[i]);
			readAC(cAcDehuff, crDiff, crDeQT[i]);
		}

		int threadsNum = thread::hardware_concurrency() - 1;
		vector<thread> th;
		atomic<int> curMCU(0);
		bool xExceed = false;
		bool yExceed = false;
		if (width % 16 && width % 16 <= 8) {
			xExceed = true;
		}
		if (height % 16 && height % 16 <= 8) {
			yExceed = true;
		}
		const int expand[4][64] = {
			{ 0,0,1,1,2,2,3,3,0,0,1,1,2,2,3,3,8,8,9,9,10,10,11,11,8,8,9,9,10,10,11,11,
			16,16,17,17,18,18,19,19,16,16,17,17,18,18,19,19,24,24,25,25,26,26,27,27,24,24,25,25,26,26,27,27 },
			{ 4,4,5,5,6,6,7,7,4,4,5,5,6,6,7,7,12,12,13,13,14,14,15,15,12,12,13,13,14,14,15,15,
			20,20,21,21,22,22,23,23,20,20,21,21,22,22,23,23,28,28,29,29,30,30,31,31,28,28,29,29,30,30,31,31 },
			{ 32,32,33,33,34,34,35,35,32,32,33,33,34,34,35,35,40,40,41,41,42,42,43,43,40,40,41,41,42,42,43,43,
			48,48,49,49,50,50,51,51,48,48,49,49,50,50,51,51,56,56,57,57,58,58,59,59,56,56,57,57,58,58,59,59 },
			{ 36,36,37,37,38,38,39,39,36,36,37,37,38,38,39,39,44,44,45,45,46,46,47,47,44,44,45,45,46,46,47,47,
			52,52,53,53,54,54,55,55,52,52,53,53,54,54,55,55,60,60,61,61,62,62,63,63,60,60,61,61,62,62,63,63 },
		};

		auto process = [&]{
			int num;
			while ((num = curMCU++) < numMCU) {
				SWORD tempA[64];
				SWORD tempB[64];
				float yMCU[4][64];
				float cbMCU[64], crMCU[64];
				float cbExpand[4][64], crExpand[4][64];
				int i = 0;
				for (int j = 4 * num; j < 4 * num + 4; j++) {
					dequantization(yDeQT[j], tempA, yqt);
					reverseZZ(tempA, tempB);
					idct(tempB, yMCU[i++]);
				}
				dequantization(cbDeQT[num], tempA, cqt);
				reverseZZ(tempA, tempB);
				idct(tempB, cbMCU);
				dequantization(crDeQT[num], tempA, cqt);
				reverseZZ(tempA, tempB);
				idct(tempB, crMCU);
				for (int j = 0; j < 4; j++) {
					for (int i = 0; i < 64; i++) {
						cbExpand[j][i] = cbMCU[expand[j][i]];
						crExpand[j][i] = crMCU[expand[j][i]];
					}
				}
				int rowPos = num / x_pos_max;
				int columnPos = num % x_pos_max;
				int blockBegin = rowPos * 4 * x_pos_max + columnPos * 2;
				if (xExceed)blockBegin -= rowPos * 2;
				int secondRowBegin = blockBegin + x_pos_max * 2;
				if (xExceed)secondRowBegin -= 1;
				convertToRgb(rMCU[blockBegin], gMCU[blockBegin], bMCU[blockBegin], yMCU[0], cbExpand[0], crExpand[0]);
				if (xExceed && columnPos != x_pos_max - 1) {
					++blockBegin;
					convertToRgb(rMCU[blockBegin], gMCU[blockBegin], bMCU[blockBegin], yMCU[1], cbExpand[1], crExpand[1]);
				}
				if (yExceed && rowPos == y_pos_max - 1)continue;
				convertToRgb(rMCU[secondRowBegin], gMCU[secondRowBegin], bMCU[secondRowBegin], yMCU[2], cbExpand[2], crExpand[2]);
				if (xExceed && columnPos != x_pos_max - 1) {
					++secondRowBegin;
					convertToRgb(rMCU[secondRowBegin], gMCU[secondRowBegin], bMCU[secondRowBegin], yMCU[3], cbExpand[3], crExpand[3]);
				}
			}
		};
		for (int j = 0; j < threadsNum; j++) {
			th.push_back(thread(process));
		}
		process();//main thread
		for (auto &ths : th)ths.join();
		th.clear();
		delete[] yDeQT; delete[] cbDeQT; delete[] crDeQT;
		break;
	}//end case 34
	case 0x21:
	case 0x12: {
		x_pos_max = (int)ceil((float)width / 8);
		y_pos_max = (int)ceil((float)height / 8);
		numMCU = (x_pos_max)*(y_pos_max);//mcu总数
		rMCU = new BYTE[numMCU][64];
		gMCU = new BYTE[numMCU][64];
		bMCU = new BYTE[numMCU][64];
		if (sofColor[0].samplingCoefficient == 0x21) {
			x_pos_max = (int)ceil((float)width / 16);
			y_pos_max = (int)ceil((float)height / 8);
		}
		else {//elif (sofColor[0].samplingCoefficient == 0x12)
			x_pos_max = (int)ceil((float)width / 8);
			y_pos_max = (int)ceil((float)height / 16);
		}
		numMCU = (x_pos_max)*(y_pos_max);//实际分块总数
		yDeQT = new SWORD[numMCU * 2][64];
		cbDeQT = new SWORD[numMCU][64];
		crDeQT = new SWORD[numMCU][64];
		for (int i = 0; i < numMCU; i++) {
			if (restartInterval) {
				if (restartCount == 0) {
					restartCount = dri.step();
					bytePos = 7;
				}
				restartCount--;
			}
			readDC(yDcDehuff, yDiff, yPred, yDeQT[4 * i]);
			readAC(yAcDehuff, yDiff, yDeQT[4 * i]);
			readDC(yDcDehuff, yDiff, yPred, yDeQT[4 * i + 1]);
			readAC(yAcDehuff, yDiff, yDeQT[4 * i + 1]);
			readDC(cDcDehuff, cbDiff, cbPred, cbDeQT[i]);
			readAC(cAcDehuff, cbDiff, cbDeQT[i]);
			readDC(cDcDehuff, crDiff, crPred, crDeQT[i]);
			readAC(cAcDehuff, crDiff, crDeQT[i]);
		}

		int threadsNum = thread::hardware_concurrency() - 1;
		vector<thread> th;
		atomic<int> curMCU(0);
		bool xExceed = false;
		bool yExceed = false;
		if (sofColor[0].samplingCoefficient == 0x21 && width % 16 && width % 16 <= 8) {
			xExceed = true;
		}
		else if (sofColor[0].samplingCoefficient == 0x12 && height % 16 && height % 16 <= 8) {
			yExceed = true;
		}
		const int expandVertical[2][64] = {
			0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,8,9,10,11,12,13,14,15,
			16,17,18,19,20,21,22,23,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,24,25,26,27,28,29,30,31,
			32,33,34,35,36,37,38,39,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,40,41,42,43,44,45,46,47,
			48,49,50,51,52,53,54,55,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,56,57,58,59,60,61,62,63
		};
		const int expandHorizontal[2][64] = {
			0,0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,16,16,17,17,18,18,19,19,20,20,21,21,22,22,23,23,
			32,32,33,33,34,34,35,35,36,36,37,37,38,38,39,39,48,48,49,49,50,50,51,51,52,52,53,53,54,54,55,55,
			8,8,9,9,10,10,11,11,12,12,13,13,14,14,15,15,24,24,25,25,26,26,27,27,28,28,29,29,30,30,31,31,
			40,40,41,41,42,42,43,43,44,44,45,45,46,46,47,47,56,56,57,57,58,58,59,59,60,60,61,61,62,62,63,63
		};

		auto process = [&] {
			int num;
			while ((num = curMCU++) < numMCU) {
				SWORD tempA[64];
				SWORD tempB[64];
				float yMCU[2][64];
				float cbMCU[64], crMCU[64];
				float cbExpand[2][64], crExpand[2][64];
				int i = 0;
				for (int j = 2 * num; j < 2 * num + 2; ++j) {
					dequantization(yDeQT[j], tempA, yqt);
					reverseZZ(tempA, tempB);
					idct(tempB, yMCU[i++]);
				}
				dequantization(cbDeQT[num], tempA, cqt);
				reverseZZ(tempA, tempB);
				idct(tempB, cbMCU);
				dequantization(crDeQT[num], tempA, cqt);
				reverseZZ(tempA, tempB);
				idct(tempB, crMCU);
				int rowPos = num / x_pos_max;
				int columnPos = num % x_pos_max;
				if (sofColor[0].samplingCoefficient == 0x21) {
					for (int j = 0; j < 2; j++) {
						for (int i = 0; i < 64; i++) {
							cbExpand[j][i] = cbMCU[expandHorizontal[j][i]];
							crExpand[j][i] = crMCU[expandHorizontal[j][i]];
						}
					}
					int blockBegin = num * 2;
					if (xExceed)blockBegin -= rowPos * 2;
					convertToRgb(rMCU[blockBegin], gMCU[blockBegin], bMCU[blockBegin], yMCU[0], cbExpand[0], crExpand[0]);
					if (xExceed && columnPos == x_pos_max - 1)continue;
					++blockBegin;
					convertToRgb(rMCU[blockBegin], gMCU[blockBegin], bMCU[blockBegin], yMCU[1], cbExpand[1], crExpand[1]);
				}
				else {
					for (int j = 0; j < 2; j++) {
						for (int i = 0; i < 64; i++) {
							cbExpand[j][i] = cbMCU[expandVertical[j][i]];
							crExpand[j][i] = crMCU[expandVertical[j][i]];
						}
					}
					int blockBegin = rowPos * 2 * x_pos_max + columnPos;
					//if (xExceed)blockBegin -= rowPos * 2;
					int secondRowBegin = blockBegin + x_pos_max;
					convertToRgb(rMCU[blockBegin], gMCU[blockBegin], bMCU[blockBegin], yMCU[0], cbExpand[0], crExpand[0]);
					if (yExceed && rowPos == y_pos_max - 1)continue;
					convertToRgb(rMCU[secondRowBegin], gMCU[secondRowBegin], bMCU[secondRowBegin], yMCU[2], cbExpand[2], crExpand[2]);
				}
			}
		};
		for (int j = 0; j < threadsNum; j++) {
			th.push_back(thread(process));
		}
		process();//main thread
		for (auto &ths : th)ths.join();
		th.clear();
		delete[] yDeQT; delete[] cbDeQT; delete[] crDeQT;
		break;
	}
	default: {
		throw "numberOfColor error";
	}
	}

	x_pos_max = (int)ceil((float)width / 8);
	y_pos_max = (int)ceil((float)height / 8);
	//numMCU = (x_pos_max)*(y_pos_max);
	int MCU_count = 0;
	int x_remainder, y_remainder;//剩余几个像素
	x_remainder = width & 7;
	y_remainder = height & 7;
	for (int y_pos = 0; y_pos < y_pos_max; y_pos++) {
		for (int x_pos = 0; x_pos < x_pos_max; x_pos++) {
			int pos_base = 8 * x_pos + width * 8 * y_pos;
			int y_total, x_total;
			if ((x_pos == x_pos_max-1) && (x_remainder != 0))
				x_total = x_remainder;
			else x_total = 8;
			if ((y_pos == y_pos_max-1) && (y_remainder != 0))
				y_total = y_remainder;
			else y_total = 8;
			for (int y = 0; y < y_total; y++) {
				for (int x = 0; x < x_total; x++) {
					int pos = pos_base + x + y * width;
					int pos_MCU = x + y * 8;
					switch (sof.numberOfColor) {
					case 3:
						r[pos] = rMCU[MCU_count][pos_MCU];
						g[pos] = gMCU[MCU_count][pos_MCU];
						b[pos] = bMCU[MCU_count][pos_MCU];
						break;
					case 1:
						int tmp = (int)round(yMCU[MCU_count][pos_MCU]) + 128;
						if (tmp > 255)tmp = 255;
						else if (tmp < 0)tmp = 0;
						this->y[pos] = (BYTE)tmp;
					}
				}
			}
			MCU_count++;
		}
	}
	switch (sof.numberOfColor) {
	case 3:	delete[] rMCU; delete[] gMCU; delete[] bMCU; break;
	case 1: delete[] yMCU; 
	}

	return 0;
}
int JPEGDecodeDecrypt::decodeDataStream() {
	width = sof.width();
	height = sof.height();
	if (sof.numberOfColor == 3) {
		r = new BYTE[width*height];
		g = new BYTE[width*height];
		b = new BYTE[width*height];
	}
	else {
		throw "encrypted file format error";
	}
	int x_pos_max, y_pos_max;
	int numMCU;
	SWORD(*yDeQT)[64], (*cbDeQT)[64], (*crDeQT)[64];
	//float(*yMCU)[64];
	BYTE(*rMCU)[64], (*gMCU)[64], (*bMCU)[64];

	switch (sofColor[0].samplingCoefficient) {
	case 17: {
		x_pos_max = (int)ceil((float)width / 8);
		y_pos_max = (int)ceil((float)height / 8);
		numMCU = (x_pos_max)*(y_pos_max);//mcu总数
		rMCU = new BYTE[numMCU][64];
		gMCU = new BYTE[numMCU][64];
		bMCU = new BYTE[numMCU][64];
		if (sof.numberOfColor == 3) {
			yDeQT = new SWORD[numMCU][64];
			cbDeQT = new SWORD[numMCU][64];
			crDeQT = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				if (restartInterval) {
					if (restartCount == 0) {
						restartCount = dri.step();
						bytePos = 7;
					}
					restartCount--;
				}
				readDC(yDcDehuff, yDiff, yPred, yDeQT[i]);
				readAC(yAcDehuff, yDiff, yDeQT[i]);
				readDC(cDcDehuff, cbDiff, cbPred, cbDeQT[i]);
				readAC(cAcDehuff, cbDiff, cbDeQT[i]);
				readDC(cDcDehuff, crDiff, crPred, crDeQT[i]);
				readAC(cAcDehuff, crDiff, crDeQT[i]);
			}

#ifdef ENCRYPT
			{
#ifdef BENCHMARK
				clock_t begin = clock();
#endif
				string posFileName = fileName;
				posFileName.erase(fileName.length() - 4, 4);
				posFileName += ".txt";
				ifstream posFile(posFileName);
				ifstream objectFile("objects.txt");
				if (posFile.good() && objectFile.good()) {
					vector<ObjectPosition> objectPosition;
					set<string> objectName;
					while (posFile.good()) {
						ObjectPosition temp(posFile);
						objectPosition.push_back(temp);
					}
					while (objectFile.good()) {
						string str;
						objectFile >> str;
						if(str.size())
							objectName.insert(str);
					}
					for (vector<ObjectPosition>::reverse_iterator it = objectPosition.rbegin(); it != objectPosition.rend(); ++it) {
						if (objectName.find(it->type) != objectName.end()) {
							Object *obj = new Object(*it, x_pos_max, yDeQT, cbDeQT, crDeQT);
							decrypt(obj->yData, obj->cbData, obj->crData, obj->numMCU);
							obj->writeBack(yDeQT, cbDeQT, crDeQT);
							delete obj;
						}
					}
					posFile.close();
					objectFile.close();
				}
				else {
					decrypt(yDeQT, cbDeQT, crDeQT, numMCU);
				}
#ifdef BENCHMARK
				cout << "Decryption cost time " << (float)(clock() - begin) / CLOCKS_PER_SEC << 's' << endl;
#endif
			}
#endif

			ofstream file("zerocount.txt");
			file << "x max: " << x_pos_max << '\t' << "y max: " << y_pos_max << endl;
			int *count=new int[numMCU];
			memset(count, 0, sizeof(int)*numMCU);
			for (int i = 0; i < numMCU; i++) {
				for (int j = 0; j < 64; j++) {
					(!yDeQT[i][j])?count[i]++:false;
				}
				file << count[i] << '\t';
			}
			file.close();
			delete[] count;

			int threadsNum = thread::hardware_concurrency() - 1;
			vector<thread> th;
			std::atomic<int> curMCU(0);

			auto process = [&](/*std::atomic<int> &curMCU*/) {
				int num;
				while ((num = curMCU++) < numMCU) {
					//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
					//{
					//	std::lock_guard<std::mutex> lock_guard(mtx);
					//	num = curMCU;
					//	if (curMCU >= numMCU)break;
					//	++curMCU;
					//	std::cout << "thread " << std::this_thread::get_id() << " has got permission to solve " << num << std::endl;
					//}
					SWORD tempA[64], tempB[64];
					float yTempC[64], cbTempC[64], crTempC[64];
					dequantization(yDeQT[num], tempA, yqt);
					reverseZZ(tempA, tempB);
					idct(tempB, yTempC);
					dequantization(cbDeQT[num], tempA, cqt);
					reverseZZ(tempA, tempB);
					idct(tempB, cbTempC);
					dequantization(crDeQT[num], tempA, cqt);
					reverseZZ(tempA, tempB);
					idct(tempB, crTempC);
					convertToRgb(rMCU[num], gMCU[num], bMCU[num], yTempC, cbTempC, crTempC);
				}
			};
			for (int j = 0; j < threadsNum; j++) {	//multi threads
				th.push_back(thread(process/*, std::ref(curMCU)*/));
			}
			process();//main thread
			for (auto &ths : th)ths.join();
			th.clear();
			delete[] yDeQT; delete[] cbDeQT; delete[] crDeQT;
			}//end if
		else {
			throw "encrypted file format error";
		}//end else
		break;
	default:
		throw "encrypted file format error";
	}//end case 17
	}
	int MCU_count = 0;
	int x_remainder, y_remainder;//剩余几个像素
	x_remainder = width & 7;
	y_remainder = height & 7;
	for (int y_pos = 0; y_pos < y_pos_max; y_pos++) {
		for (int x_pos = 0; x_pos < x_pos_max; x_pos++) {
			int pos_base = 8 * x_pos + width * 8 * y_pos;
			int y_total, x_total;
			if ((x_pos == x_pos_max - 1) && (x_remainder != 0))
				x_total = x_remainder;
			else x_total = 8;
			if ((y_pos == y_pos_max - 1) && (y_remainder != 0))
				y_total = y_remainder;
			else y_total = 8;
			for (int y = 0; y < y_total; y++) {
				for (int x = 0; x < x_total; x++) {
					int pos = pos_base + x + y * width;
					int pos_MCU = x + y * 8;
					switch (sof.numberOfColor) {
					case 3:
						r[pos] = rMCU[MCU_count][pos_MCU];
						g[pos] = gMCU[MCU_count][pos_MCU];
						b[pos] = bMCU[MCU_count][pos_MCU];
						break;
					//case 1:
					//	int tmp = (int)round(yMCU[MCU_count][pos_MCU]) + 128;
					//	if (tmp > 255)tmp = 255;
					//	else if (tmp < 0)tmp = 0;
					//	this->y[pos] = (BYTE)tmp;
					}
				}
			}
			MCU_count++;
		}
	}
	return 0;
}

#ifdef ENCRYPT_HUFF
int JPEGDecodeDecrypt::decodeCipherStream() {
	width = sof.width();
	height = sof.height();
	if (sof.numberOfColor == 3) {
		r = new BYTE[width*height];
		g = new BYTE[width*height];
		b = new BYTE[width*height];
	}
	else {
		y = new BYTE[width*height];
	}
	int x_pos, y_pos;
	int x_pos_max, y_pos_max;
	int numMCU;

	switch (sofColor[0].samplingCoefficient) {
	case 17: {
		x_pos_max = (int)ceil((float)width / 8) - 1;
		y_pos_max = (int)ceil((float)height / 8) - 1;
		numMCU = (x_pos_max + 1)*(y_pos_max + 1);//mcu总数
		if (sof.numberOfColor == 3) {
			yDeQT = new SWORD[numMCU][64];
			cbDeQT = new SWORD[numMCU][64];
			crDeQT = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				if (restartInterval) {
					if (restartCount == 0) {
						restartCount = dri.step();
						bytePos = 7;
					}
					restartCount--;
				}
				if ((i & 63) == 63) generateEncryptedHuffmanTable();
				readDcCipher(dehuffEncrypt[0], yDiff, yPred, yDeQT[i]);
				readAcCipher(dehuffEncrypt[1], yDiff, yDeQT[i]);
				readDcCipher(dehuffEncrypt[2], cbDiff, cbPred, cbDeQT[i]);
				readAcCipher(dehuffEncrypt[3], cbDiff, cbDeQT[i]);
				readDcCipher(dehuffEncrypt[2], crDiff, crPred, crDeQT[i]);
				readAcCipher(dehuffEncrypt[3], crDiff, crDeQT[i]);
				if (i == 12199) {
					int a = 0;
					a += a;
				}
			}
			yZZ = new SWORD[numMCU][64];
			cbZZ = new SWORD[numMCU][64];
			crZZ = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				dequantization(yDeQT[i], yZZ[i], yqt);
				dequantization(cbDeQT[i], cbZZ[i], cqt);
				dequantization(crDeQT[i], crZZ[i], cqt);
			}
			delete[] yDeQT; delete[] cbDeQT; delete[] crDeQT;

			yIDCT = new SWORD[numMCU][64];
			cbIDCT = new SWORD[numMCU][64];
			crIDCT = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				reverseZZ(yZZ[i], yIDCT[i]);
				reverseZZ(cbZZ[i], cbIDCT[i]);
				reverseZZ(crZZ[i], crIDCT[i]);
			}
			delete[] yZZ; delete[] cbZZ; delete[] crZZ;

			yMCU = new double[numMCU][64];
			cbMCU = new double[numMCU][64];
			crMCU = new double[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				idct(yIDCT[i], yMCU[i]);
				idct(cbIDCT[i], cbMCU[i]);
				idct(crIDCT[i], crMCU[i]);
			}
			delete[] yIDCT; delete[] cbIDCT; delete[] crIDCT;

			rMCU = new BYTE[numMCU][64];
			gMCU = new BYTE[numMCU][64];
			bMCU = new BYTE[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				convertToRgb(rMCU[i], gMCU[i], bMCU[i], yMCU[i], cbMCU[i], crMCU[i]);
			}
			delete[] yMCU; delete[] cbMCU; delete[] crMCU;
		}//end if
		else {// if (sof.numberOfColor == 1) {
			yDeQT = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				if (restartInterval) {
					if (restartCount == 0) {
						restartCount = dri.step();
						bytePos = 7;
					}
					restartCount--;
				}
				readDC(yDcDehuff, yDiff, yPred, yDeQT[i]);
				readAC(yAcDehuff, yDiff, yDeQT[i]);
			}
			yZZ = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				dequantization(yDeQT[i], yZZ[i], yqt);
			}
			delete[] yDeQT;

			yIDCT = new SWORD[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				reverseZZ(yZZ[i], yIDCT[i]);
			}
			delete[] yZZ;

			yMCU = new double[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				idct(yIDCT[i], yMCU[i]);
			}
			delete[] yIDCT;
		}//end else
		break;
	}//end case 17
	default: {
		throw "numberOfColor error";
	}
	}
	int MCU_count = 0;
	int x_remainder, y_remainder;//剩余几个像素
	x_remainder = width & 7;
	y_remainder = height & 7;
	for (y_pos = 0; y_pos <= y_pos_max; y_pos++) {
		for (x_pos = 0; x_pos <= x_pos_max; x_pos++) {
			int pos_base = 8 * x_pos + width * 8 * y_pos;
			int y_total, x_total;
			if ((x_pos == x_pos_max) && (x_remainder != 0))
				x_total = x_remainder;
			else x_total = 8;
			if ((y_pos == y_pos_max) && (y_remainder != 0))
				y_total = y_remainder;data
			else y_total = 8;
			for (int y = 0; y < y_total; y++) {
				for (int x = 0; x < x_total; x++) {
					int pos = pos_base + x + y * width;
					int pos_MCU = x + y * 8;
					switch (sof.numberOfColor) {
					case 3:
						r[pos] = rMCU[MCU_count][pos_MCU];
						g[pos] = gMCU[MCU_count][pos_MCU];
						b[pos] = bMCU[MCU_count][pos_MCU];
						break;
					case 1:
						int tmp = (int)round(yMCU[MCU_count][pos_MCU]) + 128;
						if (tmp > 255)tmp = 255;
						else if (tmp < 0)tmp = 0;
						y[pos] = (BYTE)tmp;
					}
				}
			}
			MCU_count++;
		}
	}
	switch (sof.numberOfColor) {
	case 3:	delete[] rMCU; delete[] gMCU; delete[] bMCU; break;
	case 1: delete[] yMCU;
	}

	return 0;
}
#endif
void JPEGDecode::convertToRgb(BYTE *rMCU, BYTE *gMCU, BYTE *bMCU, float *yMCU, float *cbMCU, float *crMCU) {
	float tmp;
	for (int i = 0; i < 64; i++) {
		tmp = (yMCU[i] + 128) + 1.402f*crMCU[i];
		if (tmp > 255)tmp = 255;
		else if (tmp < 0)tmp = 0;
		rMCU[i] = (BYTE)round(tmp);
		tmp = (yMCU[i] + 128) - 0.34414f*cbMCU[i] - 0.71414f*crMCU[i];
		if (tmp > 255)tmp = 255;
		else if (tmp < 0)tmp = 0;
		gMCU[i] = (BYTE)round(tmp);
		tmp = (yMCU[i] + 128) + 1.772f*cbMCU[i];
		if (tmp > 255)tmp = 255;
		else if (tmp < 0)tmp = 0;
		bMCU[i] = (BYTE)round(tmp);
	}
}
/*
void JPEGDecode::idct(SWORD*idct, float*mcu) {
	BYTE x, y;
	BYTE u, v;
	float Cu, Cv;
	float temp = 0;
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			temp = 0;
			for (v = 0; v < 8; v++) {
				for (u = 0; u < 8; u++) {
					int pos = u + v * 8;
					if (u == 0)Cu = 1 / sqrt(2);
					else Cu = 1;
					if (v == 0)Cv = 1 / sqrt(2);
					else Cv = 1;
					temp += 0.25*idct[pos] * Cu * Cv* cos((2 * x + 1)* u * PI / 16)*cos((2 * y + 1) * v * PI / 16);
				}
			}
			int pos = x + y * 8;
			mcu[pos] = temp;
		}
	}
}
*/
void JPEGDecode::idct(SWORD*idct,float*mcu) {
	float AT[64], f[64];
	mat_Transpose((float*)A, AT, 8, 8);
	mat_mul(AT, idct, f, 8, 8, 8);
	mat_mul(f, (float*)A, mcu, 8, 8, 8);
}
void JPEGDecode::dequantization(SWORD*deQT,SWORD*result,BYTE*QT) {
	for (int i = 0; i < 64; i++) {
		result[i] = deQT[i] * (SWORD)QT[i];
	}
}
void JPEGDecode::reverseZZ(SWORD*ZZ, SWORD*result) {
	for (int i = 0; i < 64; i++)
		result[i] = ZZ[zigzag[i]];//注意此处的zig-zag正反问题，这里是反zz
}
int JPEGDecode::readDC(DEHUFF*deHuff, SWORD&diff, SWORD&pred, SWORD*result) {
	WORD code;
	BYTE offset;//当前的位数
	BYTE value;
	//查找哈夫曼表
	for (offset = 1, code = 0; 1; offset++) {
		code += readOneBit();
		if (deHuff[code].length == offset) {
			value = deHuff[code].value;
			break;
		}
		code <<= 1;
	}
	if (value) {
		diff = readOneBit();
		if (diff) {
			for (char i = 0; i < (value - 1); i++) {
				diff <<= 1;
				diff += readOneBit();
			}
		}//正数
		else {
			diff = 1;
			for (char i = 0; i < (value - 1); i++) {
				diff <<= 1;
				if (!readOneBit())diff++;
			}
			diff = -diff;
		}//负数
	}
	else diff = 0;
	result[0] = diff + pred;
	pred += diff;
	return 0;
}
int JPEGDecode::readAC(DEHUFF*DEHUFF, SWORD&diff, SWORD*result) {
	WORD code;
	BYTE offset;//当前的位数
	BYTE value;
	SWORD digit = 0;//ac位数
	char zero_run_length = 0;//连0游程编码
	SWORD ac_value = 0;//结果

	for (unsigned char k = 1; k <= 63; k++) {
		//查找哈夫曼表
		for (offset = 1, code = 0; 1; offset++) {
			code += readOneBit();
			if (DEHUFF[code].length == offset) {
				value = DEHUFF[code].value;
				break;
			}
			code <<= 1;
		}
		if (value == 0xf0) {//ZRL
			for (unsigned char i = 0; i < 16; i++) {
				result[k] = 0;
				k++;
			}
			k--;
			continue;
		}
		if (value == 0) {//EOB
			for (unsigned char i = 64 - k; i > 0; i--) {
				result[k] = 0;
				k++;
			}
			break;
		}

		zero_run_length = value / 16;
		digit = value - zero_run_length * 16;
		for (unsigned char i = zero_run_length; i > 0; i--) {
			result[k] = 0;
			k++;
		}
		ac_value = readOneBit();
		if (ac_value) {
			for (char i = 0; i < (digit - 1); i++) {
				ac_value <<= 1;
				ac_value += readOneBit();
			}
		}
		else {
			ac_value = 1;
			for (char i = 0; i < (digit - 1); i++) {
				ac_value <<= 1;
				if (!readOneBit())ac_value++;
			}
			ac_value = -ac_value;
		}
		result[k] = ac_value;
	}
	return 0;
}
#ifdef ENCRYPT_HUFF
int JPEGDecodeDecrypt::readDcCipher(DEHUFFEncrypted*dehuffEncrypt, SWORD&diff, SWORD&pred, SWORD*result) {
	WORD code;
	BYTE offset;//当前的位数
	BYTE value;
	//查找哈夫曼表
	for (offset = 1, code = 0; 1; offset++) {
		code += readOneBit();
		bool cmd = false;
		for (int i = 0; i < 256; i++) {
			if (dehuffEncrypt[i].code == code && dehuffEncrypt[i].length == offset) {
				value = i;
				cmd = true;
				break;
			}
		}
		if (cmd) break;
		code <<= 1;
	}
	if (value) {
		diff = readOneBit();
		if (diff) {
			for (char i = 0; i < (value - 1); i++) {
				diff <<= 1;
				diff += readOneBit();
			}
		}//正数
		else {
			diff = 1;
			for (char i = 0; i < (value - 1); i++) {
				diff <<= 1;
				if (!readOneBit())diff++;
			}
			diff = -diff;
		}//负数
	}
	else diff = 0;
	result[0] = diff + pred;
	pred += diff;
	return 0;
}
int JPEGDecodeDecrypt::readAcCipher(DEHUFFEncrypted*dehuffEncrypt, SWORD&diff, SWORD*result) {
	WORD code;
	BYTE offset;//当前的位数
	BYTE value;
	SWORD digit = 0;//ac位数
	char zero_run_length = 0;//连0游程编码
	SWORD ac_value = 0;//结果

	for (unsigned char k = 1; k <= 63; k++) {
		//查找哈夫曼表
		for (offset = 1, code = 0; 1; offset++) {
			bool cmd = false;
			code += readOneBit();
			for (int i = 0; i < 256; i++) {
				if (dehuffEncrypt[i].code == code && dehuffEncrypt[i].length == offset) {
					value = i;
					cmd = true;
					break;
				}
			}
			if (cmd) break;
			code <<= 1;
		}
		if (value == 0xf0) {//ZRL
			for (unsigned char i = 0; i < 16; i++) {
				result[k] = 0;
				k++;
			}
			k--;
			continue;
		}
		if (value == 0) {//EOB
			for (unsigned char i = 64 - k; i > 0; i--) {
				result[k] = 0;
				k++;
			}
			break;
		}

		zero_run_length = value / 16;
		digit = value - zero_run_length * 16;
		for (char i = zero_run_length; i > 0; i--) {
			result[k] = 0;
			k++;
		}
		ac_value = readOneBit();
		if (ac_value) {
			for (char i = 0; i < (digit - 1); i++) {
				ac_value <<= 1;
				ac_value += readOneBit();
			}
		}
		else {
			ac_value = 1;
			for (char i = 0; i < (digit - 1); i++) {
				ac_value <<= 1;
				if (!readOneBit())ac_value++;
			}
			ac_value = -ac_value;
		}
		result[k] = ac_value;
	}
	return 0;
}
#endif
BYTE JPEGDecode::readOneBit() {
	BYTE value;
	if (bytePos == 7) {
		while (1) {
			file.read((char*)&byteNew, 1);	
			if (byteNew == 0xff) {
				file.read((char*)&byteNew, 1);
				if (byteNew == 0) {
					byteNew = 0xff;
					break;
				}
				else if ((byteNew & 0xf8) == 0xd0) {//D0~D7
					yPred = cbPred = crPred = 0;
					continue;
				}
				else {
					throw "file stream error";
					break;
				}
			}
			break;
		}
	}
	if (byteNew & binaryMask[bytePos]) value = 1;//值的当前位为1 (二进制)
	else value = 0;
	bytePos--;
	if (bytePos < 0) {
		bytePos = 7;
		byteNew = 0;
	}

	return value;
}
#ifdef ENCRYPT_HUFF
int JPEGDecodeDecrypt::generateEncryptedHuffmanTable(DHT dht, BYTE * huffLength, WORD * huffCode, BYTE * huffValue, DEHUFFEncrypted * dehuffEncrypt) {
	WORD sum = dht.htBitTableSum();//JPEG中 code总数=节点总数
	BYTE current_length;//1~16
	BYTE current_length_sum;//当前长度有几个值
	WORD current_node = 1;//当前用到第几个节点
	WORD current_value_number = 0;//顺次记录第几个值
	WORD next_length_node = 2;
	Huffmantree*Huffman_Tree = new Huffmantree[sum + 1];//0不用

	bool stopLoop = false;
	//生成树
	for (current_length = 1; current_length <= 16; current_length++) {
		current_length_sum = dht.htBitTable[current_length - 1];
		for (int i = next_length_node - current_node; i > 0; i--) {
			if (current_length_sum > 0) {
				Huffman_Tree[current_node].lLength = current_length;
				Huffman_Tree[current_node].lValue = huffValue[current_value_number];
				current_length_sum--;
				current_value_number++;
			}
			else {
				Huffman_Tree[current_node].lChild = next_length_node++;
			}
			if (current_length_sum > 0) {
				Huffman_Tree[current_node].rLength = current_length;
				Huffman_Tree[current_node].rValue = huffValue[current_value_number];
				current_length_sum--;
				current_value_number++;
			}
			else {
				if (next_length_node > sum) {
					stopLoop = true; break;
				}
				Huffman_Tree[current_node].rChild = next_length_node++;
			}
			current_node++;
		}
		if (stopLoop) break;
	}
	//哈夫曼mutate
	for (int i = 1; i <= sum; i++) {
		BYTE byte = ((long long)logistic()*1.0e+14) % 256;
		if (byte & 1) Huffman_Tree[i].mutate();
	}
	//生成code
	for (int i = 1; i <= sum; i++) {
		Huffman_Tree[i].lCode <<= 1;
		Huffman_Tree[i].rCode <<= 1;
		Huffman_Tree[i].rCode++;
		Huffman_Tree[i].lChild != 0 ? Huffman_Tree[Huffman_Tree[i].lChild].lCode = Huffman_Tree[Huffman_Tree[i].lChild].rCode = Huffman_Tree[i].lCode : false;
		Huffman_Tree[i].rChild != 0 ? Huffman_Tree[Huffman_Tree[i].rChild].lCode = Huffman_Tree[Huffman_Tree[i].rChild].rCode = Huffman_Tree[i].rCode : false;
	}
	//合成索引表
	for (int i = 1; i <= sum; i++) {
		if (Huffman_Tree[i].lLength != 0) {
			WORD temp = Huffman_Tree[i].lValue;
			dehuffEncrypt[temp].length = Huffman_Tree[i].lLength;
			dehuffEncrypt[temp].code = Huffman_Tree[i].lCode;
		}
		if (Huffman_Tree[i].rLength != 0) {
			WORD temp = Huffman_Tree[i].rValue;
			dehuffEncrypt[temp].length = Huffman_Tree[i].rLength;
			dehuffEncrypt[temp].code = Huffman_Tree[i].rCode;
		}
	}
	delete[] Huffman_Tree;
	return 0;
}

void JPEGDecodeDecrypt::generateEncryptedHuffmanTable() {
	generateEncryptedHuffmanTable(dht[0], yDcHuffLength, yDcHuffCode, yDcHuffVal, dehuffEncrypt[0]);
	generateEncryptedHuffmanTable(dht[1], yAcHuffLength, yAcHuffCode, yAcHuffVal, dehuffEncrypt[1]);
	if (sof.numberOfColor == 3) {//YCbCr彩色
		generateEncryptedHuffmanTable(dht[2], cDcHuffLength, cDcHuffCode, cDcHuffVal, dehuffEncrypt[2]);
		generateEncryptedHuffmanTable(dht[3], cAcHuffLength, cAcHuffCode, cAcHuffVal, dehuffEncrypt[3]);
	}
}
#endif
void JPEGDecode::generateHuffmanTable() {
	generateHuffmanTable(dht[0], yDcHuffLength, yDcHuffCode, yDcHuffVal, yDcDehuff);
	generateHuffmanTable(dht[1], yAcHuffLength, yAcHuffCode, yAcHuffVal, yAcDehuff);
	if (sof.numberOfColor == 3) {//YCbCr彩色
		generateHuffmanTable(dht[2], cDcHuffLength, cDcHuffCode, cDcHuffVal, cDcDehuff);
		generateHuffmanTable(dht[3], cAcHuffLength, cAcHuffCode, cAcHuffVal, cAcDehuff);
	}
}
int JPEGDecode::generateHuffmanTable(DHT dht, BYTE*huffLength, WORD*huffCode, BYTE*huffValue, DEHUFF*DEHUFF) {
	//生成 length table
	int htBitTableSum = dht.htBitTableSum();
	int k = 0, i = 0, j = 1;
	do {
		if (j <= dht.htBitTable[i]) {
			huffLength[k] = i + 1;
			k++; j++;
		}
		else {
			i++; j = 1;
		}
	} while (k < htBitTableSum);
	//生成 code table
	k = 0;
	WORD code = 0;
	WORD si = huffLength[0];
	while (1) {
		do {
			huffCode[k] = code;
			code++;
			k++;
			if (k >= htBitTableSum) break;
		} while (huffLength[k] == si);
		if (k >= htBitTableSum) break;
		do {
			code <<= 1;
			si++;
		} while (huffLength[k] != si);
	}
	//合成索引表
	for (int i = 0; i < dht.htBitTableSum(); i++) {
		WORD temp = huffCode[i];
		DEHUFF[temp].length = huffLength[i];
		DEHUFF[temp].value = huffValue[i];
	}
	return 0;
}
int JPEGDecode::readSegments() {
	BYTE btmp = 0xff;
	bool Data_Stream_Begin = 0;
	while (!Data_Stream_Begin) {
		while (btmp == 0xff)
			file.read((char*)&btmp, 1);
		switch (btmp) {
		case 0xd8: {
			file.read((char*)&btmp, 1);
			break;
		}//Start of Image，图像开始 216
		case 0xd9: {
			cout << "FFD9";
			return 1;//文件结束
			break;
		}//EOI
		case 0xc0: { //Start of Frame 0，帧图像开始 192 end_multi
			sequentialOrProgressive = 0xc0;
			file.read((char*)&sof, sizeof(SOF));
			if (sof.numberOfColor == 4) {
				throw "不支持CMYK颜色模式";
			}
			sofColor = new SOFColour[sof.numberOfColor];
			for (int i = 0; i < sof.numberOfColor; i++)
				file.read((char*)&sofColor[i], sizeof(SOFColour));
			break;
		}
		case 0xc2: { //Start of Frame 2
			sequentialOrProgressive = 0xc2;
			file.read((char*)&sof, sizeof(SOF));
			sofColor = new SOFColour[sof.numberOfColor];
			for (int i = 0; i < sof.numberOfColor; i++)
				file.read((char*)&sofColor[i], sizeof(SOFColour));
			break;
		}
		case 0xc4: {
			if (readHuffmanTable() == -2)exit(-2);
			break;
		}//Difine Huffman Table，定义哈夫曼表 196 multi
		case 0xda: {
			file.read((char*)&sos, sizeof(SOS));
			sosColor = new SOSColour[sos.numberOfColor];
			for (int i = 0; i < sos.numberOfColor; i++)
				file.read((char*)&sosColor[i], sizeof(SOSColour));
			ss = file.get();
			se = file.get();
			sA = file.get();
			Data_Stream_Begin = true;
			break;
		}//Start of Scan，扫描开始 218 data
		case 0xdb: {
			readQuantizationTable();
			break;
		}//Define Quantization Table，定义量化表 219 multi
		case 0xdd: {
			file.read((char*)&dri, sizeof(DRI));
			restartCount = dri.step();
			restartInterval = true;
			cout << dec << "DRI段 间隔" << restartCount << endl;
			break;
		}//Define Restart Interval，定义差分编码累计复位的间隔 221 如果没有本标记段，或间隔值为0时，就表示不存在重开始间隔和标记RST
		case 0xe0: {
			file.read((char*)&app0, sizeof(APP0));
			if (app0.length() != 16) {
				cout << "有缩略图" << endl;
				file.seekg(app0.length() - 16, ios::cur);//万一有缩略图就跳过
				app0.xThumbnail = app0.yThumbnail = 0;
				app0.lengthH = 0; app0.lengthL = 16;
			}
			break;
		}//Application，应用程序保留标记0 224
		 //case 0xdc: break;//DNL 220 seldom_exist
		default:otherSegments();
		}
		btmp = 0xff;
	}
	return 0;
}
int JPEGDecode::readHuffmanTable() {
	int HT_Bit_Table_Size;
	BYTE htInfo;
	BYTE lengthH;//段长度高字节
	BYTE lengthL;//段长度低字节
	file.read((char*)&lengthH, 1);
	file.read((char*)&lengthL, 1);
	WORD length = lengthH * 0x100 + lengthL - 2; //求段长度
	while (length) {
		file.read((char*)&htInfo, 1);
		file.seekg(-1, ios::cur);
		switch (htInfo) {
		case 0x00: {
			if (dht0Generated) {
				delete[]yDcHuffVal; delete[]yDcHuffLength; delete[]yDcHuffCode;
			}
			dht0Generated = true;
			file.read((char*)&dht[0], sizeof(DHT));
			HT_Bit_Table_Size = dht[0].htBitTableSum();
			yDcHuffVal = new BYTE[HT_Bit_Table_Size];
			yDcHuffLength = new BYTE[HT_Bit_Table_Size];
			yDcHuffCode = new WORD[HT_Bit_Table_Size];
			file.read((char*)yDcHuffVal, HT_Bit_Table_Size);
			break;
		}
		case 0x10: {
			if (dht1Generated) {
				delete[]yAcHuffVal; delete[]yAcHuffLength; delete[]yAcHuffCode;
			}
			dht1Generated = true;
			file.read((char*)&dht[1], sizeof(DHT));
			HT_Bit_Table_Size = dht[1].htBitTableSum();
			yAcHuffVal = new BYTE[HT_Bit_Table_Size];
			yAcHuffLength = new BYTE[HT_Bit_Table_Size];
			yAcHuffCode = new WORD[HT_Bit_Table_Size];
			file.read((char*)yAcHuffVal, HT_Bit_Table_Size);
			break;
		}
		case 0x01: {
			if (dht2Generated) {
				delete[]cDcHuffVal; delete[]cDcHuffLength; delete[]cDcHuffCode;
			}
			dht2Generated = true;
			file.read((char*)&dht[2], sizeof(DHT));
			HT_Bit_Table_Size = dht[2].htBitTableSum();
			cDcHuffVal = new BYTE[HT_Bit_Table_Size];
			cDcHuffLength = new BYTE[HT_Bit_Table_Size];
			cDcHuffCode = new WORD[HT_Bit_Table_Size];
			file.read((char*)cDcHuffVal, HT_Bit_Table_Size);
			break;
		}
		case 0x11: {
			if (dht3Generated) {
				delete[]cAcHuffVal; delete[]cAcHuffLength; delete[]cAcHuffCode;
			}
			dht3Generated = true;
			file.read((char*)&dht[3], sizeof(DHT));
			HT_Bit_Table_Size = dht[3].htBitTableSum();
			cAcHuffVal = new BYTE[HT_Bit_Table_Size];
			cAcHuffLength = new BYTE[HT_Bit_Table_Size];
			cAcHuffCode = new WORD[HT_Bit_Table_Size];
			file.read((char*)cAcHuffVal, HT_Bit_Table_Size);
			break;
		}
		case 0x12: {
			if (ac2Generated) {
				delete[] ac2HuffLength; delete[] ac2HuffCode; delete[] ac2HuffValue;
			}
			ac2Generated = true;
			file.read((char*)&dhtAc2, sizeof(DHT));
			HT_Bit_Table_Size = dhtAc2.htBitTableSum();
			ac2HuffValue = new BYTE[HT_Bit_Table_Size];
			ac2HuffLength = new BYTE[HT_Bit_Table_Size];
			ac2HuffCode = new WORD[HT_Bit_Table_Size];
			file.read((char*)ac2HuffValue, HT_Bit_Table_Size);
			break;
		}
		case 0x13: {
			if (ac3Generated) {
				delete[] ac3HuffLength; delete[] ac3HuffCode; delete[] ac3HuffValue;
			}
			ac3Generated = true;
			file.read((char*)&dhtAc3, sizeof(DHT));
			HT_Bit_Table_Size = dhtAc3.htBitTableSum();
			ac3HuffValue = new BYTE[HT_Bit_Table_Size];
			ac3HuffLength = new BYTE[HT_Bit_Table_Size];
			ac3HuffCode = new WORD[HT_Bit_Table_Size];
			file.read((char*)ac3HuffValue, HT_Bit_Table_Size);
			break;
		}
		default: {
			throw "Too many Huffman tables";
		}
		}
		length = length - HT_Bit_Table_Size - (WORD)sizeof(DHT);
		dhtcount++;
	}

	return 0;
}
int JPEGDecode::readQuantizationTable() {
	int Dqt_Table_Size;
	BYTE lengthH;//段长度高字节
	BYTE lengthL;//段长度低字节
	file.read((char*)&lengthH, 1);
	file.read((char*)&lengthL, 1);
	WORD length = lengthH * 0x100 + lengthL - 2; //求段长度
	while (length) {
		file.read((char*)&dqt[dqtcount], sizeof(DQT));
		if (dqt[dqtcount].qtPrecision()) {
			throw "qtPrecision should be 8bit.";
		}
		Dqt_Table_Size = 64 * (dqt[dqtcount].qtPrecision() + 1);
		if (dqt[dqtcount].qtNumber() == 0) {
			yqt = new BYTE[Dqt_Table_Size];
			file.read((char*)yqt, Dqt_Table_Size);
		}
		else if (dqt[dqtcount].qtNumber() == 1) {
			cqt = new BYTE[Dqt_Table_Size];
			file.read((char*)cqt, Dqt_Table_Size);
		}
		length = length - Dqt_Table_Size - (WORD)sizeof(DQT);
		dqtcount++;
	}
	return 0;
}

int JPEGDecode::preRead()
{
	int soi, app0, dqt, sof0, sof2, dht, dri, sos, eoi, dnl, com;
	soi = app0 = dqt = sof0 = sof2 = dht = dri = sos = eoi = dnl = com = 0;
	int i = 0;
	BYTE btmp = 0xff;//比特数据暂存
	BYTE btmp_p = 0xff;//扫描时暂存前一位进行比较
	while (file.good()) {
		btmp_p = btmp;
		file.read((char*)&btmp, 1);
		if (btmp != 0xff && btmp_p == 0xff) {
			if (btmp)cout << hex << (int)btmp << ' ';
			switch (btmp) {
			case 0xd8:soi++; break;//SOI
			case 0xd9:eoi++; break;//EOI
			case 0xc0:sof0++; break;//SOF
			case 0xc2:sof2++; break;//SOF2
			case 0xc4:dht++; break;//DHT
			case 0xda:sos++; break;//SOS
			case 0xdb:dqt++; break;//DQT
			case 0xdd:dri++; break;//DRI
			case 0xe0:app0++; break;//APP0
			case 0xfe:com++; break;//COM
			case 0xdc:dnl++; break;//DNL
			default:break;
			}
			i++;
		}
	}

	cout << "\n------------------------预读取提示--------------------------" << endl;
	if (soi == 0) {
		throw "error: not JPEG file!";
	}
	if (dqt == 0 && dht == 0) {
		throw "error: not dct-based jpeg!";
	}
	cout << "------------------------------------------------------------" << endl;
	file.clear();
	file.seekg(0);
	return 0;
}
int JPEGDecode::otherSegments() {
	BYTE lengthH;//段长度高字节
	BYTE lengthL;//段长度低字节
	file.read((char*)&lengthH, 1);
	file.read((char*)&lengthL, 1);
	WORD length = lengthH * 0x100 + lengthL - 2;//求需要跳过的段长度
	file.seekg(length, ios::cur);
	return 0;
}

void JPEGDecode::show(fstream& file)//临时显示文件
{
	BYTE a;
	for (int i = 0; i < 400 && file.good(); i++) {
		file.read((char*)&a, 1);
		cout << hex << (short)a << " ";
		int j = (i + 1) % 20;
		if (!j)cout << endl;
	}
	cout << endl;
}

//显示图像主要数据
void JPEGDecode::showFileQtHt() {
	int j = 0;
	if (sof.numberOfColor != sos.numberOfColor) {
		cout << "颜色分量前后不一致！" << endl;
	}
	cout << dec << "width:" << sof.width() << " height:" << sof.height() << endl;

	cout << "QT:" << dqtcount << "个 HT:" << dhtcount << "个" << endl;
	cout << "颜色数（3即YCbCr）:" << (WORD)sof.numberOfColor << endl;
	for (int i = 0; i < sof.numberOfColor; i++) {
		cout << "颜色分量ID:" << (WORD)sofColor[i].colorID << " 水平采样因子" << (WORD)sofColor[i].horizontalSamplingCoefficient() << " 垂直采样因子" << (WORD)sofColor[i].verticalSamplingCoefficient() << endl;
		cout << " 使用的QT号:" << (WORD)sofColor[i].qtUsed;
		cout << " DC使用的HT号:" << (WORD)sosColor[i].dcHtTableUsed() << " AC使用的HT号:" << (WORD)sosColor[i].acHtTableUsed() << endl;
	}
#ifdef _VCRUNTIME_H
	cout << "------------------临时显示HT表权值--------------------" << endl;

	cout << "HTnumber:" << (WORD)dht[j].htNumber() << " htType(0=DC 1=AC):" << (WORD)dht[j].htType() << endl;
	for (int i = 0; i < 16; i++) {
		cout << (WORD)dht[j].htBitTable[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << dec << (WORD)yDcHuffLength[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		char str[17];
		_itoa_s(yDcHuffCode[i], str, 2);
		cout << str << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << hex << (WORD)yDcHuffVal[i] << ' ';
	}
	cout << endl;
	j = 1;
	cout << "HTnumber:" << (WORD)dht[j].htNumber() << " htType(0=DC 1=AC):" << (WORD)dht[j].htType() << endl;
	for (int i = 0; i < 16; i++) {
		cout << (WORD)dht[j].htBitTable[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << dec << (WORD)yAcHuffLength[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		char str[17];
		_itoa_s(yAcHuffCode[i], str, 2);
		cout << str << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << hex << (WORD)yAcHuffVal[i] << ' ';
	}
	cout << endl;
	j = 2;
	cout << "HTnumber:" << (WORD)dht[j].htNumber() << " htType(0=DC 1=AC):" << (WORD)dht[j].htType() << endl;
	for (int i = 0; i < 16; i++) {
		cout << (WORD)dht[j].htBitTable[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << dec << (WORD)cDcHuffLength[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		char str[17];
		_itoa_s(cDcHuffCode[i], str, 2);
		cout << str << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << hex << (WORD)cDcHuffVal[i] << ' ';
	}
	cout << endl;
	j = 3;
	cout << "HTnumber:" << (WORD)dht[j].htNumber() << " htType(0=DC 1=AC):" << (WORD)dht[j].htType() << endl;
	for (int i = 0; i < 16; i++) {
		cout << (WORD)dht[j].htBitTable[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << dec << (WORD)cAcHuffLength[i] << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		char str[17];
		_itoa_s(cAcHuffCode[i], str, 2);
		cout << str << ' ';
	}
	cout << endl;
	for (int i = 0; i < dht[j].htBitTableSum(); i++) {
		cout << hex << (WORD)cAcHuffVal[i] << ' ';
	}
	cout << endl;
#endif
	cout << "------------------临时显示QT表-------------------------" << endl;
	j = 0;
	cout << "QTnumber:" << (WORD)dqt[j].qtNumber() << " QT精度:" << (WORD)dqt[j].qtPrecision() << endl;
	for (int i = 0; i < 64 * (dqt[j].qtPrecision() + 1); i++) {
		cout << dec << (WORD)yqt[i] << ' ';
		if ((i + 1) % 8 == 0)cout << endl;
	}
	cout << endl;
	j = 1;
	cout << "QTnumber:" << (WORD)dqt[j].qtNumber() << " QT精度:" << (WORD)dqt[j].qtPrecision() << endl;
	for (int i = 0; i < 64 * (dqt[j].qtPrecision() + 1); i++) {
		cout << dec << (WORD)cqt[i] << ' ';
		if ((i + 1) % 8 == 0)cout << endl;
	}
	cout << endl;
}

#ifdef ENCRYPT
void JPEGDecodeDecrypt::decrypt(SWORD(*yMcu)[64], SWORD(*cbMcu)[64], SWORD(*crMcu)[64], int numMCU) {
	double x = 1.5121615730212, y = 12.102462138545, z = -9.4525140151405, u = 0.49161415934378;
	double lx = 0.63726194247947;
	const int ROUND = 3;
	SWORD(*mcu)[64] = new SWORD[numMCU * 3][64];
	memcpy(mcu, yMcu, 64 * numMCU * sizeof(SWORD));
	memcpy(mcu + numMCU, cbMcu, 64 * numMCU * sizeof(SWORD));
	memcpy(mcu + 2 * numMCU, crMcu, 64 * numMCU * sizeof(SWORD));
	for (int round = 0; round < ROUND; round++) {
		//dc decrypt
		HyperChaoticLvSystem lvDC(x, y, z, u);
		LogisticMap logisticDC(lx);
		std::function<void(SWORD(*)[64], int, ChaoticMap&, ChaoticMap&)>
			decryptDCFunc(std::bind(&JPEGDecodeDecrypt::decryptDCScramble, this, std::placeholders::_1,
				std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		thread th(decryptDCFunc, mcu, 3 * numMCU, std::ref(lvDC), std::ref(logisticDC));

		//ac decrypt
		HyperChaoticLvSystem lvYAC(x, y, z, u);
		//HyperChaoticLvSystem lvCAC(lvYAC);
		LogisticMap logisticYAC(lx);
		//LogisticMap logisticCAC(logisticYAC);
		//std::function<void(SWORD(*)[64], int, ChaoticMap&, ChaoticMap&)>
		//	func(std::bind(&JPEGDecodeDecrypt::decryptAC, this, std::placeholders::_1,
		//		std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		//thread th1 = thread(func, mcu, numMCU, std::ref(lvYAC), std::ref(logisticYAC));
		//thread th2 = thread(func, mcu + numMCU, 2 * numMCU, std::ref(lvCAC), std::ref(logisticCAC));
		decryptAC(mcu, 3 * numMCU, lvYAC, logisticYAC);

		//synchronous: wait for threads
		th.join();
		//th1.join();
		//th2.join();

		//recover block scramble 
		HyperChaoticLvSystem lvBlockY(x, y, z, u);
		//HyperChaoticLvSystem lvBlockC(lvBlockY);
		decryptBlockScrambling(mcu, 3 * numMCU, lvBlockY);
		//auto decryptBlockScramblingFunc(std::bind(&JPEGDecodeDecrypt::decryptBlockScrambling, this,
		//	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		//thread th3(decryptBlockScramblingFunc, mcu, numMCU, std::ref(lvBlockY));
		//decryptBlockScrambling(mcu + numMCU, 2 * numMCU, lvBlockC);
		//th3.join();
	}
	memcpy(yMcu, mcu, 64 * numMCU * sizeof(SWORD));
	memcpy(cbMcu, mcu + numMCU, 64 * numMCU * sizeof(SWORD));
	memcpy(crMcu, mcu + numMCU * 2, 64 * numMCU * sizeof(SWORD));
	delete[] mcu;
}
/*
void JPEGDecode::decryptComputeDiff(SWORD(*mcu)[64], int numMCU) {
//计算差分值
int pred = mcu[0][0];
for (int i = 1; i < numMCU; i++) {
int currentDCValue = mcu[i][0];
mcu[i][0] = currentDCValue - pred;
pred = currentDCValue;
}
}
void JPEGDecode::decryptReComputeDiff(SWORD(*mcu)[64], int numMCU) {
//差分值还原
int pred = mcu[0][0];
for (int i = 1; i < numMCU; i++) {
pred = mcu[i][0] + pred;
mcu[i][0] = pred;
}
}
*/
void JPEGDecodeDecrypt::decryptBlockScrambling(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation) {
	//块置乱
	int *keyStream = new int[numMCU];
	int *keyStreamPointer = keyStream + numMCU - 1;
	SWORD *mcuPointer = mcu[numMCU - 1];
	SWORD dataTemp[64];
	for (int i = 0; i < numMCU; i++) {
		keyStream[i] = ((long long)((permutation.keySequence())*1.0e+14) % (numMCU - i));
	}
	for (int i = 0; i < numMCU; i++) {
		memcpy(dataTemp, mcuPointer, sizeof(SWORD) * 64);
		memcpy(mcuPointer, mcuPointer + 64 * *keyStreamPointer, sizeof(SWORD) * 64);
		memcpy(mcuPointer + 64 * *keyStreamPointer, dataTemp, sizeof(SWORD) * 64);
		mcuPointer -= 64; keyStreamPointer--;
	}
	delete[] keyStream;
}
void JPEGDecodeDecrypt::decryptDCScramble(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap &substitution) {
	//DC scramble
	//取出DC系数
	SWORD *dc = new SWORD[numMCU];
	SWORD *dcPointer = dc;
	SWORD *mcuPointer = mcu[0];
	for (int i = 0; i < numMCU; i++) {
		*dcPointer = *mcuPointer;
		dcPointer++;
		mcuPointer += 64;
	}

	//反扩散
	//int pred = 0;
	char lastBit = 0;
	char lastNegative = 0;
	dcPointer = dc;
	for (int i = 0; i < numMCU; i++) {
		if (*dcPointer == 0 || *dcPointer == -1024) {
			dcPointer++;
			continue;
		}
		//???????
		SWORD absDC = abs(*dcPointer);
		char  scrambledNegative = *dcPointer < 0 ? 1 : 0;
		vector<char>dcBits;
		dcBits.reserve(12);
		while (absDC) {
			dcBits.push_back(absDC & 1);
			absDC >>= 1;
		}
		//首先展开
		char negative = 0;
		int k = ((long long)((substitution.keySequence())*1.0e+14) & 1);
		negative = lastNegative ^ scrambledNegative ^ k;
		lastNegative = scrambledNegative;
		//结合扩散，进行DC值扰乱		
		vector<char>scrambledBits;
		scrambledBits.reserve(12);
		vector<char>::iterator iterator;
		for (iterator = dcBits.begin(); iterator != dcBits.end() - 1; ++iterator) {
			char cipherBit = *iterator;
			int k = (long long)((substitution.keySequence())*1.0e+14) & 1;
			*iterator = lastBit ^ *iterator ^ k;
			scrambledBits.push_back(*iterator);
			lastBit = cipherBit;
		}
		scrambledBits.push_back(*iterator);

		//写回、测试
		SWORD scrambled = 0;
		for (vector<char>::reverse_iterator iterator = scrambledBits.rbegin(); iterator != scrambledBits.rend(); ++iterator) {
			scrambled <<= 1;
			scrambled += *iterator;
		}
		if (negative)scrambled = -scrambled;
		*dcPointer = scrambled;
		dcPointer++;
	}

	//反置乱
	int *keyStream = new int[numMCU];
	int *keyStreamPointer = keyStream + numMCU - 1;
	dcPointer = dc + numMCU - 1;
	SWORD dataTemp;
	for (int i = 0; i < numMCU; i++) {
		keyStream[i] = ((long long)((permutation.keySequence())*1.0e+14) % (numMCU - i));
	}
	for (int i = 0; i < numMCU; i++) {
		dataTemp = *dcPointer;
		*dcPointer = *(dcPointer + *keyStreamPointer);
		*(dcPointer + *keyStreamPointer) = dataTemp;
		dcPointer--; keyStreamPointer--;
	}
	delete[] keyStream;

	//写回
	dcPointer = dc;
	mcuPointer = mcu[0];
	for (int i = 0; i < numMCU; i++) {
		*mcuPointer = *dcPointer;
		dcPointer++;
		mcuPointer += 64;
	}

	delete[] dc;
}
void JPEGDecodeDecrypt::decryptAC(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap &substitution) {
	//对AC系数进行内部置乱、扩散
	//mcu内部置乱：按照编码时RRRRSSSS+附加值的组合
	//非0值扩散：参考DC扩散
	WORD previous = 0;
	char previousNegative = 0;

	for (int j = 0; j < numMCU; j++) {
		vector<PartData> mcuData;
		mcuData.reserve(32);
		PartData partData;
		int zrlCount = 0;
		int zeroCount = 0;
		bool hasZrl = false;
		BYTE end0Pos;//最后一个非0AC系数位置
		for (end0Pos = 63; end0Pos > 0 && mcu[j][end0Pos] == 0; end0Pos--);
		for (int i = 1; i <= end0Pos; i++) {
			partData.data[zeroCount] = mcu[j][i];//记录数据
			if (mcu[j][i] == 0) {
				//为0先计数，再判断ZRL
				zeroCount++;
				if (zeroCount == 16) {
					zrlCount++;
					partData.size = zeroCount;
					mcuData.push_back(partData);//压栈
					partData.clear();//重置
					zeroCount = 0;
					hasZrl = true;
				}
			}
			else {
				zeroCount++;
				partData.size = zeroCount;
				mcuData.push_back(partData);//压栈
				partData.clear();//重置
				zeroCount = 0;
			}
		}

		//反扩散
		vector<PartData>::iterator it;
		for (it = mcuData.begin(); it != mcuData.end(); ++it) {
			if (it->isZRL()) {
			//	++it;
				continue;
			}
			//AC没有0数据
			//首先取绝对值
			SWORD data = it->data[it->size - 1];
			WORD absData = abs(data);
			char scrambledNegative = data < 0 ? 1 : 0;
			unsigned mask = 0;
			unsigned digit = 0;
			for (WORD temp = absData; temp; temp >>= 1) {
				digit++;
				mask <<= 1;
				mask++;
			}
			//正负号处理
			char negative = 0;
			char key = (char)((long long)(substitution.keySequence()*1.0e+14) & 1);
			negative = previousNegative ^ scrambledNegative ^ key;
			previousNegative = scrambledNegative;
			//结合扩散，进行DC值扰乱
			mask >>= 1;//首位1不动，保位数
			if (digit > 2) {
				int key = (long long)((substitution.keySequence())*1.0e+14) & mask;
				unsigned temp = absData;
				absData = (((key ^ absData ^ (previous & mask)) + mask + 1 - key) & mask)
						| (1 << (digit - 1));
				previous = temp;
			}
			else if (digit > 1) {
				int key = (long long)((substitution.keySequence())*1.0e+14) & mask;
				unsigned temp = absData;
				absData = ((key ^ absData ^ (previous & mask)) & mask) | (1 << (digit - 1));
				previous = temp;
			}
			//写回
			it->data[it->size - 1] = negative ? -absData : absData;
		}

		//反置乱
		int size = (int) mcuData.size();
		if (size) {
			vector<int> zrlPos;
			if (hasZrl) {
				size = size * 2 - 1 - zrlCount;
			}
			double *keyStream = new double[size];
			double *keyStreamPointer = keyStream + size - 1;
			for (int i = 0; i < size; i++) {
				keyStream[i] = permutation.keySequence();
			}
			if (hasZrl) {
				//反置乱ZRL
				it = mcuData.end();
				--it;
				size = (int) mcuData.size() - 1;
				for (int i = size - 1; i >= 0; i--) {
					--it;
					int keyStream = ((long long) ((*keyStreamPointer) * 1.0e+14)
							% (size - i));
					vector<PartData>::iterator itDst = it + keyStream;
					//for (int i = 0; i < keyStream; i++)++itDst;
					partData = *it;
					*it = *itDst;			// *(it + keyStream);
					*itDst = partData;
					keyStreamPointer--;
				}
				//去除ZRL
				it = mcuData.begin();
				int pos = 0;
				while (it != mcuData.end()) {
					if (it->isZRL()) {
						//记录坐标 删除数据
						zrlPos.push_back(pos);
						mcuData.erase(it);
						//重置
						pos = 0;
						it = mcuData.begin();
						continue;
					}
					pos++;
					++it;
				}
			}
			//反置乱非零ac整体
			it = mcuData.end();
			size = (int) mcuData.size();
			for (int i = size - 1; i >= 0; i--) {
				--it;
				int keyStream = ((long long) ((*keyStreamPointer) * 1.0e+14)
						% (size - i));
				vector<PartData>::iterator itDst = it + keyStream;
				//for (int i = 0; i < keyStream; i++)++itDst;
				partData = *it;
				*it = *itDst;			// *(it + keyStream);
				*itDst = partData;
				keyStreamPointer--;
			}
			if (hasZrl) {
				//加入ZRL
				PartData zrl;
				zrl.size = 16;
				for (vector<int>::reverse_iterator riterator = zrlPos.rbegin();
						riterator != zrlPos.rend(); ++riterator) {
					mcuData.insert(mcuData.begin() + *riterator, zrl);
				}
			}
			//反置乱收尾
			delete[] keyStream;
		}

		//写回
		int i = 1;
		for (it = mcuData.begin(); it != mcuData.end(); ++it) {
			for (int k = 0; k < it->size; k++) {
				mcu[j][i++] = it->data[k];
			}
		}
	}
}

#endif

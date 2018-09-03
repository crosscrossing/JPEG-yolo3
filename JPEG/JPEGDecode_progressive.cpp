#include"JPEGDecode.h"
int JPEGDecode::decodeProgressiveMain() {
	width = sof.width();
	height = sof.height();
	int x_pos, y_pos;
	int x_pos_max, y_pos_max;
	int Y_EOBRUN = 0, Cb_EOBRUN = 0, Cr_EOBRUN = 0;
	int *EOBRUN = NULL;
	int numMCU, numChroMCU;
	SWORD(*yZZ)[64], (*cbZZ)[64], (*crZZ)[64];
	SWORD(*yIDCT)[64], (*cbIDCT)[64], (*crIDCT)[64];
	SWORD(*yDeQT)[64], (*cbDeQT)[64], (*crDeQT)[64];
	float(*yMCU)[64], (*cbMCU)[64], (*crMCU)[64];
	BYTE(*rMCU)[64], (*gMCU)[64], (*bMCU)[64];

	if (sofColor[0].samplingCoefficient ^ 17)throw"采样方式暂不支持";
	x_pos_max = sofColor[0].samplingCoefficient & 0x20 ? (int)ceil((float)width / 16) * 2 - 1 : (int)ceil((float)width / 8) - 1;
	y_pos_max = sofColor[0].samplingCoefficient & 0x02 ? (int)ceil((float)height / 16) * 2 - 1 : (int)ceil((float)height / 8) - 1;
	numMCU = (x_pos_max + 1)*(y_pos_max + 1);//mcu总数
	numChroMCU = numMCU;
	if (sofColor[0].samplingCoefficient & 0x20)numChroMCU /= 2;
	if (sofColor[0].samplingCoefficient & 0x02)numChroMCU /= 2;
	//问题：一次扫描中有多个颜色分量则按照顺序方式处理，只有一个分量则按其自身顺序处理
	yDeQT = new SWORD[numMCU][64];
	if (sof.numberOfColor == 3) {
		cbDeQT = new SWORD[numChroMCU][64];
		crDeQT = new SWORD[numChroMCU][64];
	}
	while (1) {
		BYTE Ah = sA / 16;
		BYTE Al = sA % 16;
		if (!Ah) {//successive approximation high = 0 说明是first scan 或者是sprectral selection
				  //判断分量使用的是哪个哈夫曼表 DC没问题 AC2和3需要判断
			if (ss == 0 && se == 0) {// scan of DC coefficients	
				generateHuffmanTable(dht[0], yDcHuffLength, yDcHuffCode, yDcHuffVal, yDcDehuff);
				if (sof.numberOfColor == 3) {//YCbCr彩色
					generateHuffmanTable(dht[2], cDcHuffLength, cDcHuffCode, cDcHuffVal, cDcDehuff);
				}
				DEHUFF *YDCTableUsed, *CDCTableUsed;
				YDCTableUsed = sosColor[0].dcHtTableUsed() == 0 ? yDcDehuff : cDcDehuff;
				if (sof.numberOfColor == 3) {
					if (sosColor[1].dcHtTableUsed() != sosColor[2].dcHtTableUsed()) throw"sosColor[1]和[2]不相同";
					CDCTableUsed = sosColor[1].dcHtTableUsed() == 0 ? yDcDehuff : cDcDehuff;//sosColor[1]和[2]应该相同
				}
				for (int i = 0; i < numMCU; i++) {
					readDCProgressiveFirstscan(YDCTableUsed, yDiff, yPred, Ah, Al, yDeQT[i]);
					if (sofColor[0].samplingCoefficient & 0x20) readDCProgressiveFirstscan(YDCTableUsed, yDiff, yPred, Ah, Al, yDeQT[++i]);
					if (sofColor[0].samplingCoefficient & 0x02) readDCProgressiveFirstscan(YDCTableUsed, yDiff, yPred, Ah, Al, yDeQT[++i]);
					if (!(sofColor[0].samplingCoefficient ^ 0x22)) readDCProgressiveFirstscan(YDCTableUsed, yDiff, yPred, Ah, Al, yDeQT[++i]);
					if (sof.numberOfColor == 3) {
						int j = i;
						if (sofColor[0].samplingCoefficient & 0x20)j /= 2;
						if (sofColor[0].samplingCoefficient & 0x02)j /= 2;
						readDCProgressiveFirstscan(CDCTableUsed, cbDiff, cbPred, Ah, Al, cbDeQT[j]);
						readDCProgressiveFirstscan(CDCTableUsed, crDiff, crPred, Ah, Al, crDeQT[j]);
					}
				}
				delete[] yDcDehuff; delete[] cDcDehuff;
				yDcDehuff = new DEHUFF[65536]; cDcDehuff = new DEHUFF[65536];
			}
			else if (ss == 0 && se != 0) {
				throw "first scan error";
			}
			else {
				DEHUFF *dehuff;
				SWORD *blockPointer;
				//AC一般各个颜色分开保存
				switch (sosColor[0].acHtTableUsed()) {
				case 0: {
					generateHuffmanTable(dht[1], yAcHuffLength, yAcHuffCode, yAcHuffVal, yAcDehuff);
					dehuff = yAcDehuff;
					break;
				}
				case 1: {
					generateHuffmanTable(dht[3], cAcHuffLength, cAcHuffCode, cAcHuffVal, cAcDehuff);
					dehuff = cAcDehuff;
					break;
				}
				case 2: {
					dehuff = new DEHUFF[65536];
					generateHuffmanTable(dhtAc2, ac2HuffLength, ac2HuffCode, ac2HuffValue, dehuff);
					break;
				}
				case 3: {
					dehuff = new DEHUFF[65536];
					generateHuffmanTable(dhtAc3, ac3HuffLength, ac3HuffCode, ac3HuffValue, dehuff);
					break;
				}
				default:
					throw "acHtTableUsed error";
				}
				switch (sosColor[0].colorID) {
				case 1: blockPointer = *yDeQT; EOBRUN = &Y_EOBRUN; break;
				case 2: blockPointer = *cbDeQT; EOBRUN = &Cb_EOBRUN; break;
				case 3: blockPointer = *crDeQT; EOBRUN = &Cr_EOBRUN; break;
				}
				if (sosColor[0].colorID == 1) {
					for (int i = 0; i < numMCU; i++) {
						readACProgressiveFirstscan(dehuff, ss, se, Al, *EOBRUN, blockPointer);
						blockPointer += 64;
					}
				}
				else {
					for (int i = 0; i < numChroMCU; i++) {
						readACProgressiveFirstscan(dehuff, ss, se, Al, *EOBRUN, blockPointer);
						blockPointer += 64;
					}
				}
				switch (sosColor[0].acHtTableUsed()) {
				case 0:
					delete[] yAcDehuff;
					yAcDehuff = new DEHUFF[65536];
					break;
				case 1:
					delete[] cAcDehuff;
					cAcDehuff = new DEHUFF[65536];
					break;
				case 2:case 3:
					delete[]dehuff;
					break;
				}
			}
		}
		else {
			if (ss == 0 && se == 0) {// scan of DC coefficients
				if (sof.numberOfColor == 3) {
					readDCProgressiveSubsequentScans(Ah, Al, yDeQT, cbDeQT, crDeQT, numMCU);
				}
				else {
					readDCProgressiveSubsequentScans(Ah, Al, yDeQT, numMCU);
				}
			}
			else {
				int numMCUTemp;
				DEHUFF *dehuff;
				SWORD **blockPointer;
				switch (sosColor[0].colorID) {
				case 1: blockPointer = (SWORD**)yDeQT; EOBRUN = &Y_EOBRUN; numMCUTemp = numMCU; break;
				case 2: blockPointer = (SWORD**)cbDeQT; EOBRUN = &Cb_EOBRUN; numMCUTemp = numChroMCU; break;
				case 3: blockPointer = (SWORD**)crDeQT; EOBRUN = &Cr_EOBRUN; numMCUTemp = numChroMCU; break;
				}

				//AC一般各个颜色分开保存
				switch (sosColor[0].acHtTableUsed()) {
				case 0: {
					generateHuffmanTable(dht[1], yAcHuffLength, yAcHuffCode, yAcHuffVal, yAcDehuff);
					dehuff = yAcDehuff;
					readACProgressiveSubsequentScans(dehuff, ss, se, Ah, Al, (SWORD(*)[64])blockPointer, numMCUTemp);
					delete[] yAcDehuff;
					yAcDehuff = new DEHUFF[65536];
					break;
				}
				case 1: {
					generateHuffmanTable(dht[3], cAcHuffLength, cAcHuffCode, cAcHuffVal, cAcDehuff);
					dehuff = cAcDehuff;
					readACProgressiveSubsequentScans(dehuff, ss, se, Ah, Al, (SWORD(*)[64])blockPointer, numMCUTemp);
					delete[] cAcDehuff;
					cAcDehuff = new DEHUFF[65536];
					break;
				}
				case 2: {
					dehuff = new DEHUFF[65536];
					generateHuffmanTable(dhtAc2, ac2HuffLength, ac2HuffCode, ac2HuffValue, dehuff);
					readACProgressiveSubsequentScans(dehuff, ss, se, Ah, Al, (SWORD(*)[64])blockPointer, numMCUTemp);
					delete[]dehuff;
					break;
				}
				case 3: {
					dehuff = new DEHUFF[65536];
					generateHuffmanTable(dhtAc3, ac3HuffLength, ac3HuffCode, ac3HuffValue, dehuff);
					readACProgressiveSubsequentScans(dehuff, ss, se, Ah, Al, (SWORD(*)[64])blockPointer, numMCUTemp);
					delete[]dehuff;
					break;
				}
				default:
					throw "acHtTableUsed error";
				}
			}
		}
		bytePos = 7;
		delete[] sosColor;
		if (readSegments())break;
	}

	if (sof.numberOfColor == 3) {
		r = new BYTE[width*height];
		g = new BYTE[width*height];
		b = new BYTE[width*height];
	}
	else {
		y = new BYTE[width*height];
	}
	switch (sofColor[0].samplingCoefficient) {
	case 17: {
		if(sof.numberOfColor == 3){
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

			yMCU = new float[numMCU][64];
			cbMCU = new float[numMCU][64];
			crMCU = new float[numMCU][64];
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

			yMCU = new float[numMCU][64];
			for (int i = 0; i < numMCU; i++) {
				idct(yIDCT[i], yMCU[i]);
			}
			delete[] yIDCT;
		}//end else
		break;
	}//end case 17
	case 34: {
		yZZ = new SWORD[numMCU][64];
		cbZZ = new SWORD[numMCU / 4][64];
		crZZ = new SWORD[numMCU / 4][64];
		for (int i = 0; i < numMCU / 4; i++) {
			for (int j = 4 * i; j < 4 * i + 4; j++) dequantization(yDeQT[j], yZZ[j], yqt);
			dequantization(cbDeQT[i], cbZZ[i], cqt);
			dequantization(crDeQT[i], crZZ[i], cqt);
		}
		delete[] yDeQT; delete[] cbDeQT; delete[] crDeQT;

		yIDCT = new SWORD[numMCU][64];
		cbIDCT = new SWORD[numMCU / 4][64];
		crIDCT = new SWORD[numMCU / 4][64];
		for (int i = 0; i < numMCU / 4; i++) {
			for (int j = 4 * i; j < 4 * i + 4; j++) reverseZZ(yZZ[j], yIDCT[j]);
			reverseZZ(cbZZ[i], cbIDCT[i]);
			reverseZZ(crZZ[i], crIDCT[i]);
		}
		delete[] yZZ; delete[] cbZZ; delete[] crZZ;

		yMCU = new float[numMCU][64];
		cbMCU = new float[numMCU / 4][64];
		crMCU = new float[numMCU / 4][64];
		for (int i = 0; i < numMCU / 4; i++) {
			for (int j = 4 * i; j < 4 * i + 4; j++) {

				idct(yIDCT[j], yMCU[j]);
			}
			idct(cbIDCT[i], cbMCU[i]);
			idct(crIDCT[i], crMCU[i]);
		}
		delete[] yIDCT; delete[] cbIDCT; delete[] crIDCT;

		float **Y_MCUExpanded = new float *[numMCU];
		float(*Cb_MCUExpanded)[64] = new float[numMCU][64];
		float(*Cr_MCUExpanded)[64] = new float[numMCU][64];
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
		int j = 0, k = 0, n = 0;
		int kStart = k;
		int pos;
		int posExpand;
		for (y_pos = 0; y_pos <= y_pos_max; y_pos++) {
			for (x_pos = 0; x_pos <= x_pos_max; x_pos++) {
				Y_MCUExpanded[j++] = yMCU[k];
				x_pos & 1 ? k += 3 : k++;
				n = (x_pos & 0x1) + (y_pos & 0x1) * 2;
				pos = x_pos / 2 + (y_pos / 2)*(int)ceil((float)(x_pos_max + 1) / 2);
				posExpand = x_pos + y_pos*(x_pos_max + 1);
				for (int loop = 0; loop < 64; loop++) {
					Cb_MCUExpanded[posExpand][loop] = cbMCU[pos][expand[n][loop]];
					Cr_MCUExpanded[posExpand][loop] = crMCU[pos][expand[n][loop]];
				}
			}
			y_pos & 1 ? kStart += (x_pos_max + 1) * 2 - 2 : kStart += 2;
			k = kStart;
		}

		bool xExceed = false;
		bool exceed = false;
		if (width % 16 && width % 16 <= 8) {
			exceed = true;
			xExceed = true;
			x_pos_max = (int)ceil((float)width / 8) - 1;
		}
		if (height % 16 && height % 16 <= 8) {
			exceed = true;
			y_pos_max = (int)ceil((float)height / 8) - 1;
		}
		exceed ? numMCU = (x_pos_max + 1)*(y_pos_max + 1) : false;

		rMCU = new BYTE[numMCU][64];
		gMCU = new BYTE[numMCU][64];
		bMCU = new BYTE[numMCU][64];
		int m = 0;
		x_pos = 0;
		for (int i = 0; i < numMCU; i++, m++) {
			convertToRgb(rMCU[i], gMCU[i], bMCU[i], Y_MCUExpanded[m], Cb_MCUExpanded[m], Cr_MCUExpanded[m]);
			if (xExceed) {
				x_pos++;
				if (x_pos > x_pos_max) {
					x_pos = 0;
					m++;
				}
			}
		}
		delete[] yMCU; delete[] cbMCU; delete[] crMCU;
		delete[] Y_MCUExpanded; delete[] Cb_MCUExpanded; delete[] Cr_MCUExpanded;
		break;
	}//end case 34
	case 0x21:case 0x12: {
		yZZ = new SWORD[numMCU][64];
		cbZZ = new SWORD[numMCU / 2][64];
		crZZ = new SWORD[numMCU / 2][64];
		for (int i = 0; i < numMCU / 2; i++) {
			for (int j = 2 * i; j < 2 * i + 2; j++) dequantization(yDeQT[j], yZZ[j], yqt);
			dequantization(cbDeQT[i], cbZZ[i], cqt);
			dequantization(crDeQT[i], crZZ[i], cqt);
		}
		delete[] yDeQT; delete[] cbDeQT; delete[] crDeQT;

		yIDCT = new SWORD[numMCU][64];
		cbIDCT = new SWORD[numMCU / 2][64];
		crIDCT = new SWORD[numMCU / 2][64];
		for (int i = 0; i < numMCU / 2; i++) {
			for (int j = 2 * i; j < 2 * i + 2; j++) reverseZZ(yZZ[j], yIDCT[j]);
			reverseZZ(cbZZ[i], cbIDCT[i]);
			reverseZZ(crZZ[i], crIDCT[i]);
		}
		delete[] yZZ; delete[] cbZZ; delete[] crZZ;

		yMCU = new float[numMCU][64];
		cbMCU = new float[numMCU / 2][64];
		crMCU = new float[numMCU / 2][64];
		for (int i = 0; i < numMCU / 2; i++) {
			for (int j = 2 * i; j < 2 * i + 2; j++) idct(yIDCT[j], yMCU[j]);
			idct(cbIDCT[i], cbMCU[i]);
			idct(crIDCT[i], crMCU[i]);
		}
		delete[] yIDCT; delete[] cbIDCT; delete[] crIDCT;

		rMCU = new BYTE[numMCU][64];
		gMCU = new BYTE[numMCU][64];
		bMCU = new BYTE[numMCU][64];
		float **Y_MCUExpanded;
		float(*Cb_MCUExpanded)[64] = new float[numMCU][64];
		float(*Cr_MCUExpanded)[64] = new float[numMCU][64];
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
		if (sofColor[0].samplingCoefficient == 0x21) {
			for (y_pos = 0; y_pos <= y_pos_max; y_pos++) {
				for (x_pos = 0; x_pos <= x_pos_max; x_pos++) {
					int n = x_pos & 0x1;
					int pos = x_pos / 2 + y_pos*(int)ceil((float)(x_pos_max + 1) / 2);
					int posExpand = x_pos + y_pos*(x_pos_max + 1);
					for (int loop = 0; loop < 64; loop++) {
						Cb_MCUExpanded[posExpand][loop] = cbMCU[pos][expandHorizontal[n][loop]];
						Cr_MCUExpanded[posExpand][loop] = crMCU[pos][expandHorizontal[n][loop]];
					}
				}
			}
		}
		else {
			Y_MCUExpanded = new float *[numMCU];
			int j = 0; int k = 0;
			int kStart = k;
			for (y_pos = 0; y_pos <= y_pos_max; y_pos++) {
				for (x_pos = 0; x_pos <= x_pos_max; x_pos++) {
					Y_MCUExpanded[j++] = yMCU[k];
					k += 2;
					int n = y_pos & 0x1;
					int pos = x_pos + (y_pos / 2)*(x_pos_max + 1);
					int posExpand = x_pos + y_pos*(x_pos_max + 1);
					for (int loop = 0; loop < 64; loop++) {
						Cb_MCUExpanded[posExpand][loop] = cbMCU[pos][expandVertical[n][loop]];
						Cr_MCUExpanded[posExpand][loop] = crMCU[pos][expandVertical[n][loop]];
					}
				}
				y_pos & 1 ? kStart += (x_pos_max + 1) * 2 - 1 : kStart++;
				k = kStart;
			}
		}

		bool xExceed = false;
		bool exceed = false;
		if (sofColor[0].samplingCoefficient == 0x21 && width % 16 && width % 16 <= 8) {
			exceed = true;
			xExceed = true;
			x_pos_max = (int)ceil((float)width / 8) - 1;
		}
		if (sofColor[0].samplingCoefficient == 0x12 && height % 16 && height % 16 <= 8) {
			exceed = true;
			y_pos_max = (int)ceil((float)height / 8) - 1;
		}
		exceed ? numMCU = (x_pos_max + 1)*(y_pos_max + 1) : false;

		rMCU = new BYTE[numMCU][64];
		gMCU = new BYTE[numMCU][64];
		bMCU = new BYTE[numMCU][64];
		int k = 0;
		x_pos = 0;
		for (int i = 0; i < numMCU; i++, k++) {
			sofColor[0].samplingCoefficient == 0x21 ?
				convertToRgb(rMCU[i], gMCU[i], bMCU[i], yMCU[k], Cb_MCUExpanded[k], Cr_MCUExpanded[k]) :
				convertToRgb(rMCU[i], gMCU[i], bMCU[i], Y_MCUExpanded[k], Cb_MCUExpanded[k], Cr_MCUExpanded[k]);
			if (xExceed) {
				x_pos++;
				if (x_pos > x_pos_max) {
					x_pos = 0;
					k++;
				}
			}
		}
		delete[] yMCU; delete[] cbMCU; delete[] crMCU;
		delete[] Cb_MCUExpanded; delete[] Cr_MCUExpanded;
		if (sofColor[0].samplingCoefficient == 0x21) delete[] Y_MCUExpanded;
		break;
	}
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

int JPEGDecode::readDCProgressiveFirstscan(DEHUFF*DEHUFF, SWORD&diff, SWORD&pred, const BYTE Ah, const BYTE Al, SWORD*result) {
	WORD code;
	BYTE offset;//当前的位数
	BYTE value;
	int point_trans = (int)pow(2, Al);//点变换
	//查找哈夫曼表
	for (offset = 1, code = 0; 1; offset++) {
		code += readOneBit();
		if (DEHUFF[code].length == offset) {
			value = DEHUFF[code].value;
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
	if (Al)diff *= point_trans;//根据Al左移
	result[0] = diff + pred;
	pred += diff;
	return 0;
}
int JPEGDecode::readACProgressiveFirstscan(DEHUFF*DEHUFF, const BYTE ss, const BYTE se, const BYTE Al, int&EOBRUN, SWORD*result) {
	if (EOBRUN != 0) {
		for (unsigned char k = ss; k <= se; k++) {
			result[k] = 0;
		}
		EOBRUN--;
		return 1;//其余不执行
	}
	WORD code;
	BYTE offset;//当前的位数
	BYTE value;
	SWORD digit = 0;//ac位数
	char zero_run_length = 0;//连0游程编码
	SWORD ac_value = 0;//结果
	int point_trans = 1 << Al;// (int)pow(2, Al);//点变换
	for (unsigned char k = ss; k <= se; k++) {
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
			for (char i = 0; i < 16; i++) {
				result[k] = 0;
				k++;
			}
			k--;
			continue;
		}
		if (value == 0) {//EOB0
			for (char i = se - k; i >= 0; i--) {
				result[k] = 0;
				k++;
			}
			break;
		}
		else if (value % 16 == 0) {//EOB1-14
			EOBRUN = 1;
			digit = value / 16;
			for (char i = 0; i < digit; i++) {
				EOBRUN <<= 1;
				EOBRUN += readOneBit();
			}
			for (char i = se - k; i >= 0; i--) {
				result[k] = 0;
				k++;
			}
			EOBRUN--;
			break;
		}
		zero_run_length = value / 16;
		digit = value % 16;
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
		if (Al) ac_value *= point_trans;
		result[k] = ac_value;
	}
	return 0;
}
int JPEGDecode::readACProgressiveSubsequentScans(DEHUFF*DEHUFF, const BYTE ss, const BYTE se, const BYTE Ah, const BYTE Al, SWORD(*result)[64], int MCU_sum) {
	WORD code;
	BYTE offset;//当前的位数
	BYTE value;
	SWORD digit = 0;//ac位数
	char zero_run_length = 0;//连0游程编码
	SWORD ac_value = 0;//结果
	int point_trans = 1 << Al;// (int)pow(2, Al);//点变换
	int EOBRUN = 0;
	int cur_MCU = 0;
	struct LinkList {
		int which_block;
		int which_byte;
		struct LinkList  *next;
		LinkList() {
			which_block = which_byte = 0;
			next = NULL;
		}
	};
	LinkList *node, *head;
	while (cur_MCU < MCU_sum) {
		for (unsigned char k = ss; k <= se; k++) {
			//	head = node = (LinkList*)malloc(sizeof(LinkList));
			head = node = new LinkList;

			//查找哈夫曼表
			for (offset = 1, code = 0; 1; offset++) {
				code += readOneBit();
				if (DEHUFF[code].length == offset) {
					value = DEHUFF[code].value;
					break;
				}
				code <<= 1;
			}
			if (value % 16 == 0 && value != 0xF0) {
				if (value == 0) {//EOB0
					for (; k <= se; k++) {
						while (result[cur_MCU][k] != 0 && k <= se) {
							node->which_block = cur_MCU;
							node->which_byte = k;
							node = node->next = new LinkList;
							k++;
						}
						if (k <= se) result[cur_MCU][k] = 0;
					}
				}
				else {//EOB1-14
					EOBRUN = 1;
					digit = value / 16;
					for (char i = 0; i < digit; i++) {
						EOBRUN <<= 1;
						EOBRUN += readOneBit();
					}
					for (; k <= se; k++) {
						while (result[cur_MCU][k] != 0 && k <= se) {
							node->which_block = cur_MCU;
							node->which_byte = k;
							node = node->next = new LinkList;
							k++;
						}
						if (k <= se) result[cur_MCU][k] = 0;
					}
					EOBRUN--;
					cur_MCU++;
					while (EOBRUN > 0) {
						for (unsigned char k = ss; k <= se; k++) {
							while (result[cur_MCU][k] != 0 && k <= se) {
								node->which_block = cur_MCU;
								node->which_byte = k;
								node = node->next = new LinkList;
								k++;
							}
							if (k <= se) result[cur_MCU][k] = 0;
						}
						EOBRUN--;
						cur_MCU++;
					}
					cur_MCU--;
				}
				while (1) {
					if (head->next == NULL) {
						delete head;
						break;
					}
					node = head;
					if (result[node->which_block][node->which_byte] > 0) {
						result[node->which_block][node->which_byte] += point_trans*readOneBit();
					}
					else if (result[node->which_block][node->which_byte] < 0) {
						result[node->which_block][node->which_byte] -= point_trans*readOneBit();
					}
					head = node->next;
					delete node;
				}
				break;
			}
			else {
				if (value == 0xf0) {//ZRL
					for (char i = 0; i < 16; i++) {
						while (result[cur_MCU][k] != 0 && k <= se) {
							node->which_block = cur_MCU;
							node->which_byte = k;
							node = node->next = new LinkList;
							k++;
						}
						result[cur_MCU][k] = 0;
						k++;
					}
					k--;
				}
				else {
					zero_run_length = value / 16;
					digit = 1;//ssss只能为1
					//	digit = value % 16;
					for (char i = zero_run_length; i > 0; i--) {
						while (result[cur_MCU][k] != 0 && k <= se) {
							node->which_block = cur_MCU;
							node->which_byte = k;
							node = node->next = new LinkList;
							k++;
						}
						result[cur_MCU][k] = 0;
						k++;
					}
					if (ac_value = readOneBit()) {
						/*	for (char i = 0; i < (digit - 1); i++) {
						ac_value <<= 1;
						ac_value += readOneBit();
						}*/
					}
					else {
						ac_value = -1;
						/*	ac_value = 1;
						for (char i = 0; i < (digit - 1); i++) {
						ac_value <<= 1;
						if (!readOneBit())ac_value++;
						}
						ac_value = -ac_value;*/
					}
					while (result[cur_MCU][k] != 0 && k <= se) {
						node->which_block = cur_MCU;
						node->which_byte = k;
						node = node->next = new LinkList;
						k++;
					}
					ac_value *= point_trans;
					result[cur_MCU][k] += ac_value;
				}
				while (1) {
					if (head->next == NULL) {
						delete head;
						break;
					}
					node = head;
					if (result[node->which_block][node->which_byte] > 0) {
						result[node->which_block][node->which_byte] += point_trans*readOneBit();
					}
					else if (result[node->which_block][node->which_byte] < 0) {
						result[node->which_block][node->which_byte] -= point_trans*readOneBit();
					}
					head = node->next;
					delete node;
				}
			}
		}
		cur_MCU++;
	}
	return 0;
}
int JPEGDecode::readDCProgressiveSubsequentScans(const BYTE Ah, const BYTE Al, SWORD(*y)[64], SWORD(*Cb)[64], SWORD(*Cr)[64], int MCU_sum) {
	for (int cur_MCU = 0; cur_MCU < MCU_sum; cur_MCU++) {
		y[cur_MCU][0] += readOneBit();
		if (sofColor[0].samplingCoefficient & 0x20)	y[++cur_MCU][0] += readOneBit();
		if (sofColor[0].samplingCoefficient & 0x02)	y[++cur_MCU][0] += readOneBit();
		if (!(sofColor[0].samplingCoefficient ^ 0x22)) 	y[++cur_MCU][0] += readOneBit();
		int j = cur_MCU;
		if (sofColor[0].samplingCoefficient & 0x20)j /= 2;
		if (sofColor[0].samplingCoefficient & 0x02)j /= 2;
		Cb[j][0] += readOneBit();
		Cr[j][0] += readOneBit();
	}
	return 0;
}
int JPEGDecode::readDCProgressiveSubsequentScans(const BYTE Ah, const BYTE Al, SWORD(*y)[64], int MCU_sum) {
	for (int cur_MCU = 0; cur_MCU < MCU_sum; cur_MCU++) {
		y[cur_MCU][0] += readOneBit();
	}
	return 0;
}

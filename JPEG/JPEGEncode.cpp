#include"JPEGEncode.h"
#ifdef BENCHMARK
#include<ctime>
#endif
using namespace std;
void JPEGEncode::encodeMain(string fileName, int width, int height, const BYTE *r, const BYTE *g, const BYTE *b) {
	this->fileName = fileName;
	file.open(this->fileName.c_str(), ios::out | ios::binary);
	if (!file) {
		throw "JPG file failed to open.";
	}
	/*..........量化清晰度:越大的数值压缩越大，反之更清晰压缩小.........*/
	int scaleFactor = 25;
	ifstream scaleFactorFile("scale_factor.txt");
	if (!file) {
		throw "scale factor file failed to open.";
	}
	scaleFactorFile >> scaleFactor;
	scaleFactorFile.close();

	this->r = r; this->g = g; this->b = b;
	app0 = APP0(0, 16, 'J', 'F', 'I', 'F', 0, 1, 1, 0, 0, 1, 0, 1, 0, 0);
	dqt[0] = DQT(0);
	dqt[1] = DQT(1);
	sof = SOF(0, 17, 8, height / 0x100, height % 0x100, width / 0x100, width % 0x100, 3);
	sofColor[0] = SOFColour(1, 0x11, 0);
	sofColor[1] = SOFColour(2, 0x11, 1);
	sofColor[2] = SOFColour(3, 0x11, 1);
	dht[0] = DHT(0, stdDcLuminanceLength);//y_dc
	dht[1] = DHT(16, stdAcLuminanceLength);//y_ac
	dht[2] = DHT(1, stdDcChrominanceLength);//c_dc
	dht[3] = DHT(17, stdAcChrominanceLength);//c_ac
	sos = SOS(0, 12, 3);
	sosColor[0] = SOSColour(1, 0);
	sosColor[1] = SOSColour(2, 0x11);
	sosColor[2] = SOSColour(3, 0x11);
	yQtBasicTable = stdLuminanceQT;
	cQtBasicTable = stdChrominanceQT;
	yqt = new BYTE[64];	cqt = new BYTE[64];
	yDcHuffVal = new BYTE[12]; cDcHuffVal = new BYTE[12];
	yAcHuffVal = new BYTE[162]; cAcHuffVal = new BYTE[162];
	yDcHuffLength = new BYTE[12]; cDcHuffLength = new BYTE[12];
	yAcHuffLength = new BYTE[162]; cAcHuffLength = new BYTE[162];
	yDcHuffCode = new WORD[12];	cDcHuffCode = new WORD[12];
	yAcHuffCode = new WORD[162]; cAcHuffCode = new WORD[162];
	yDcEnHuff = new ENHUFF[256]; yAcEnHuff = new ENHUFF[256];
	cDcEnHuff = new ENHUFF[256]; cAcEnHuff = new ENHUFF[256];
	memcpy(yDcHuffVal, stdDcLuminanceValue, 12);
	memcpy(cDcHuffVal, stdDcChrominanceValue, 12);
	memcpy(yAcHuffVal, stdAcLuminanceValue, 162);
	memcpy(cAcHuffVal, stdAcChrominanceValue, 162);
	byteNew = 0; bytePos = 7;

	encode(scaleFactor);
	delete[] yDcHuffVal; delete[] cDcHuffVal; delete[] yAcHuffVal; delete[] cAcHuffVal;
	delete[] yDcHuffLength; delete[] cDcHuffLength; delete[] yAcHuffLength; delete[] cAcHuffLength;
	delete[] yDcHuffCode; delete[] yAcHuffCode; delete[]cDcHuffCode; delete[] cAcHuffCode;
	delete[] yDcEnHuff; delete[] yAcEnHuff; delete[] cDcEnHuff; delete[] cAcEnHuff;
}

void JPEGEncode::encode(int scalefactor) {
	generateHuffmanTable(dht[0], yDcHuffLength, yDcHuffCode, yDcHuffVal, yDcEnHuff);
	generateHuffmanTable(dht[1], yAcHuffLength, yAcHuffCode, yAcHuffVal, yAcEnHuff);
	generateHuffmanTable(dht[2], cDcHuffLength, cDcHuffCode, cDcHuffVal, cDcEnHuff);
	generateHuffmanTable(dht[3], cAcHuffLength, cAcHuffCode, cAcHuffVal, cAcEnHuff);
	set_quant_table(yQtBasicTable, scalefactor, yqt);
	set_quant_table(cQtBasicTable, scalefactor, cqt);
	//showFileQtHt();
	writeFileHeader();
	width = sof.width();
	height = sof.height();
	int x_pos, y_pos;
	int x_remainder, y_remainder;//剩余几个像素
	int x_pos_max, y_pos_max;
	BYTE(*rMCU)[64], (*gMCU)[64], (*bMCU)[64];
	SWORD(*yResult)[64], (*cbResult)[64], (*crResult)[64];

	x_pos_max = (int)ceil((float)width / 8) - 1;
	x_remainder = width & 7;
	y_pos_max = (int)ceil((float)height / 8) - 1;
	y_remainder = height & 7;
	numMCU = (x_pos_max + 1)*(y_pos_max + 1);//mcu总数
	rMCU = new BYTE[numMCU][64];
	gMCU = new BYTE[numMCU][64];
	bMCU = new BYTE[numMCU][64];
	yResult = new SWORD[numMCU][64];
	cbResult = new SWORD[numMCU][64];
	crResult = new SWORD[numMCU][64];

	int MCU_count = 0;
	for (y_pos = 0; y_pos <= y_pos_max; y_pos++) {
		for (x_pos = 0; x_pos <= x_pos_max; x_pos++) {
			int pos_base = 8 * x_pos + width * 8 * y_pos;
			int y_total, x_total;
			((x_pos == x_pos_max) && (x_remainder != 0)) ? x_total = x_remainder : x_total = 8;
			((y_pos == y_pos_max) && (y_remainder != 0)) ? y_total = y_remainder : y_total = 8;
			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					int pos = pos_base + x + y * width;
					int pos_MCU = x + y * 8;
					if ((x < x_total) && (y < y_total)) {
						rMCU[MCU_count][pos_MCU] = r[pos];
						gMCU[MCU_count][pos_MCU] = g[pos];
						bMCU[MCU_count][pos_MCU] = b[pos];
					}
					else {
						rMCU[MCU_count][pos_MCU] = 0;
						gMCU[MCU_count][pos_MCU] = 0;
						bMCU[MCU_count][pos_MCU] = 0;
					}
				}
			}
			MCU_count++;
		}
	}

	int threadsNum = thread::hardware_concurrency() - 1;
	vector<thread> th;
	atomic<int> curMCU(0);
	auto process = [&](/*int &curMCU*/) {//lambda表达式 &引用外部变量
		int num;
		float yTempA[64];
		float cbTempA[64];
		float crTempA[64];
		float tempA[64], tempB[64];
		while ((num = curMCU++) < numMCU) {
			//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			//{
			//	std::lock_guard<std::mutex> lock_guard(mtx);
			//	num = curMCU;
			//	if (curMCU >= numMCU)break;
			//	++curMCU;
				//std::cout << "thread " << std::this_thread::get_id() << " has got permission to solve " << num << std::endl;
			//}
			convertToYCbCr(rMCU[num], gMCU[num], bMCU[num], yTempA, cbTempA, crTempA);
			dct(yTempA, tempA);
			zz(tempA, tempB);
			quantization(tempB, yResult[num], yqt);
			dct(cbTempA, tempA);
			zz(tempA, tempB);
			quantization(tempB, cbResult[num], cqt);
			dct(crTempA, tempA);
			zz(tempA, tempB);
			quantization(tempB, crResult[num], cqt);
		}
	};
	for (int j = 0; j < threadsNum; j++) {	//multi threads
		th.push_back(thread(process/*, std::ref(curMCU)*/));
	}
	process();//main thread
	for (auto &ths : th)ths.join();
	th.clear();
	delete[] rMCU; delete[] gMCU; delete[] bMCU;

	SWORD yPred = 0, cbPred = 0, crPred = 0;
	for (int i = 0; i < numMCU; i++) {	
		writeDC(yDcEnHuff, yResult[i], yPred);
		writeAC(yAcEnHuff, yResult[i]);
		writeDC(cDcEnHuff, cbResult[i], cbPred);
		writeAC(cAcEnHuff, cbResult[i]);
		writeDC(cDcEnHuff, crResult[i], crPred);
		writeAC(cAcEnHuff, crResult[i]);
	}
	delete[] yResult;delete[] cbResult;delete[] crResult;

	/*处理结尾*/
	if (bytePos < 7) {
		if (byteNew == 0xff) {
			file.write((char*)&byteNew, 1);
			byteNew = 0;
			file.write((char*)&byteNew, 1);
		}
		else
			file.write((char*)&byteNew, 1);
	}
	file.put((char)0xff);
	file.put((char)0xd9);
	file.close();
}
void JPEGEncodeEncrypt::encode(int scalefactor) {
#ifdef ENCRYPT_HUFF
	x = 0.2865612612905;
	for (int i = 0; i < 200; i++) logistic();
	generateEncryptedHuffmanTable();
#else
	generateHuffmanTable(dht[0], yDcHuffLength, yDcHuffCode, yDcHuffVal, yDcEnHuff);
	generateHuffmanTable(dht[1], yAcHuffLength, yAcHuffCode, yAcHuffVal, yAcEnHuff);
	generateHuffmanTable(dht[2], cDcHuffLength, cDcHuffCode, cDcHuffVal, cDcEnHuff);
	generateHuffmanTable(dht[3], cAcHuffLength, cAcHuffCode, cAcHuffVal, cAcEnHuff);
#endif
	set_quant_table(yQtBasicTable, scalefactor, yqt);
	set_quant_table(cQtBasicTable, scalefactor, cqt);
	writeFileHeader();
	//showFileQtHt();
	width = sof.width();
	height = sof.height();
	int x_pos, y_pos;
	int x_remainder, y_remainder;//剩余几个像素
	int x_pos_max, y_pos_max;
	BYTE(*rMCU)[64], (*gMCU)[64], (*bMCU)[64];
	SWORD(*yResult)[64], (*cbResult)[64], (*crResult)[64];

	x_pos_max = (int)ceil((float)width / 8) - 1;
	x_remainder = width & 7;
	y_pos_max = (int)ceil((float)height / 8) - 1;
	y_remainder = height & 7;
	numMCU = (x_pos_max + 1)*(y_pos_max + 1);//mcu总数
	rMCU = new BYTE[numMCU][64];
	gMCU = new BYTE[numMCU][64];
	bMCU = new BYTE[numMCU][64];
	yResult = new SWORD[numMCU][64];
	cbResult = new SWORD[numMCU][64];
	crResult = new SWORD[numMCU][64];

	int MCU_count = 0;
	for (y_pos = 0; y_pos <= y_pos_max; y_pos++) {
		for (x_pos = 0; x_pos <= x_pos_max; x_pos++) {
			int pos_base = 8 * x_pos + width * 8 * y_pos;
			int y_total, x_total;
			((x_pos == x_pos_max) && (x_remainder != 0)) ? x_total = x_remainder : x_total = 8;
			((y_pos == y_pos_max) && (y_remainder != 0)) ? y_total = y_remainder : y_total = 8;
			for (int y = 0; y < 8; y++) {
				for (int x = 0; x < 8; x++) {
					int pos = pos_base + x + y * width;
					int pos_MCU = x + y * 8;
					if ((x < x_total) && (y < y_total)) {
						rMCU[MCU_count][pos_MCU] = r[pos];
						gMCU[MCU_count][pos_MCU] = g[pos];
						bMCU[MCU_count][pos_MCU] = b[pos];
					}
					else {
						rMCU[MCU_count][pos_MCU] = 0;
						gMCU[MCU_count][pos_MCU] = 0;
						bMCU[MCU_count][pos_MCU] = 0;
					}
				}
			}
			MCU_count++;
		}
	}

	int threadsNum = thread::hardware_concurrency() - 1;
	vector<thread> th;
	atomic<int> curMCU(0);
	auto process = [&](/*int &curMCU*/) {//lambda表达式 &引用外部变量
		int num;
		float yTempA[64];
		float cbTempA[64];
		float crTempA[64];
		float tempA[64], tempB[64];
		while ((num = curMCU++) < numMCU) {
			//std::this_thread::sleep_for(std::chrono::nanoseconds(1));
			//{
			//	std::lock_guard<std::mutex> lock_guard(mtx);
			//	num = curMCU;
			//	if (curMCU >= numMCU)break;
			//	++curMCU;
			//std::cout << "thread " << std::this_thread::get_id() << " has got permission to solve " << num << std::endl;
			//}
			convertToYCbCr(rMCU[num], gMCU[num], bMCU[num], yTempA, cbTempA, crTempA);
			dct(yTempA, tempA);
			zz(tempA, tempB);
			quantization(tempB, yResult[num], yqt);
			dct(cbTempA, tempA);
			zz(tempA, tempB);
			quantization(tempB, cbResult[num], cqt);
			dct(crTempA, tempA);
			zz(tempA, tempB);
			quantization(tempB, crResult[num], cqt);
		}
	};
	for (int j = 0; j < threadsNum; j++) {	//multi threads
		th.push_back(thread(process/*, std::ref(curMCU)*/));
	}
	process();//main thread
	for (auto &ths : th)ths.join();
	th.clear();
	delete[] rMCU; delete[] gMCU; delete[] bMCU;

#ifdef ENCRYPT
	{
#ifdef BENCHMARK
		clock_t begin = clock();
#endif
		string posFileName = fileName;
		posFileName.erase(posFileName.length() - 4, 4);
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
			cout << "Encrypt:" << endl;
			for (vector<ObjectPosition>::iterator it = objectPosition.begin(); it != objectPosition.end(); ++it) {
				if (objectName.find(it->type) != objectName.end()) {
					cout << it->type << endl;
					Object *obj = new Object(*it, x_pos_max + 1, yResult, cbResult, crResult);
					encrypt(obj->yData, obj->cbData, obj->crData, obj->numMCU);
					obj->writeBack(yResult, cbResult, crResult);
					delete obj;
				}
			}
			posFile.close();
			objectFile.close();
		}
		else {
			cout << "No position file or object file. Fully encrypt instead." << endl;
			encrypt(yResult, cbResult, crResult, numMCU);
		}
#ifdef BENCHMARK
		cout << "Encryption cost time " << (double)(clock() - begin) / CLOCKS_PER_SEC << 's' << endl;
#endif
	}
#endif

	SWORD yPred = 0, cbPred = 0, crPred = 0;
	for (int i = 0; i < numMCU; i++) {
#ifdef ENCRYPT_HUFF
		if ((i & 63) == 63) {
			generateEncryptedHuffmanTable();
		}
#endif
		writeDC(yDcEnHuff, yResult[i], yPred);
		writeAC(yAcEnHuff, yResult[i]);
		writeDC(cDcEnHuff, cbResult[i], cbPred);
		writeAC(cAcEnHuff, cbResult[i]);
		writeDC(cDcEnHuff, crResult[i], crPred);
		writeAC(cAcEnHuff, crResult[i]);
	}
	delete[] yResult; delete[] cbResult; delete[] crResult;

	/*处理结尾*/
	if (bytePos < 7) {
		if (byteNew == 0xff) {
			file.write((char*)&byteNew, 1);
			byteNew = 0;
			file.write((char*)&byteNew, 1);
		}
		else
			file.write((char*)&byteNew, 1);
	}
	file.put((char)0xff);
	file.put((char)0xd9);
	file.close();
}

int JPEGEncode::writeFileHeader() {
	int length;
	BYTE length_H, length_L;

	file.put((char)0xff);
	file.put((char)0xd8);
	file.put((char)0xff);
	file.put((char)0xe0);
	file.write((char*)&app0, sizeof(APP0));

	length = 2 + sizeof(DQT) + 64;
	length_H = length / 0x100;
	length_L = length - length_H * 0x100;
	file.put((char)0xff);
	file.put((char)0xdb);
	file.put(length_H);
	file.put(length_L);
	file.write((char*)&dqt[0], sizeof(DQT));
	file.write((char*)yqt, 64);
	file.put((char)0xff);
	file.put((char)0xdb);
	file.put(length_H);
	file.put(length_L);
	file.write((char*)&dqt[1], sizeof(DQT));
	file.write((char*)cqt, 64);

	file.put((char)0xff);
	file.put((char)0xc0);
	file.write((char*)&sof, sizeof(SOF));
	file.write((char*)&sofColor[0], sizeof(SOFColour));
	file.write((char*)&sofColor[1], sizeof(SOFColour));
	file.write((char*)&sofColor[2], sizeof(SOFColour));

	file.put((char)0xff);
	file.put((char)0xc4);
	length = 2 + sizeof(DHT) + dht[0].htBitTableSum();
	length_H = length / 0x100;
	length_L = length - length_H * 0x100;
	file.put(length_H);
	file.put(length_L);
	file.write((char*)&dht[0], sizeof(DHT));
	file.write((char*)yDcHuffVal, dht[0].htBitTableSum());
	file.put((char)0xff);
	file.put((char)0xc4);
	length = 2 + sizeof(DHT) + dht[1].htBitTableSum();
	length_H = length / 0x100;
	length_L = length - length_H * 0x100;
	file.put(length_H);
	file.put(length_L);
	file.write((char*)&dht[1], sizeof(DHT));
	file.write((char*)yAcHuffVal, dht[1].htBitTableSum());
	file.put((char)0xff);
	file.put((char)0xc4);
	length = 2 + sizeof(DHT) + dht[2].htBitTableSum();
	length_H = length / 0x100;
	length_L = length - length_H * 0x100;
	file.put(length_H);
	file.put(length_L);
	file.write((char*)&dht[2], sizeof(DHT));
	file.write((char*)cDcHuffVal, dht[2].htBitTableSum());
	file.put((char)0xff);
	file.put((char)0xc4);
	length = 2 + sizeof(DHT) + dht[3].htBitTableSum();
	length_H = length / 0x100;
	length_L = length - length_H * 0x100;
	file.put(length_H);
	file.put(length_L);
	file.write((char*)&dht[3], sizeof(DHT));
	file.write((char*)cAcHuffVal, dht[3].htBitTableSum());


	file.put((char)0xff);
	file.put((char)0xda);
	file.write((char*)&sos, sizeof(SOS));
	file.write((char*)&sosColor[0], sizeof(SOSColour));
	file.write((char*)&sosColor[1], sizeof(SOSColour));
	file.write((char*)&sosColor[2], sizeof(SOSColour));
	file.put((char)0);
	file.put((char)0x3f);
	file.put((char)0);
	return 0;
}
int JPEGEncode::generateHuffmanTable(DHT dht, BYTE*huffLength, WORD*huffCode, BYTE*huffValue, ENHUFF*ENHUFF) {
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
	for (int i = 0; i < htBitTableSum; i++) {
		WORD temp = huffValue[i];
		ENHUFF[temp].length = huffLength[i];
		ENHUFF[temp].code = huffCode[i];
	}
	return 0;
}
#ifdef ENCRYPTHUFF
void JPEGEncodeEncrypt::generateEncryptedHuffmanTable() {
	generateEncryptedHuffmanTable(dht[0], yDcHuffLength, yDcHuffCode, yDcHuffVal, yDcEnHuff);
	generateEncryptedHuffmanTable(dht[1], yAcHuffLength, yAcHuffCode, yAcHuffVal, yAcEnHuff);
	generateEncryptedHuffmanTable(dht[2], cDcHuffLength, cDcHuffCode, cDcHuffVal, cDcEnHuff);
	generateEncryptedHuffmanTable(dht[3], cAcHuffLength, cAcHuffCode, cAcHuffVal, cAcEnHuff);
}
int JPEGEncodeEncrypt::generateEncryptedHuffmanTable(DHT dht, BYTE*huffLength, WORD*huffCode, BYTE*huffValue, ENHUFF*ENHUFF) {
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
		if(stopLoop) break;
	}
	//哈夫曼mutate
	for (int i = 1; i <= sum; i++) {
		BYTE byte = ((long long)logistic()*1.0e+14) % 256;
		if(byte & 1) Huffman_Tree[i].mutate();
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
			ENHUFF[temp].length = Huffman_Tree[i].lLength;
			ENHUFF[temp].code = Huffman_Tree[i].lCode;
		}
		if (Huffman_Tree[i].rLength != 0) {
			WORD temp = Huffman_Tree[i].rValue;
			ENHUFF[temp].length = Huffman_Tree[i].rLength;
			ENHUFF[temp].code = Huffman_Tree[i].rCode;
		}
	}
	delete[] Huffman_Tree;
	return 0;
}
#endif
void JPEGEncode::convertToYCbCr(BYTE *rMCU, BYTE *gMCU, BYTE *bMCU, float *yMCU, float *cbMCU, float *crMCU) {
	for (int i = 0; i < 64; i++) {
		yMCU[i] = 0.299f*rMCU[i] + 0.587f*gMCU[i] + 0.114f*bMCU[i] - 128;
		cbMCU[i] = -0.168736f*rMCU[i] - 0.331264f*gMCU[i] + 0.5f*bMCU[i];
		crMCU[i] = 0.5f*rMCU[i] - 0.418688f*gMCU[i] - 0.081312f*bMCU[i];
	} 
}
void JPEGEncode::dct(float*mcu, float*dct) {
	float y[64], AT[64];
	mat_mul((float*)A, mcu, y, 8, 8, 8);
	mat_Transpose((float*)A, AT, 8, 8);
	mat_mul(y, AT, dct, 8, 8, 8);
}
void JPEGEncode::zz(float*ZZ, float*result) {
	for (int i = 0; i<64; i++)
		result[zigzag[i]] = ZZ[i];//注意正反的问题，这里是正向zz变换
}
void JPEGEncode::set_quant_table(const BYTE *basicTable, int scalefactor, BYTE *newTable)// Set quantization table and zigzag reorder it
{
	BYTE i;
	int temp;
	for (i = 0; i < 64; i++)
	{
		temp = ((int)basicTable[i] * scalefactor + 50) / 100;
		// limit the values to the valid range
		if (temp <= 0)	temp = 1;
		if (temp > 255) temp = 255;
		newTable[zigzag[i]] = (BYTE)temp;
	}
}
void JPEGEncode::quantization(float*QT_data, SWORD*result, BYTE*QT_table) {
	float temp;
	for (int i = 0; i < 64; i++) {
		temp = QT_data[i] / QT_table[i];
		result[i] = (SWORD)round(temp);	//Round to nearest integer.
	}
}
int JPEGEncode::writeDC(ENHUFF*enHuff, SWORD*afterQT, SWORD&pred) {
	SWORD diff = afterQT[0] - pred;
	BYTE digit;//待编码值位数
	SWORD temp;//用来判断位数是多少的临时变量
	WORD nagative_temp;//如果是负数进行调整
	SWORD offset;//当前写的位数
	if (diff == 0) {
		for (offset = enHuff[0].length - 1; offset >= 0; offset--) {
			WORD i;
			i = enHuff[0].code & binaryMask[offset];
			writeOneBit(i);
		}
	}
	else {
		for (digit = 0, temp = diff; temp != 0; temp /= 2) {
			digit++;
		}
		for (offset = enHuff[digit].length - 1; offset >= 0; offset--) {
			WORD i;
			i = enHuff[digit].code & binaryMask[offset];
			writeOneBit(i);
		}
		if (diff < 0) {
			nagative_temp = (WORD)diff - 1;
			for (offset = digit - 1; offset >= 0; offset--) {
				WORD i;
				i = nagative_temp & binaryMask[offset];
				writeOneBit(i);
			}
		}
		else {
			for (offset = digit - 1; offset >= 0; offset--) {
				WORD i;
				i = diff & binaryMask[offset];
				writeOneBit(i);
			}
		}
	}
	pred += diff;
	return 0;
}
int JPEGEncode::writeAC(ENHUFF*ENHUFF, SWORD*afterQT) {
	SWORD temp;
	SWORD offset;//当前的位数
	BYTE digit;
	//BYTE zero_run_length = 0;//连0游程编码
	BYTE startpos;
	BYTE nrzeroes;//连0数量
	BYTE end0pos;
	for (end0pos = 63; (end0pos > 0) && (afterQT[end0pos] == 0); end0pos--);	//找到结尾
	int i = 1;
	while (i <= end0pos)
	{
		startpos = i;
		for (; (afterQT[i] == 0) && (i <= end0pos); i++);//跳过0
		nrzeroes = i - startpos;
		if (nrzeroes >= 16) //ZRL
		{
			for (BYTE nrmarker = 1; nrmarker <= nrzeroes / 16; nrmarker++) {
				for (offset = ENHUFF[0xF0].length - 1; offset >= 0; offset--) {
					WORD i;
					i = ENHUFF[0xF0].code & binaryMask[offset];
					writeOneBit(i);
				}
			}
			nrzeroes = nrzeroes % 16;//还剩几位0
		}
//		nrzeroes * 16 + digit
		for (digit = 0, temp = afterQT[i]; temp != 0; temp /= 2) {
			digit++;
		}
		for (offset = ENHUFF[nrzeroes * 16 + digit].length - 1; offset >= 0; offset--) {
			WORD i;
			i = ENHUFF[nrzeroes * 16 + digit].code & binaryMask[offset];
			writeOneBit(i);
		}
		if (afterQT[i] < 0) {
			WORD nagative_temp = (WORD)afterQT[i] - 1;
			for (offset = digit - 1; offset >= 0; offset--) {
				WORD a;
				a = nagative_temp & binaryMask[offset];
				writeOneBit(a);
			}
		}
		else {
			for (offset = digit - 1; offset >= 0; offset--) {
				WORD a;
				a = afterQT[i] & binaryMask[offset];
				writeOneBit(a);
			}
		}
		i++;
	}

	if (end0pos != 63) {//写EOB
		for (offset = ENHUFF[0].length - 1; offset >= 0; offset--) {
			WORD i;
			i = ENHUFF[0].code & binaryMask[offset];
			writeOneBit(i);
		}
	}
	return 0;
}

void JPEGEncode::writeOneBit(WORD w) {
	bool bit;
	if (w == 0) bit = false;
	else bit = true;
	if (bit) //值的当前位为1 (二进制)
		byteNew |= binaryMask[bytePos];
	bytePos--;

	if (bytePos < 0) {
		if (byteNew == 0xff) {
			file.write((char*)&byteNew, 1);
			byteNew = 0;
			file.write((char*)&byteNew, 1);
		}
		else {
			file.write((char*)&byteNew, 1);
		}
		bytePos = 7;
		byteNew = 0;
	}
}

void JPEGEncode::showFileQtHt() {
	int j = 0;
	if (sof.numberOfColor != sos.numberOfColor) {
		cout << "颜色分量前后不一致！" << endl;
	}
	cout << "width:" << sof.width() << " height:" << sof.height() << endl;

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
		cout << dec << (WORD)yDcHuffVal[i] << ' ';
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
		cout << dec << (WORD)yAcHuffVal[i] << ' ';
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
		cout << dec << (WORD)cDcHuffVal[i] << ' ';
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
		cout << dec << (WORD)cAcHuffVal[i] << ' ';
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

void JPEGEncodeEncrypt::encrypt(SWORD(*yMcu)[64], SWORD(*cbMcu)[64], SWORD(*crMcu)[64], int numMCU) {
	double x = 1.5121615730212, y = 12.102462138545, z = -9.4525140151405, u = 0.49161415934378;
	double lx = 0.63726194247947;
	const int ROUND = 3;
	SWORD(*mcu)[64] = new SWORD[numMCU * 3][64];
	memcpy(mcu, yMcu, 64 * numMCU * sizeof(SWORD));
	memcpy(mcu + numMCU, cbMcu, 64 * numMCU * sizeof(SWORD));
	memcpy(mcu + 2 * numMCU, crMcu, 64 * numMCU * sizeof(SWORD));

	//取出系数
	ofstream ori("ori.txt");
	ofstream oriAC("oriAC.txt");
	SWORD *mcuPointer = mcu[0];
	for (int i = 0; i < 3 * numMCU; i++) {
		ori << *mcuPointer << '\t';
		for (int i = 1; i < 64; i++) {
			if (*(mcuPointer + i) != 0) {
				oriAC << *(mcuPointer + i) << "00" << i << '\t';
			}
		}
		mcuPointer += 64;
	}
	ori.close();
	oriAC.close();

	for (int round = 0; round < ROUND; round++) {
		//block scrambling
		HyperChaoticLvSystem lvBlockY(x, y, z, u);
		//HyperChaoticLvSystem lvBlockC(lvBlockY);
		//auto encryptBlockScramblingFunc(std::bind(&JPEGEncodeEncrypt::encryptBlockScrambling, this,
		//	std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
		//thread th1(encryptBlockScramblingFunc, mcu, numMCU, std::ref(lvBlockY));
		//encryptBlockScrambling(mcu + numMCU, 2 * numMCU, lvBlockC);
		//th1.join();
		encryptBlockScrambling(mcu, 3 * numMCU, lvBlockY);

		//dc encrypt
		HyperChaoticLvSystem lvDC(x, y, z, u);
		LogisticMap logisticDC(lx);
		std::function<void(SWORD(*)[64], int, ChaoticMap&, ChaoticMap&)>
				encryptDCFunc(std::bind(&JPEGEncodeEncrypt::encryptDCScramble, this, std::placeholders::_1,
					std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		thread th(encryptDCFunc, mcu, 3 * numMCU, std::ref(lvDC), std::ref(logisticDC));
		//ac encrypt
		HyperChaoticLvSystem lvYAC(x, y, z, u);
		//HyperChaoticLvSystem lvCAC(lvYAC);
		LogisticMap logisticYAC(lx);
		//LogisticMap logisticCAC(logisticYAC);
		//std::function<void(SWORD(*)[64], int, ChaoticMap&, ChaoticMap&)>
		//	encryptACFunc(std::bind(&JPEGEncodeEncrypt::encryptAC, this, std::placeholders::_1,
		//		std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
		//thread th2(encryptACFunc, mcu + numMCU, numMCU, std::ref(lvYAC), std::ref(logisticYAC));
		//thread th3(encryptACFunc, mcu + 2 * numMCU, numMCU, std::ref(lvCAC), std::ref(logisticCAC));
		encryptAC(mcu, 3 * numMCU, lvYAC, logisticYAC);

		//synchronous: wait for threads
		th.join();
		//th2.join();
		//th3.join();
	}
	memcpy(yMcu, mcu, 64 * numMCU * sizeof(SWORD));
	memcpy(cbMcu, mcu + numMCU, 64 * numMCU * sizeof(SWORD));
	memcpy(crMcu, mcu + numMCU * 2, 64 * numMCU * sizeof(SWORD));

	//取出系数
	ofstream res("res.txt");
	ofstream resAC("resAC.txt");
	mcuPointer = mcu[0];
	for (int i = 0; i < 3 * numMCU; i++) {
		res << *mcuPointer << '\t';
		for (int i = 1; i < 64; i++) {
			if (*(mcuPointer + i) != 0) {
				resAC << *(mcuPointer + i) << "00" << i << '\t';
			}
		}
		mcuPointer += 64;
	}
	res.close();
	resAC.close();


	delete[] mcu;
}
/*
void JPEGEncodeEncrypt::roiEncrypt(const char* posFileName) {
	file >> type >> x >> y >> width >> height;

}
void JPEGEncodeEncrypt::encryptComputeDiff(SWORD(*mcu)[64], int numMCU) {
	//计算差分值
	int pred = mcu[0][0];
	for (int i = 1; i < numMCU; i++) {
		int currentDCValue = mcu[i][0];
		mcu[i][0] = currentDCValue - pred;
		pred = currentDCValue;
	}
}
void JPEGEncodeEncrypt::encryptReComputeDiff(SWORD(*mcu)[64], int numMCU) {
	//差分值还原
	int pred = mcu[0][0];
	for (int i = 1; i < numMCU; i++) {
		pred = mcu[i][0] + pred;
		mcu[i][0] = pred;
	}
}
*/
void JPEGEncodeEncrypt::encryptBlockScrambling(SWORD(*mcu)[64], int numMCU, ChaoticMap &chaoticMap) {
	//块置乱
	int *keyStream = new int[numMCU];
	int *keyStreamPointer = keyStream;
	SWORD *mcuPointer = mcu[0];
	SWORD dataTemp[64];
	for (int i = 0; i < numMCU; i++) {
		keyStream[i] = ((long long)((chaoticMap.keySequence())*1.0e+14) % (numMCU - i));
		memcpy(dataTemp, mcuPointer, sizeof(SWORD) * 64);
		memcpy(mcuPointer, mcuPointer + 64 * (*keyStreamPointer), sizeof(SWORD) * 64);
		memcpy(mcuPointer + 64 * (*keyStreamPointer), dataTemp, sizeof(SWORD) * 64);
		mcuPointer += 64; keyStreamPointer++;
	}
	delete[] keyStream;
}
void JPEGEncodeEncrypt::encryptDCScramble(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap &substitution) {
#ifdef BENCHMARK
	clock_t time = clock();
#endif
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
	//加密部分
	//置乱
	int *keyStream = new int[numMCU];
	int *keyStreamPointer = keyStream;
	dcPointer = dc;
	SWORD dataTemp;
	for (int i = 0; i < numMCU; i++) {
		keyStream[i] = ((long long)((permutation.keySequence())*1.0e+14) % (numMCU - i));
		dataTemp = *dcPointer;
		*dcPointer = *(dcPointer + *keyStreamPointer);
		*(dcPointer + *keyStreamPointer) = dataTemp;
		dcPointer++; keyStreamPointer++;
	}
	delete[] keyStream;

	//扩散

	//int pred = 0;
	char lastBit = 0;//扩散初值
	char lastNegative = 0;//正负扩散初值
	dcPointer = dc;
	for (int i = 0; i < numMCU; i++) {
		if (*dcPointer == 0 || *dcPointer == -1024) {
			dcPointer++;
			continue;
		}
		//首先取绝对值
		SWORD absDC = abs(*dcPointer);
		char negative = *dcPointer < 0 ? 1 : 0;
		vector<char>dcBits;
		dcBits.reserve(12);
		while (absDC) {
			dcBits.push_back(absDC & 1);
			absDC >>= 1;
		}
		//正负号处理
		char scrambledNegative = 0;
		//	int scrambledMax = 0, scrambledMin = 0;
		//	for (int i = dcBits.size(); i > 0; i--) {
		//		scrambledMax <<= 1;
		//		scrambledMax++;
		//	}
		//	scrambledMin = ~scrambledMax;
		//	if (scrambledMax > 1016 / qt - pred) scrambledNegative = 1;
		//	else if (scrambledMin < -1024 / qt - pred) scrambledNegative = 0;
		//	else {
		int k = ((long long)((substitution.keySequence())*1.0e+14) & 1);
		scrambledNegative = lastNegative ^ negative ^ k;
		lastNegative = scrambledNegative;
		//	}
		//进行DC值扩散
		char bit = lastBit;
		vector<char>scrambledBits;
		scrambledBits.reserve(12);
		vector<char>::iterator iterator;
		for (iterator = dcBits.begin(); iterator != dcBits.end() - 1; ++iterator) {
			int k = ((long long)((substitution.keySequence())*1.0e+14) & 1);
			bit = bit ^ *iterator ^ k;
			scrambledBits.push_back(bit);
		}
		scrambledBits.push_back(*iterator);//该位固定为1
		lastBit = bit;

		//写回、测试
		SWORD scrambled = 0;
		for (vector<char>::reverse_iterator iterator = scrambledBits.rbegin(); iterator != scrambledBits.rend(); ++iterator) {
			scrambled <<= 1;
			scrambled += *iterator;
		}
		if (scrambledNegative) scrambled = -scrambled;
		*dcPointer = scrambled;
		//pred += scrambled;
		dcPointer++;
	}
	/*//测试数据
	static char name[13] = "DCtest_a.txt";
	ofstream fp(name);
	for (int i = 0; i < numMCU; i++) {
	fp << dc[i] << '\t';
	}
	fp.close();
	name[7]++;*/
	//写回
	dcPointer = dc;
	mcuPointer = mcu[0];
	for (int i = 0; i < numMCU; i++) {
		*mcuPointer = *dcPointer;
		dcPointer++;
		mcuPointer += 64;
	}

	delete[] dc;
#ifdef BENCHMARK
	cout << "DC encrypt time: " << (double)(clock() - time) / CLOCKS_PER_SEC << 's' << endl;
#endif
}
void JPEGEncodeEncrypt::encryptAC(SWORD(*mcu)[64], int numMCU, ChaoticMap &permutation, ChaoticMap &substitution) {
	//对AC系数进行内部置乱、扩散
	//mcu置乱
	//mcu内部置乱：按照编码时RRRRSSSS+附加值的组合
	//非0值扩散：参考DC扩散
#ifdef BENCHMARK
	clock_t time = clock();
#endif
	WORD previous = 0;//扩散初值
	char previousNegative = 0;//正负扩散初值
	for (int j = 0; j < numMCU; j++) {
		vector<PartData> mcuData;
		mcuData.reserve(16);
		PartData partData;
		int zeroCount = 0;
		bool hasZrl = false;
		BYTE end0Pos;//最后一个非0AC系数位置
		for (end0Pos = 63; end0Pos > 0 && mcu[j][end0Pos] == 0; end0Pos--);	//找到结尾
		//分组
		for (int i = 1; i <= end0Pos; i++) {
			partData.data[zeroCount] = mcu[j][i];//记录数据
			if (mcu[j][i] == 0) {
				//为0先计数，再判断ZRL
				zeroCount++;
				if (zeroCount == 16) {
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

		//置乱
		vector<PartData>::iterator iterator(mcuData.begin());
		//去除ZRL
		vector<int> zrlPos;
		if (hasZrl) {
			int pos = 0;
			while (iterator != mcuData.end()) {
				if (iterator->isZRL()) {
					//记录坐标 删除数据
					zrlPos.push_back(pos);
					mcuData.erase(iterator);
					//重置
					pos = 0;
					iterator = mcuData.begin();
					continue;
				}
				pos++;
				++iterator;
			}
		}
		//先置乱非零ac整体
		int keyStream;
		int size = (int) mcuData.size();
		iterator = mcuData.begin();
		for (int i = 0; i < size; i++) {
			long long temp = (long long) ((permutation.keySequence()) * 1.0e+14);
			keyStream = temp
					% (size - i);		//非零ac系数 个
			vector<PartData>::iterator itDst = iterator + keyStream;
			partData = *iterator;
			*iterator = *itDst;
			*itDst = partData;
			++iterator;
		}
		if (hasZrl) {
			//加入ZRL
			PartData zrl;
			zrl.size = 16;
			iterator = mcuData.begin();
			for (vector<int>::reverse_iterator riterator = zrlPos.rbegin();
					riterator != zrlPos.rend(); ++riterator) {
				mcuData.insert(mcuData.begin() + *riterator, zrl);
			}
			//置乱ZRL
			iterator = mcuData.begin();
			size = (int) mcuData.size() - 1;
			for (int i = 0; i < size; i++) {
				keyStream = ((long long) ((permutation.keySequence()) * 1.0e+14)
						% (size - i));		//所有符号-1 个
				vector<PartData>::iterator itDst = iterator + keyStream;
				partData = *iterator;
				*iterator = *itDst;
				*itDst = partData;
				++iterator;
			}
		}

		//扩散
		vector<PartData>::iterator it(mcuData.begin());
		for (it = mcuData.begin(); it != mcuData.end(); ++it) {
			if (it->isZRL()) {
				//	++it;
				continue;
			}
			//AC没有0数据
			//首先取绝对值
			SWORD data = it->data[it->size - 1];
			WORD absData = abs(data);
			char negative = data < 0 ? 1 : 0;
			unsigned mask = 0;
			unsigned digit = 0;
			for (WORD temp = absData; temp; temp >>= 1) {
				digit++;
				mask <<= 1;
				mask++;
			}
			//正负号处理
			char scrambledNegative = 0;
			char key = (char)((long long)(substitution.keySequence()*1.0e+14) & 1);
			scrambledNegative = previousNegative ^ negative ^ key;
			previousNegative = scrambledNegative;
			//进行扩散
			mask >>= 1;//首位1不动，保位数
			if (digit > 2) {
				int key = (long long)((substitution.keySequence())*1.0e+14) & mask;
				//unsigned temp = ((previous ^ (long long)((substitution.keySequence())*1.0e+14)) & mask);
				//absData = absData ^ temp;
				absData = (key ^ ((absData + key) & mask) ^ (previous & mask)) | (1 << (digit - 1));
				previous = absData;
			}
			else if (digit > 1) {
				int key = (long long)((substitution.keySequence())*1.0e+14) & mask;
				//unsigned temp = ((previous ^ (long long)((substitution.keySequence())*1.0e+14)) & mask);
				//absData = absData ^ temp;
				absData = (key ^ (absData & mask) ^ (previous & mask)) | (1 << (digit - 1));
				previous = absData;
			}
			//写回
			it->data[it->size - 1] = scrambledNegative ? -absData : absData;
		}

		//写回
		int i = 1;
		for (it = mcuData.begin(); it != mcuData.end(); ++it) {
			for (int k = 0; k < it->size; k++) {
				mcu[j][i++] = it->data[k];
			}
		}
	}
#ifdef BENCHMARK
	cout << "AC encrypt time: " << (double)(clock() - time) / CLOCKS_PER_SEC << 's' << endl;
#endif
}
#endif
/*
vector::end
The past-the-end element is the theoretical element that would follow the last element in the vector. 
It does not point to any element, and thus shall not be dereferenced.
*/

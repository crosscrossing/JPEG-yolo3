#include"JPEGDecode.h"
#include"BMPheader.h"
#include"JPEGEncode.h"
#ifdef BENCHMARK
#include<ctime>
#endif
#ifdef DETECT
#include"darknet.h"
#endif

void testDC() {
	using namespace std;
	vector<int> ori;
	vector<int> res;
	fstream test("ori.txt");
	while (test.good()) {
		int temp;
		test >> temp;
		ori.push_back(temp);
	}
	test.close();
	test.open("res.txt");
	while (test.good()) {
		int temp;
		test >> temp;
		res.push_back(temp);
	}
	test.close();

	int count = 0;
	int error = 0;
	vector<int>::iterator ori_it = ori.begin();
	vector<int>::iterator res_it = res.begin();
	for (; ori_it != ori.end(); ++ori_it, ++res_it) {
		if (*ori_it != *res_it) {
			error++;
		}
		count++;
	}
	cout << "DC变化率测试： " << (double)error / count << endl;
	return;
}
void testAC() {
	using namespace std;
	vector<int> ori;
	vector<int> res;
	fstream test("oriAC.txt");
	while (test.good()) {
		int temp;
		test >> temp;
		ori.push_back(temp);
	}
	test.close();
	test.open("resAC.txt");
	while (test.good()) {
		int temp;
		test >> temp;
		res.push_back(temp);
	}
	test.close();

	int count = 0;
	int error = 0;
	vector<int>::iterator ori_it = ori.begin();
	vector<int>::iterator res_it = res.begin();
	for (; ori_it != ori.end(); ++ori_it, ++res_it) {
		if (*ori_it != *res_it) {
			error++;
		}
		count++;
	}
	cout << "AC变化率测试： " << (double)error / count << endl;
	return;
}

int main(int argc, char *argv[]) {
	using namespace std;
	try {
		if (argc != 3) {
			throw "Usage: exe_file  bmp jpg"; // 用法: 可执行文件  图像
		}
#ifdef DETECT
		{
			string fileName(argv[1]);
			string detectName(fileName);
			detectName.erase(detectName.length() - 4, 4);
			int darknetArgc = 9 + 2;
			char *darknetArgv[] = { "", "detector", "test", "cfg/coco.data",
					"cfg/yolov3.cfg", "data/yolov3.weights", (char*)fileName.c_str(), "-out",
					(char*)detectName.c_str(), "-thresh", "0.5" };
			/*char *darknetArgv[] = { "", "detector", "test", "cfg/coco.data",
								"cfg/yolov2.cfg", "data/yolov2.weights", (char*)fileName.c_str(), "-out",
								(char*)detectName.c_str(), "-thresh", "0.3" };*/
			darknet::main(darknetArgc, darknetArgv);
		}
#endif
		{
			string jpgNewName(argv[1]);
			jpgNewName.erase(jpgNewName.length() - 4, 4);
			jpgNewName += ".jpg";
			BMPDecode bmpRead(argv[1]);
			cout << "BMP decode complete." << endl;
#ifdef BENCHMARK
			clock_t time = clock();
#endif
			if (bmpRead.getNumberOfColor() == 3) {
#ifndef ENCRYPT
				JPEGEncode
#else
				JPEGEncodeEncrypt
#endif
				encode(jpgNewName, bmpRead.getWidth(), bmpRead.getHeight(),
						bmpRead.getR(), bmpRead.getG(), bmpRead.getB());
			} else {
				throw "grey jpeg encoding unsupported now";
			}
#ifdef BENCHMARK
			cout << "JPEG encode complete: " << (double)(clock() - time) / CLOCKS_PER_SEC << 's' << endl;
#else
			cout << "JPEG encode complete." << endl;
#endif
		}
		{
			string bmpNewName(argv[2]);
			bmpNewName.erase(bmpNewName.length() - 4, 4);
			bmpNewName += "_new.bmp";
#ifdef BENCHMARK
			clock_t time = clock();
#endif
#ifndef ENCRYPT
			JPEGDecode
#else
			JPEGDecodeDecrypt
#endif
			decode(argv[2]); //jpg_name
#ifdef BENCHMARK
					cout << "JPEG decode complete: " << (double)(clock() - time) / CLOCKS_PER_SEC << 's' << endl;
#else
			cout << "JPEG decode complete." << endl;
#endif
			if (decode.getNumberOfColor() == 3) {
				BMPEncode bmpWrite(bmpNewName, decode.getWidth(),
						decode.getHeight(), 24, decode.getR(), decode.getG(),
						decode.getB());
			} else {
				BMPEncode bmpWrite(bmpNewName, decode.getWidth(),
						decode.getHeight(), 8, decode.getY());
			}
			cout << "BMP encode complete." << endl;
		}
	} catch (const char *error) {
		cerr
				<< "--------------------------------------------------E---R---R---O---R----------------------------------------------------"
				<< endl;
		cerr << error << endl;
		system("pause");
		return 1;
	} catch (exception &e) {
		cerr
				<< "--------------------------------------------------E---R---R---O---R----------------------------------------------------"
				<< endl;
		cerr << "Standard exception caught: " << e.what() << endl;
		system("pause");
		return 1;
	}

	testDC();
	testAC();
	return 0;
}

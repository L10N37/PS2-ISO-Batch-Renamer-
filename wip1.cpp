#include<fstream>
#include<iostream>
#include<direct.h>
#include<regex>

using namespace std;


#define RootDirRecordBegin 0x809E	// Stored sometimes as MSB OR LSB @ $809E / $809F and MSB @ $80A4/$80A5. Most commonly (not always) points to a record @ 01 05 or 00 16 (0x82800 [DVD] / 0xB000 [CD])
#define GrabDirRecordMSB 0x80A4		// +1 to grab 2nd byte
#define RootDirSizeMSB 0x80AC		// +1 to grab 2nd byte , Also written as both MSB and LSB in the PVD, A lot of CD games report 2048bytes (full sector) for root dir listing size for some reason.
#define DiscSectorSize 2048


int fileindex = 0;
std::string nextfile[1000];




	int main()
	{
	

		//Generate a text file list of PS2 ISO files to go through
		system("dir /B *.iso > PS2_iso_files.txt");

		// Count files to rename

		int amount_of_files = 0;
		string line;
		ifstream file("PS2_iso_files.txt");
		while (getline(file, line))
		amount_of_files++;
		cout << "Numbers of files to rename : " << amount_of_files << endl;


		
		
		ifstream f("PS2_iso_files.txt");
		for (int fileindex = 0; fileindex < amount_of_files; fileindex++) {
			
			std::string filename;
			std::getline(f, filename);
			nextfile[fileindex] = filename;
			cout << "NEXT FILE TO PROCESS IS: " << nextfile[fileindex] <<endl;
			

			//  Start parsing of file names to fstream for processing
			fstream file1(nextfile[fileindex], ios::in | ios::binary);


			// Get RootDir Location
			int RootDirLoc0, RootDirLoc1, RootDirConv;
			char x;
			file1.seekg(GrabDirRecordMSB, ios::beg);
			file1.read((&x), 1);
			RootDirLoc0 = static_cast<int>(x);
			file1.seekg(GrabDirRecordMSB + 0x01, ios::beg);
			file1.read((&x), 1);
			RootDirLoc1 = static_cast<int>(x);
			cout << "RootDir located @ sector 0" << RootDirLoc0 << "0" << RootDirLoc1 << endl;
			RootDirConv = (RootDirLoc0 << 8) | RootDirLoc1; // int concatenated to hex value
			int RootDirectoryLocation = RootDirConv * DiscSectorSize;
			cout << "root directory calculated to be @ offset: 0x" << hex << RootDirectoryLocation << endl;


			// Store RootDir in RAM

			int RootDirBuffer, RootDirContents[1024];

			for (int i = 0; i < 1024; i++) {
				file1.seekg(RootDirectoryLocation + i, ios::beg);
				file1.read((&x), 1);
				RootDirBuffer = static_cast<int>(x);
				RootDirContents[i] = RootDirBuffer;
				cout << RootDirContents[i];
			}


		}

		
	}

#include<fstream>
#include<iostream>
#include<regex>
#include <string>
#include <conio.h>
#include <stdio.h>

using namespace std;


#define RootDirRecordBegin 0x809E	// Stored sometimes as MSB OR LSB @ $809E / $809F and MSB @ $80A4/$80A5. Most commonly (not always) points to a record @ 01 05 or 00 16 (0x82800 [DVD] / 0xB000 [CD])
#define GrabDirRecordMSB 0x80A4		// +1 to grab 2nd byte
#define RootDirSizeMSB 0x80AC		// +1 to grab 2nd byte , Also written as both MSB and LSB in the PVD, A lot of CD games report 2048bytes (full sector) for root dir listing size for some reason.
#define DiscSectorSize 2048


int fileindex = 0;
std::string nextfile[1000];
std::string RegionStringGlobal;



string ID_String_conversion(char* x) {
	string r = x;
	return r;
}

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
	file.close();



	ifstream f("PS2_iso_files.txt");
	for (fileindex = 0; fileindex < amount_of_files; fileindex++)
	{

		std::string filename;
		std::getline(f, filename);
		nextfile[fileindex] = filename;
		cout << "NEXT FILE TO PROCESS IS: " << nextfile[fileindex] << endl;


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
		uint32_t RootDirectoryLocation = RootDirConv * DiscSectorSize;
		cout << "root directory calculated to be @ offset: 0x" << hex << RootDirectoryLocation << endl;


		// Store RootDir in RAM
		int RootDirBuffer, RootDirContents[2048];
		
		for (int i = 0; i < DiscSectorSize; i++) {
			file1.seekg(RootDirectoryLocation + i, ios::beg);
			file1.read((&x), 1);
			RootDirBuffer = static_cast<int>(x);
			RootDirContents[i] = RootDirBuffer;
			//cout << RootDirContents[i];
		}


		// Search RootDirContents array for 0x45, 0x4D, 0x2E, 0x43, 0x4E, 0x46 ("EM.CNF"  in root directory listing, work our way back from here to file location bytes)

		const int CNF[] = { 0x45, 0x4D, 0x2E, 0x43, 0x4E, 0x46 };
		auto it1 = std::search(begin(RootDirContents), end(RootDirContents), begin(CNF), end(CNF));
		long long int index = std::distance(begin(RootDirContents), it1);

		std::cout << "System CNF Location Found?: " << (it1 == end(RootDirContents) ? "no" : "yes") << endl;
		//std::cout << ""
		cout << "Distance into array the start of the complete CNF string (as bytes) was found: 0x" << index + 1 << endl;


		// Go back to the open ISO file, locate SYSTEM.CNF file location bytes in RootDir

		long long int SystemLocationByte1, SystemLocationByte2, SystemLocationByte3, SystemLocationByte4, SystemLocationByteConv;

		file1.seekg(RootDirectoryLocation + index - 31, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte1 = static_cast<unsigned char>(x);

		file1.seekg(RootDirectoryLocation + index - 30, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte2 = static_cast<unsigned char>(x);

		file1.seekg(RootDirectoryLocation + index - 29, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte3 = static_cast<unsigned char>(x);


		file1.seekg(RootDirectoryLocation + index - 28, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte4 = static_cast<unsigned char>(x);


		SystemLocationByteConv = (SystemLocationByte1 << 24) | (SystemLocationByte2 << 16) | (SystemLocationByte3 << 8) | SystemLocationByte4; // int concatenated to hex value
		cout << "SYSTEM.CNF located @ sector 0x" << SystemLocationByte1 << SystemLocationByte2 << SystemLocationByte3 << SystemLocationByte4 << endl;

		cout << "Pre HEX OFFSET conversion: "<< SystemLocationByteConv << endl;
		long long int SystemCNFLocation = SystemLocationByteConv * DiscSectorSize;
		cout << "SYSTEM.CNF calculated to be @ offset: 0x" << SystemCNFLocation << endl;


		// Extract GameID from SYSTEM.CNF and convert it to a string
		// +16 bytes won't work on all games as some are +14 bytes in, so we need to search for " 0:\ " (end of BOOT2) this is "0x30 0x3A 0x3C", its a precursor to the GAMEID.
		// ....which means we need to store the SYSTEM.CNF in an array

		// Store SYSTEM.CNF in RAM
		int SystemCNFBuffer, SystemCNFContents[64];

		for (int i = 0; i < 64; i++) {
			file1.seekg(SystemCNFLocation + i, ios::beg);
			file1.read((&x), 1);
			SystemCNFBuffer = static_cast<int>(x);
			SystemCNFContents[i] = SystemCNFBuffer;
			//cout << SystemCNFContents[i];
		}
		
		const long long int IDPrecursor[] = { 0x30, 0x3A, 0x5C };
		auto it6 = std::search(begin(SystemCNFContents), end(SystemCNFContents), begin(IDPrecursor), end(IDPrecursor));
		long long int index5 = std::distance(begin(SystemCNFContents), it6);

		std::cout << "Game ID Precursor located in SYSTEM.CNF?: " << (it6 == end(SystemCNFContents) ? "no" : "yes") << endl;

		cout << "Distance into array the start of the precursor (as bytes) was found: 0x" << index5 + 3 << endl;
		

				
		long long int GameIDLocation = SystemCNFLocation + index5 +3 ;



		std::string Game_ID;
		char gameidchars[12];

		for (int i = 0; i < 12; i++) {
			file1.seekg(GameIDLocation + i, ios::beg);
			file1.get(gameidchars[i]);
			Game_ID = ID_String_conversion(gameidchars);

		}
		// Clean junk characters @ end of the string for displaying on terminal window
		for (int i = 11; i < 44; i++) {
			Game_ID[i] = ' ';
		}
		// Create a sub.str for even comparison further down
		std::string Game_IDSubString = Game_ID.substr(0, 11);

		cout << "GAME ID extracted from ISO: " << Game_ID;

	
		// Open Look up table/ Find GameID Line in GameID text file - actual game title follows
		int linecounter = 0;
		std::string Find_Game_ID; // temp storage while seeking per iteration
		std::string SubString;	  // Sub-String for GameID comparision / find the correct line for title extraction}


		ifstream gamelistfile("gameid.txt", ios::in);
		do {
			std::getline(gamelistfile, Find_Game_ID);
			SubString = Find_Game_ID.substr(0, 11);
			linecounter++;
			//cout << "SubString: " << SubString.length() << endl;
			//cout << "Game_ID: " << Game_IDSubString.length() << endl;

		} while (SubString!= Game_IDSubString);


		
		cout << "Game Title/ ID: " << Find_Game_ID << endl;
		Find_Game_ID.erase(0, 11);	// erase GAMEID from title name, leaving only game title
		Find_Game_ID.pop_back();				// erase the final null character (there's one at the end of every title in the namebase
		Find_Game_ID.erase(0, 1);		// removes first NULL character
		std::string newfilename = Find_Game_ID + ".iso";
		cout << "New file name:" << newfilename << endl;


		cout << "GameID for title rename found @ line: 0x " << linecounter << endl;
		


		// Close ISO file, it's ready for renaming
		file1.close();
		//rename the file
		int successful_rename = rename(filename.c_str(), newfilename.c_str());
		if (successful_rename == 0) { cout << "Game renamed succesfully!" << endl; }
		else { cout << "Error renaming, multi-disc game? attempting to append filename with disc #" << endl; 
		newfilename = Find_Game_ID + " (Disc 2).iso";
		rename(filename.c_str(), newfilename.c_str());
		}


		cout << endl; cout << endl; cout << endl;
	}

	
	return 0;
}

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
std::string nextfile[5000];
std::string RegionStringGlobal;

string ID_String_conversion(char* x) {
	std::string r = x;
	return r;
}

int main()
{

	//Generate a text file list of PS2 ISO files to go through
	system("dir /B *.iso > PS2_iso_files.txt");
	//Generate a text file list of games that failed to rename
	std::ofstream failedFile("failed.txt");

	// Count files to rename

	int amount_of_files = 0;
	std::string line;
	std::ifstream file("PS2_iso_files.txt");
	while (getline(file, line))
		amount_of_files++;
	cout << "Numbers of files to rename : " << amount_of_files << endl;
	file.close();

	std::ifstream f("PS2_iso_files.txt");
	for (fileindex = 0; fileindex < amount_of_files; fileindex++)
	{
		bool endOfFileReached = false;
		std::string filename;
		std::getline(f, filename);
		nextfile[fileindex] = filename;
		std::cout << "NEXT FILE TO PROCESS IS: " << nextfile[fileindex] << endl;

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
		}

		// Search RootDirContents array for 0x45, 0x4D, 0x2E, 0x43, 0x4E, 0x46 ("EM.CNF"  in root directory listing, work our way back from here to file location bytes)

		const int CNF[] = { 0x45, 0x4D, 0x2E, 0x43, 0x4E, 0x46 };
		auto it1 = std::search(begin(RootDirContents), end(RootDirContents), begin(CNF), end(CNF));
		long long int index = std::distance(begin(RootDirContents), it1);

		std::cout << "System CNF Location Found?: " << (it1 == end(RootDirContents) ? "no" : "yes") << endl;
		std::cout << "Distance into array the start of the complete CNF string (as bytes) was found: 0x" << index + 1 << endl;


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
		std::cout << "SYSTEM.CNF located @ sector 0x" << SystemLocationByte1 << SystemLocationByte2 << SystemLocationByte3 << SystemLocationByte4 << endl;

		//std::cout << "Pre HEX OFFSET conversion: "<< SystemLocationByteConv << endl;
		long long int SystemCNFLocation = SystemLocationByteConv * DiscSectorSize;
		std::cout << "SYSTEM.CNF calculated to be @ offset: 0x" << SystemCNFLocation << endl;


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
		}

		const long long int IDPrecursor[] = { 0x30, 0x3A, 0x5C };
		auto it6 = std::search(begin(SystemCNFContents), end(SystemCNFContents), begin(IDPrecursor), end(IDPrecursor));
		long long int index5 = std::distance(begin(SystemCNFContents), it6);

		std::cout << "Game ID Precursor located in SYSTEM.CNF?: " << (it6 == end(SystemCNFContents) ? "no" : "yes") << endl;

		std::cout << "Distance into array the start of the precursor (as bytes) was found: 0x" << index5 + 3 << endl;

		long long int GameIDLocation = SystemCNFLocation + index5 + 3;

		std::string Game_ID;
		char gameidchars[11];

		for (int i = 0; i < 11; i++) {
			file1.seekg(GameIDLocation + i, ios::beg);
			file1.get(gameidchars[i]);
			Game_ID = ID_String_conversion(gameidchars);
		}

		std::string Game_IDSubString = Game_ID.substr(0, 11);

		std::cout << "GAME ID extracted from ISO: " << Game_IDSubString << endl;

		// Open Look up table/ Find GameID Line in GameID text file - actual game title follows
		int linecounter = 0;
		std::string Find_Game_ID; // temp storage while seeking per iteration
		std::string SubString;	  // Sub-String for GameID comparision / find the correct line for title extraction}

		std::ifstream gamelistfile("gameid.txt", ios::in);

		do {
			std::getline(gamelistfile, Find_Game_ID);
			SubString = Find_Game_ID.substr(0, 11);
			linecounter++;

			// If the end of database file is reached and the game rename has failed, write the filename to "failed.txt" and break the loop
			if (gamelistfile.eof() && !endOfFileReached) {
				endOfFileReached = true;
				failedFile << filename << std::endl; // Write the filename to "failed.txt"
				break;
			}
		} while (SubString != Game_IDSubString);

		if (!endOfFileReached) {

			std::cout << "Game Title/ ID: " << Find_Game_ID << endl;
			Find_Game_ID.erase(0, 11);	// erase GAMEID from title name, leaving only game title
			Find_Game_ID.pop_back();	// erase the final null character (there's one at the end of every title in the namebase)
			Find_Game_ID.erase(0, 1);	// removes first NULL character
			std::string newfilename = Find_Game_ID + ".iso";
			std::cout << "New file name:" << newfilename << endl;

			std::cout << "GameID for title rename found @ line: 0x " << linecounter << endl;

			file1.close();

			// Derive original CHD filename from ISO filename
			std::string chdOldName = filename;
			if (chdOldName.size() > 4 && chdOldName.substr(chdOldName.size() - 4) == ".iso") {
				chdOldName.replace(chdOldName.size() - 4, 4, ".chd"); // replace .iso with .chd
			}

			// Create new CHD name using game title
			std::string newChdName = newfilename;
			if (newChdName.size() > 4 && newChdName.substr(newChdName.size() - 4) == ".iso") {
				newChdName.replace(newChdName.size() - 4, 4, ".chd"); // replace .iso with .chd
			}

			// Rename CHD
			int successful_rename = rename(chdOldName.c_str(), newChdName.c_str());
			if (successful_rename == 0) {
				cout << "CHD renamed successfully!" << endl;

				// Now delete the ISO and CUE files
				std::string cueFile = filename;
				if (cueFile.size() > 4 && cueFile.substr(cueFile.size() - 4) == ".iso") {
					cueFile.replace(cueFile.size() - 4, 4, ".cue");
				}

				if (remove(filename.c_str()) == 0)
					std::cout << "Deleted ISO file: " << filename << endl;
				else
					std::cout << "Failed to delete ISO file: " << filename << endl;

				if (remove(cueFile.c_str()) == 0)
					std::cout << "Deleted CUE file: " << cueFile << endl;
				else
					std::cout << "Failed to delete CUE file: " << cueFile << endl;

			}
			else {
				std::cout << "Error renaming CHD: file might be open or already renamed." << endl;
			}

			std::cout << endl; std::cout << endl; std::cout << endl;
		}
	}

	// Close "failed.txt"
	failedFile.close();

	return 0;
}

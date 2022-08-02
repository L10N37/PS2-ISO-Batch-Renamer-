#include<fstream>
#include<iostream>
#include<regex>



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
		int RootDirectoryLocation = RootDirConv * DiscSectorSize;
		cout << "root directory calculated to be @ offset: 0x" << hex << RootDirectoryLocation << endl;


		// Store RootDir in RAM

		int RootDirBuffer, RootDirContents[2048];

		for (int i = 0; i < 2048; i++) {
			file1.seekg(RootDirectoryLocation + i, ios::beg);
			file1.read((&x), 1);
			RootDirBuffer = static_cast<int>(x);
			RootDirContents[i] = RootDirBuffer;
			//cout << RootDirContents[i];
		}


		// THIS IS PART OF THE FOR LOOP ITERATING THROUGH FILES IT PLACED IN THE .TXT FILE
		// Search RootDirContents array for 0x43 0x4E 0x46 (CNF extension in root directory listing, work our way from back from here to file location bytes)

		const int CNF[] = { 0x43, 0x4E, 0x46 };
		auto it1 = std::search(begin(RootDirContents), end(RootDirContents), begin(CNF), end(CNF));
		int index = std::distance(begin(RootDirContents), it1);

		std::cout << "System CNF Location Found?: " << (it1 == end(RootDirContents) ? "no" : "yes") << endl;

		cout << "Distance into array the start of the complete CNF string (as bytes) was found: 0x" << index + 1 << endl;



		// Go back to the open ISO file, locate SYSTEM.CNF file location bytes in RootDir

		uint32_t SystemLocationByte1, SystemLocationByte2, SystemLocationByte3, SystemLocationByte4, SystemLocationByteConv;

		file1.seekg(RootDirectoryLocation + index - 34, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte1 = static_cast<unsigned char>(x);

		file1.seekg(RootDirectoryLocation + index - 33, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte2 = static_cast<unsigned char>(x);

		file1.seekg(RootDirectoryLocation + index - 32, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte3 = static_cast<unsigned char>(x);


		file1.seekg(RootDirectoryLocation + index - 31, ios::beg);
		file1.read((&x), 1);
		SystemLocationByte4 = static_cast<unsigned char>(x);




		SystemLocationByteConv = (SystemLocationByte1 << 24) | (SystemLocationByte2 << 16) | (SystemLocationByte3 << 8) | SystemLocationByte4; // int concatenated to hex value
		cout << "SYSTEM.CNF located @ sector 0x" << SystemLocationByte1 << SystemLocationByte2 << SystemLocationByte3 << SystemLocationByte4 << endl;



		int SystemCNFLocation = SystemLocationByteConv * DiscSectorSize;
		cout << "SYSTEM.CNF calculated to be @ offset: 0x" << hex << SystemCNFLocation << endl;


		// Extract GameID from SYSTEM.CNF and convert it to a string
		int GameIDLocation = SystemCNFLocation + 16;
		std::string Game_ID;
		char gameidchars[12];

		for (int i = 0; i < 12; i++) {
			file1.seekg(GameIDLocation + i, ios::beg);
			file1.get(gameidchars[i]);
			Game_ID = ID_String_conversion(gameidchars);

		}
		// Clean junk characters @ end of the string
		for (int i = 11; i < 44; i++) {
		Game_ID[i] = ' ';
	}


		cout << "GAME ID: " << Game_ID << endl;
		
	
	}

	
	return 0;
}

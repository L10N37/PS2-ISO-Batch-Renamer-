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




int process() {

	
	//Open files
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
		// cout << RootDirContents[i];
	}
				
	return 0;
	
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
		for (int fileindex = 0; fileindex < amount_of_files; fileindex++) {
			
			string filename;
			std::getline(f, filename);
			nextfile[fileindex] = filename;
			cout << "NEXT FILE TO PROCESS IS: " << nextfile[fileindex] <<endl;
			

			//  Start parsing of file names to fstream for processing
			process();


		}

		
	}















/*
	string ID_String_conversion(char* x) {
		string r = x;
		return r;
	}
	*/


			
		/*
		fstream file;
		file.open("SLUS_202.26.iso", ios::in | ios::binary); //temporary, change to variable to rotate through games list


		
		if (!file.is_open())
		{
			cout << "error, can not open ISO file/s\n";
		}

		else
		{
			cout << "scanning file for game ID\n";

			*/






	/*
		do
		{

			f.seekg(fseek_offset, ios::beg);
			f.get(GAMEID);
			fseek_offset++;






		}


		while (fseek_offset != fseekend);
		*/
		//(FETCHED_ID[0] != 'S' || FETCHED_ID[1] != 'L' || FETCHED_ID[2] != 'U' || FETCHED_ID[3] != 'S' && fseek_offset != fseek_begin + 3);

		//cout << "GameRegionID: " << FETCHED_ID << endl;
		//cout << "GameID as string: " << RegionStringGlobal << endl;
			/*
				for (int x = 0; x < 7; x++) {

					char FETCHED_ID_INDEX = 0;
					f.seekg(1, ios::cur);
					FETCHED_ID[FETCHED_ID_INDEX] = GAMEID;
				}
				cout << "GameID#: " << FETCHED_CODE;
		     */

			 /*
					 FETCHED_ID[GAMEID_INDEX] =GAMEID;
					 string RegionString = ID_String_conversion(FETCHED_ID);
					 GAMEID_INDEX++;
					 RegionStringGlobal = RegionString;
					 */
					 //cout << RegionStringGlobal;


	/*
				// Print file offset of found RegionID characters
				switch (GAMEID) {
				case 'S':

					PositionIndicator1 = f.tellg();
					cout << "Found Character 'S' @ Location: ";
					cout << hex << PositionIndicator1 << endl;
					f.seekg(PositionIndicator1, ios::beg);
					break;
				case 'L':

					PositionIndicator1 = f.tellg();
					cout << "Found Character 'L' @ Location: ";
					cout << hex << PositionIndicator1 << endl;
					break;
				case 'U':

					PositionIndicator1 = f.tellg();
					cout << "Found Character 'U' @ Location: ";
					cout << hex << PositionIndicator1 << endl;
					break;

				}
				*/
				//f.open(TempWorkPath+"SYSTEM.CNF", ios::in); 
				//system("start SLUS_202.26.iso");
				//system("wmic logicaldisk get name");
				//cout << "Please enter the drive letter your first PS2 ISO mounted to: "<< endl;
				//cin >> DriveLetter;
				//cout << "Please enter a path for temp work folder, i.e. x:/temp/ , YOU MUST USE BACKSLASHES, not forward slashes as in the example" << endl;
				//cin >> TempWorkPath;



				//Creating folder
				//if (_mkdir(TempWorkPath.c_str()) == -1) //change to create with o/w
				//{
				//	cerr << " Error : " << strerror(errno) << endl;
				//}

				//else
				//{
				//	cout << "Temporary folder for GAMEID extraction created" << endl;

				//}
				//system("TIMEOUT /T 5");
				//cout << " Waiting 5s for image to mount" << endl;


		//std::string SystemDriveLetter = "copy " + DriveLetter + ":\SYSTEM.CNF " + TempWorkPath;

		//system(SystemDriveLetter.c_str());


//int GAMEID_INDEX = 0;

			//std::string RegionString;












			/*
						do
						{


							f.seekg(fseek_offset, ios::beg);
							f.get(GAMEID);
							FETCHED_ID[GAMEID_INDEX] = GAMEID;
							RegionString = ID_String_conversion(FETCHED_ID);
							RegionStringGlobal = RegionString;
							GameIDloop++;
							GAMEID_INDEX++;
							fseek_offset++;

									}

					while (GameIDloop < 11);

				} */



				//cout << "GameID: " << RegionStringGlobal << endl;
				//GameIDloop = 0x00;
				//convert above array into a string, use this string for renaming ISO file (after unmounting it)

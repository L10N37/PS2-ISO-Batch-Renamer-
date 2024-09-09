# PS2-ISO-Batch-Renamer- (c) VajskiDs Consoles 2022

Just unzip the gameid.txt and ps2.exe file into your ps2 ISO folder.<br />
Grab the gameid.txt from the root of this repository as it will be more up to date!<br />
No matter what the game is called (as long as it has extension *.iso) - it will be renamed appropriately.<br />
!!PS2 ISO's ONLY!!

Requires dotnet: https://dotnet.microsoft.com/en-us/download<br />
Demo: https://www.youtube.com/watch?v=4r0DLwKOaX0<br />

A few minor changes were made post demo video after testing with a much greater amount of games.
The initial versions were searching for 3 HEX values for "CNF". 
It appears some games have more than one ".CNF" file - the search was enlarged to "EM.CNF" and adjustments were made to work backwards from here to the SYSTEM.CNF location bytes.

I also realised Onimusha Dawn of dreams has 2 GAMEID's, just the GAMEID's used on the lookup table are doubled up on names. 


<br />
I have uploaded the original excel file from "Ranzer11" @ https://forums.pcsx2.net/Thread-PS2-Renamer-Tool-v1
<br />
This seems to be a far more thorough list, but isn't formattted correctly!
<br />

gameid.txt comes from HDLBatch! I will keep an updated version here and append changes made. <br />
Please note, when adding your own -Add them with a null at the end of the line and no blank new line.<br />
<br />
So far:
<br />
Onimusha - Dawn of dreams has being separated to 2 discs (they were sharing the same title)<br />
<br />
Added Disc Numbers: <br />
<br />
SLES_820.38 Onimusha - Dawn of Dreams [Disc 1] <br />
SLES_820.39 Onimusha - Dawn of Dreams [Disc 2] <br />
<br />
Added Disc Number and re-grouped: <br />
<br />
SLUS_211.80 Onimusha - Dawn of Dreams [Disc 1] <br />
SLUS_213.62 Onimusha - Dawn of Dreams [Disc 2] <br />

Corrected Typos <br />

SCES_533.99 Yazuka Fury <br />
SCES_541.71 Yazuka <br />
SLES_533.99 Yazuka Fury <br />
SLES_541.71 Yazuka <br />
SLUS_217.69 Yazuka 2 <br />
<br />
All to 'Yakuza' 
<br />
<br />
<br />
<br />
Added
<br />
SLUS_218.67 Guitar Hero - Van Halen <br />
SLUS_219.28 Scooby Doo - and the Spooky Swamp <br />
SLUS_219.00 Scooby-Doo! First Frights <br />
SLUS_218.99 Silent Hill- Shattered Memories <br />
SCUS_975.84 Syphon Filter: Logan's Shadow <br />
SLUS_219.04 Teenage Mutant Ninja Turtles: Smash-Up <br />
SLUS_219.15 The Lord of the Rings - Aragorn's Quest <br />
SLUS_218.83 Disney-Pixar's Cars - Race-O-Rama <br />
SLUS_219.23 Dora the Explorer- Dora Saves the Crystal Kingdom <br />
SLUS_219.44 Dora the Explorer- Dora's Big Birthday Adventure <br />
SLUS_219.26 Ni Hao - Kai-Lan- Super Game Day <br />
SLUS_219.55 PRO EVOLUTION SOCCER 2013 <br />
SCUS_943.46 SingStar Latino <br />
SCUS_976.27 Singstar Pop Volume 2 <br />
SLES_546.68 Thomas  Friends - A Day at the Races<br />
SLUS_219.40 WWE ALL STARS <br />
SLUS_219.01 WWE Smackdown! vs. RAW 2010 <br /> 
SLUS_219.39 WWE Smackdown! vs. RAW 2011 <br />
SLUS_210.33 Second Sight <br />
SLPS-20318 3D Kakutou Tsukuru 2 [Enterbrain Collection] <br />
SLES-54973 Avventure di Lupin III, Le - Lupin la Morte, Zenigata l'Amore <br />
SLES-82019 Cy Girls (Aska Disc) <br />
SLES-82021 Cy Girls (Aska Disc) <br />
SLES-82018 Cy Girls (Ice Disc) <br />
SLES-82020 Cy Girls (Ice Disc) <br />
SCES-54623 Grand Theft Auto - Vice City Stories <br />
SLES-54623 Grand Theft Auto - Vice City Stories <br />
SLPM-66917 Grand Theft Auto - Vice City Stories <br />
SLUS-21590 Grand Theft Auto - Vice City Stories <br />
SLES-50903 MegaRace 3 - Nanotech Disaster <br />
SLUS-20932 Mercenaries - Playground of Destruction <br />
SLES-52588 Mercenaries - Playground of Destruction <br />
SLES-52590 Mercenaries - Playground of Destruction <br />
SLES-53008 Mercenaries - Playground of Destruction <br />
SLES-52589 Mercenaries - Playground of Destruction <br />
SLED-52979 Mercenaries - Playground of Destruction (Demo) <br />
SLES-54998 Mercenaries 2 - World in Flames <br />
SLES-54999 Mercenaries 2 - World in Flames <br />
SLES-54997 Mercenaries 2 - World in Flames <br />
SLES-55000 Mercenaries 2 - World in Flames <br />
SLES-55001 Mercenaries 2 - World in Flames <br />
SCES-55019 Ratchet & Clank - Size Matters <br />
SLES-54644 Valkyrie Profile 2 - Silmeria <br />
SLES-54645 Valkyrie Profile 2 - Silmeria <br />
SLES-54647 Valkyrie Profile 2 - Silmeria <br />
SLES-54646 Valkyrie Profile 2 - Silmeria <br />
SLES-54648 Valkyrie Profile 2 - Silmeria <br />
SLPS-25196 Venus & Braves - Majo to Megami to Horobi no Yogen <br />
SLPS-25195 Venus & Braves - Majo to Megami to Horobi no Yogen (Premium Box) <br />
SLPS-73236 Venus & Braves - Majo to Megami to Horobi no Yogen <br />
SCAJ-20012 Venus & Braves - Majo to Megami to Horobi no Yogen <br />
<br />
<br />
09/09/24 <br />
SCUS_976.57 MLB 11: The Show <br />
SLUS_208.07 Space Channel 5: Special Edition <br />
SLUS_209.14 Ribbit King Plus! (Bonus Disc) <br />
SLUS_215.63 Ribbit King Plus! (Bonus Disc) <br />
SLUS_217.04 Final Fantasy XI: Vana'diel Collection 2008 <br />
SLUS_217.05 Madden NFL 08 <br />
SLUS_217.13 Winter Sports 2008: The Ultimate Challenge <br />
SLUS_217.38 Nitrobike <br />
SLUS_217.64 Cake Mania: Baker's Challenge <br />
SLUS_217.86 Summer Athletics: The Ultimate Challenge <br />
SLUS_217.98 Shepherd's Crossing <br />
SLUS_218.41 Cabela's Dangerous Hunts 2009 <br />
SLUS_218.98 Band Hero <br />
SLUS_219.05 FIFA 10 <br />
SLUS_219.06 Cabela's Outdoor Adventures (2009) <br />
SLUS_219.07 Jurassic: The Hunted <br />
SLUS_219.09 DJ Hero <br />
SLUS_219.18 Pro Evolution Soccer 2010 <br />
SLUS_219.19 Backyard Football '10 <br />
SLUS_219.27 Sakura Wars: So Long, My Love (Disc 1) <br />
SLUS_219.30 Sakura Wars: So Long, My Love (Disc 2) <br />
SLUS_219.29 Major League Baseball 2K10 <br />
SLUS_219.31 Disney Pixar Toy Story 3 <br />
SLUS_219.32 NCAA Football 11 <br />
SLUS_219.33 Despicable Me: The Game <br />
SLUS_219.34 Rapala Pro Bass Fishing <br />
SLUS_219.35 Cabela's North American Adventures <br />
SLUS_219.36 NBA 2K11 <br />
SLUS_219.37 Madden NFL 11 <br />
SLUS_219.38 Ben 10 Ultimate Alien: Cosmic Destruction <br />
SLUS_219.41 FIFA 11 <br />
SLUS_219.42 Pro Evolution Soccer 2011 <br />
SLUS_219.45 Major League Baseball 2K11 <br />
SLUS_219.46 Madden NFL 12 <br />
SLUS_219.47 FIFA 12 <br />
SLUS_219.48 Pro Evolution Soccer 2012 <br />
SLUS_219.50 NBA 2K12 <br />
SLUS_219.51 Major League Baseball 2K12 <br />
<br />
Also corrected the formatting of the last database entries prior to 09/09/24 (SLPS-20318 to SCAJ-20012)
<br />
<br />
<p align="center">
  <img src="BatchRenamerLinux.png" alt="working in Kubuntu via WINE">
</p>
<br />
<br />
<br />
<br />
Yes, it works in linux.


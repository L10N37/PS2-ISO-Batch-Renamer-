# PS2-ISO-Batch-Renamer-

Just unzip the gameid.txt and ps2.exe file into your ps2 ISO folder.
No matter what the game is called (as long as it has extension *.iso) - it will be renamed appropriately.

Requires dotnet: https://dotnet.microsoft.com/en-us/download
Demo: https://www.youtube.com/watch?v=4r0DLwKOaX0

A few minor changes were made post demo video after testing with a much greater amount of games.
The initial versions were searching for 3 HEX values for "CNF". 
It appears some games have more than one ".CNF" file - the search was enlarged to "EM.CNF" and adjustments were made to work backwards from here to the SYSTEM.CNF location bytes.

I also realised Onimusha Dawn of dreams has 2 GAMEID's, just the GAMEID's used on the lookup table are doubled up on names. 
You could modify the 'gameid.txt' to suitably rename them as Disc 1 and Disc 2 if you know which is which.

(c) VajskiDs Consoles 2022

# V4.0.1 database audit

Date: 3 September 2026

## Result

The V4.0.0 database contained 13,791 records representing 13,740 unique game
IDs. There were 51 repeated records across 51 IDs:

- 16 repeated exactly the same title.
- 35 contained alternate names, edition labels, transliterations, or genuinely
  conflicting games.

V4.0.1 consolidates every repeated ID into one audited record. It also corrects
17 otherwise unique titles that contained Windows-forbidden filename
characters. The resulting built-in database contains:

- **13,740 records**
- **13,740 unique IDs**
- **0 duplicate IDs**
- **0 parsed titles containing Windows-forbidden `< > : " / \\ | ? *`
  characters**

No disc-identification logic was changed. External databases still retain the
established first-match-wins behavior.

## Reference method

Serial/title conflicts were checked against the current
[GameDB-PS2 dataset](https://github.com/niemasd/GameDB-PS2), whose source list
includes Redump, PlayStation DataCenter, GameFAQs, MiSTer Addons, and VGCollect.
The snapshot used for this audit was
[2026-07-16_20-23-50](https://github.com/niemasd/GameDB-PS2/releases/tag/2026-07-16_20-23-50).
PAL conflicts were also cross-checked against the
[PlayStation DataCenter PAL list](https://psxdatacenter.com/psx2/pal_list2.html).

Reference punctuation that is invalid in a Windows filename was normalized to
` - ` or omitted. Existing edition and disc qualifiers were retained where
they distinguish a physical release.

## Exact duplicate records removed

`SCED_515.37`, `SCES_546.23`, `SLAJ_250.35`, `SLES_546.23`,
`SLES_546.44`, `SLPM_623.20`, `SLPM_667.79`, `SLPM_669.17`,
`SLPS_203.18`, `SLUS_209.32`, `SLUS_210.33`, `SLUS_215.90`,
`SLUS_290.42`, `SLUS_290.55`, `SLUS_290.65`, and `SLUS_290.84`.

## Conflicting or alternate duplicate records consolidated

| Game ID | Retained V4.0.1 title |
| --- | --- |
| `PCPX_966.49` | Gran Turismo 4 - First Preview |
| `SCAJ_200.12` | Venus & Braves |
| `SCKA_200.58` | Action Romance Bumpy Trot |
| `SCKA_200.62` | Saru! Get You! 3 |
| `SLAJ_250.23` | Shin Sangoku Musou 3 - Moushouden |
| `SLES_503.82` | Silent Hill 2 |
| `SLES_512.27` | Lara Croft Tomb Raider - The Angel of Darkness |
| `SLES_512.52` | The Lord of the Rings - The Two Towers |
| `SLES_512.58` | 007 - Nightfire [Platinum] |
| `SLES_518.97` | The Simpsons - Hit & Run |
| `SLES_520.05` | 007 - Everything or Nothing |
| `SLES_525.88` | Mercenaries - Playground of Destruction |
| `SLES_525.90` | Mercenaries - Playground of Destruction |
| `SLES_528.01` | The Lord of the Rings - The Third Age |
| `SLES_529.74` | GoldenEye - Rogue Agent |
| `SLES_533.42` | Cricket 2005 |
| `SLES_533.57` | 21 Card Games |
| `SLES_533.66` | Killer7 |
| `SLES_540.16` | AND 1 Streetball |
| `SLES_541.47` | Ice Age 2 - The Meltdown |
| `SLES_542.09` | The Sopranos - Road to Respect |
| `SLES_543.47` | The Sims 2 - Pets |
| `SLES_548.15` | The Legend of Spyro - The Eternal Night |
| `SLES_549.98` | Mercenaries 2 - L'enfer des Favelas |
| `SLES_820.18` | Cy Girls (Ice Disc) |
| `SLES_820.20` | Cy Girls (Ice Disc) |
| `SLES_820.21` | Cy Girls (Aska Disc) |
| `SLKA_251.03` | Shinseiki Evangelion 2 - Evangelions |
| `SLKA_251.31` | Jyuouki - Project Altered Beast |
| `SLKA_253.17` | Jin Samguk Mussang 3 |
| `SLPM_650.40` | The Fear [Disc 1] |
| `SLPS_251.95` | Venus & Braves - Majo to Megami to Horobi no Yogen [Premium Box] |
| `SLPS_251.96` | Venus & Braves - Majo to Megami to Horobi no Yogen |
| `SLPS_254.85` | Kidou Senshi Gundam - Ver. 1.5 [Gundam the Best] |
| `SLPS_732.36` | Venus & Braves - Majo to Megami to Horobi no Yogen [PlayStation 2 The Best] |

## Windows filename-safety corrections

| Game ID | Filename-safe V4.0.1 title |
| --- | --- |
| `SLES_546.04` | ¡Qué Pasa Neng! El Videojuego |
| `SLPM_550.11` | Poi Hito Natsu no Keiken! [Limited Edition] |
| `SLPM_550.12` | Ppoi! Hito Natsu no Keiken! |
| `SLPM_551.32` | Kira Kira - Rock 'N' Roll Show [First Print Limited Edition] |
| `SLPM_551.33` | Kira Kira - Rock 'n' Roll Show |
| `SLPM_551.49` | Loveroot Zero - Kiss Kiss Labyrinth |
| `SLPM_552.79` | Otometeki Koi Kakumei - Love Revo!! [Best Edition] |
| `SLPM_552.88` | Shin Koihime Musou - Otome Ryouran Sangokushi Engi |
| `SLPM_620.93` | Simple 2000 Series Ultimate Vol. 1 - Love Smash! |
| `SLPS_200.61` | Mamimume Mogacho no Print Hour |
| `SLPS_258.83` | Katekyoo Hitman Reborn! Nerae! Ring x Vongola Trainers |
| `SLPS_258.99` | Katekyoo Hitman Reborn! Let's Ansatsu! Nerawareta 10-daime! [Best Collection] |
| `SLPS_259.09` | Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 13 - Shin Seiki Evangelion - Yakusoku no Toki |
| `SLPS_259.11` | Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 11 - Shin Seiki Evangelion - Magokoro o, Kimi ni |
| `SLPS_259.31` | Katekyoo Hitman Reborn! Nerae! Ring x Vongola Trainers [Best Collection] |
| `SLPS_259.42` | Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 14 - CR Shin Seiki Evangelion - Saigo no Shisha |
| `SLPS_259.43` | Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 14 - CR Shin Seiki Evangelion - Saigo no Shisha [Limited Special Box] |

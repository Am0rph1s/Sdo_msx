ProjName = "nau_dx_intro";
ProjModules = [ "nau_dx_intro" ];
LibModules = [ "system", "bios", "vdp", "print", "input", "psg" ];
Machine = "1";
Target = "ROM_ASCII16";
ROMSize = 64;
ProjSegments = "nau_dx_intro";
AppSignature = true;
AppCompany = "ND";
AppID = "N1";
Optim = "Speed";
RawFiles = [
    { segment: 1, file: "intro_sc2_raw.bin" },
    { segment: 2, file: "game_p1.bin" },
    { segment: 3, file: "game_p2.bin" }
];
Verbose = true;

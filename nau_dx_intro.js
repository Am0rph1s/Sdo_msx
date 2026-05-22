ProjName = "nau_dx_intro";
ProjModules = [ "nau_dx_intro" ];
LibModules = [ "system", "bios", "vdp", "print", "input", "psg" ];
Machine = "1";
Target = "ROM_KONAMI_SCC";
ROMSize = 64;
ProjSegments = "nau_dx_intro";
AppSignature = true;
AppCompany = "ND";
AppID = "N1";
Optim = "Speed";
RawFiles = [
    { segment: 1, file: "intro_sc2_raw_b1.bin" },
    { segment: 2, file: "intro_sc2_raw_b2.bin" },
    { segment: 3, file: "game_p1_b1.bin" },
    { segment: 4, file: "game_p1_b2.bin" },
    { segment: 5, file: "game_p2_b1.bin" },
    { segment: 6, file: "game_p2_b2.bin" }
];
Verbose = true;

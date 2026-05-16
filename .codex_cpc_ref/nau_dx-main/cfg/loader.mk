# Loader: overscan ConvImgCpc (LZW) + melodia AY, retorna a BASIC via RET.
# Flux split: RUN"DISC" → MEMORY → LOAD INTRO → CALL &8300 (RET) → LOAD "TRANSIT.BIN",&BB00 → CALL &BB00.
#       Monolític: RUN "SDO.BIN".
#
# DISC_HIDDEN=1: línia |USER 15 + dsk_hide_bins (CAT només DISC). Per defecte 0: alguns emuladors
# no implementen bé la RSX |USER — si RUN"DISC" falla, deixa 0 o posa DISC_HIDDEN := 1 només si ho has provat.
DISC_HIDDEN ?= 0
LOADER_REL  := $(OBJDIR)/loader.rel
LOADER_IHX  := $(OBJDIR)/loader.ihx
LOADER_MAP  := $(OBJDIR)/loader.map
LOADER_BIN  := $(OBJDIR)/loader.bin
LOADER_CAS_INC := $(OBJDIR)/loader_cas_entry.inc
DISC_BAS    := $(OBJDIR)/DISC

$(LOADER_REL): loader/loader.s loader/titol_data_converted.s | $(OBJDIR)
	$(Z80ASM) $(Z80ASMFLAGS) $(Z80CCINCLUDE) $(LOADER_REL) loader/loader.s

# --code-loc 0x8300: just per sobre de la pantalla overscan (que acaba a ~&7FFF/&8200).
# _DATA a &A500: just després de _CODE (~&A49x); gap mínim al .bin (no &A4xx→&B000).
# Si _CODE creix per sobre de &A4FF, puja aquesta adreça.
$(LOADER_IHX): $(LOADER_REL)
	$(Z80CC) -mz80 --no-std-crt0 --nostdlib -L$(Z80_SDCC_LIBDIR) -Wl-u -Wl-m \
	  --code-loc 0x8300 --data-loc 0xA500 -lz80 $(LOADER_REL) -o $(LOADER_IHX)

$(LOADER_CAS_INC): $(LOADER_IHX)
	@python3 -c "import re,sys;\
m=open('$(LOADER_MAP)','r',errors='ignore').read();\
r=re.search(r'^\s*([0-9A-Fa-f]+)\s+nau_dx_cas_jp4000\b', m, re.M);\
e=re.search(r'^\s*([0-9A-Fa-f]+)\s+nau_dx_cas_jp4000_end\b', m, re.M);\
sys.exit('nau_dx_cas_jp4000 missing from loader.map') if not r else None;\
sys.exit('nau_dx_cas_jp4000_end missing from loader.map') if not e else None;\
a=int(r.group(1),16); b=int(e.group(1),16); L=b-a;\
open('$(LOADER_CAS_INC)','w').write('; generated from loader.map\n.equ NAU_DX_CAS_ENTRY, #'+hex(a)+'\n.equ NAU_DX_CAS_LEN, #'+hex(L)+'\n')"

$(LOADER_BIN): $(LOADER_IHX)
	$(HEX2BIN) -p 00 $(LOADER_IHX) | $(TEE) $(LOADER_BIN).log

$(DISC_BAS): tools/make_disc_bas.py | $(OBJDIR)
	python3 tools/make_disc_bas.py $(DISC_BAS)

.PHONY: loader-dsk
# DISC: import com a BINARY (-t 1). iDSK usa -t 0=ASCII / -t 1=BINARY; amb -t 0 el tokenitzat queda malament.
loader-dsk: $(DSKINC) $(LOADER_BIN) $(DISC_BAS)
	@$(IDSK) $(DSK) -i $(DISC_BAS) -t 1 -f
	@$(IDSK) $(DSK) -i $(LOADER_BIN) -c 0x8300 -e 0x8300 -t 1 -f
ifeq ($(DISC_HIDDEN),1)
	python3 tools/dsk_hide_bins.py $(DSK)
	@echo "[nau] DSK: DISC (USER 0) + SDO/LOADER (USER 15) — CAT només DISC; RUN\"DISC\""
else
	@echo "[nau] DSK: DISC_HIDDEN=0 — tots els fitxers USER 0 (sense amagar al CAT)"
endif

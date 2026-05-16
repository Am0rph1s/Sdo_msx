/* Crides a TRANSIT.BIN resident (adreces fixes; veure Makefile + transit.map). */

#include "nau_shared.h"

#ifndef NAU_TRANSIT_LOAD_MENU
#define NAU_TRANSIT_LOAD_MENU  NAU_TRANSIT_ORG
#endif
#ifndef NAU_TRANSIT_LOAD_GAME
#define NAU_TRANSIT_LOAD_GAME  (NAU_TRANSIT_ORG + 6)
#endif

void nau_transit_load_menu(void) {
    typedef void (*tv)(void);
    ((tv)NAU_TRANSIT_LOAD_MENU)();
}

void nau_transit_load_game(void) {
    typedef void (*tv)(void);
    ((tv)NAU_TRANSIT_LOAD_GAME)();
}

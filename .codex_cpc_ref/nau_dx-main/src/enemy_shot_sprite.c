#include <cpctelera.h>
#include "enemy_shot_sprite.h"

u8 enemyshot_masked[ENEMYSHOT_NUM_TYPES][2 * ENEMYSHOT_W_BYTES * ENEMYSHOT_H_PIX]={
    {
        cpctm_px2byteM0(15,15), cpctm_px2byteM0(0,0),
        cpctm_px2byteM0(15,0),  cpctm_px2byteM0(0,6),
        cpctm_px2byteM0(15,0),  cpctm_px2byteM0(0,6),
        cpctm_px2byteM0(15,15), cpctm_px2byteM0(0,0)
    },
    {
        cpctm_px2byteM0(15,0), cpctm_px2byteM0(0,9),
        cpctm_px2byteM0(0,0),  cpctm_px2byteM0(9,9),
        cpctm_px2byteM0(0,0),  cpctm_px2byteM0(9,9),
        cpctm_px2byteM0(15,0), cpctm_px2byteM0(0,9)
    }
};

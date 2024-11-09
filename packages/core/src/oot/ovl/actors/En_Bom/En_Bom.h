#ifndef OVL_EN_BOM
#define OVL_EN_BOM

#include <combo.h>

struct EnBom;

typedef void (*EnBomActionFunc)(struct EnBom*, struct PlayState*);

typedef struct EnBom {
    /* 0x0000 */ Actor actor;
    /* 0x014C */ ColliderCylinder bombCollider;
    /* 0x0198 */ ColliderJntSph explosionCollider;
    /* 0x01B8 */ ColliderJntSphElement explosionColliderItems[1];
    /* 0x01F8 */ s16 timer;
    /* 0x01FA */ s16 flashSpeedScale;
    /* 0x01FC */ f32 flashIntensity;
    /* 0x0200 */ u8 colliderSetOC;
    /* 0x0204 */ EnBomActionFunc actionFunc;
} EnBom; // size = 0x0208

typedef enum EnBomType {
    /* 0x00 */ BOMB_BODY,
    /* 0x01 */ BOMB_EXPLOSION
} EnBomType;

#endif
#include <combo/xflags.h>
#include <combo/global.h>
#include "En_Butte.h"

#define FLAGS 0

#if defined(GAME_OOT)
# define GAMEPLAY_KEEP_DL_EFFECT_FLASH1         0x040105d0
# define GAMEPLAY_FIELD_KEEP_SKEL_BUTTERFLY     0x050036f0
# define GAMEPLAY_FIELD_KEEP_ANIM_BUTTERFLY     0x05002470
#endif

void EnButte_Init(Actor_EnButte* this, GameState_Play* play);
void EnButte_Destroy(Actor_EnButte* this, GameState_Play* play2);
void EnButte_Update(Actor_EnButte* this, GameState_Play* play);
void EnButte_Draw(Actor_EnButte* this, GameState_Play* play);

void EnButte_SetupFlyAround(Actor_EnButte* this);
void EnButte_FlyAround(Actor_EnButte* this, GameState_Play* play);
void EnButte_SetupFollowLink(Actor_EnButte* this);
void EnButte_FollowLink(Actor_EnButte* this, GameState_Play* play);
void EnButte_SetupTransformIntoFairy(Actor_EnButte* this);
void EnButte_TransformIntoFairy(Actor_EnButte* this, GameState_Play* play);
void EnButte_SetupWaitToDie(Actor_EnButte* this);
void EnButte_WaitToDie(Actor_EnButte* this, GameState_Play* play);

static s8 sQuickTransform;

static ColliderJntSphElementInit sJntSphElementsInit[] = {
    { {
          ELEMTYPE_UNK0,
          { 0x00000000, 0x00, 0x00 },
          { 0xFFCFFFFF, 0x000, 0x00 },
          ATELEM_NONE,
          ACELEM_NONE,
          OCELEM_ON,
      },
      { 0, { { 0, 0, 0 }, 5 }, 100 } },
};
static ColliderJntSphInit sColliderInit = {
    {
        COLTYPE_NONE,
        AT_NONE,
        AC_NONE,
        OC1_ON | OC1_TYPE_PLAYER | OC1_TYPE_1,
        OC2_TYPE_1,
        COLSHAPE_JNTSPH,
    },
    1,
    sJntSphElementsInit,
};

typedef struct EnButteFlightParams {
    /* 0x00 */ s16 minTime;
    /* 0x02 */ s16 maxTime;
    /* 0x04 */ f32 speedXZTarget;
    /* 0x08 */ f32 speedXZScale;
    /* 0x0C */ f32 speedXZStep;
    /* 0x10 */ s16 rotYStep;
} EnButteFlightParams; // size = 0x14

static EnButteFlightParams sFlyAroundParams[] = {
    { 5, 35, 0.0f, 0.1f, 0.5f, 0 },
    { 10, 45, 1.1f, 0.1f, 0.25f, 1000 },
    { 10, 40, 1.5f, 0.1f, 0.3f, 2000 },
};
static EnButteFlightParams sFollowLinkParams[] = {
    { 3, 3, 0.8f, 0.1f, 0.2f, 0 },
    { 10, 20, 2.0f, 0.3f, 1.0f, 0 },
    { 10, 20, 2.4f, 0.3f, 1.0f, 0 },
};

void EnButte_SelectFlightParams(Actor_EnButte* this, EnButteFlightParams* flightParams) {
    if (this->flightParamsIdx == 0) {
        if (Rand_ZeroOne() < 0.6f) {
            this->flightParamsIdx = 1;
        } else {
            this->flightParamsIdx = 2;
        }
    } else {
        this->flightParamsIdx = 0;
    }

    this->timer = Rand_S16Offset(flightParams->minTime, flightParams->maxTime);
}

static f32 sTransformationEffectScale = 0.0f;
static s16 sTransformationEffectAlpha = 0;

static void EnButte_Xflags(Xflag* xflag, Actor_EnButte* this, GameState_Play* play)
{
    xflag->id = this->actor.actorIndex;
    xflag->roomId = this->actor.room;
    xflag->sceneId = play->sceneId;
    xflag->setupId = g.sceneSetupId;
    xflag->sliceId = this->actor.params;
}

static int EnButte_IsShuffled(Actor_EnButte* this, GameState_Play* play)
{
    Xflag xflag;
    ComboItemQuery q;
    ComboItemOverride o;

    EnButte_Xflags(&xflag, this, play);
    if (comboXflagsGet(&xflag))
        return FALSE;
    comboXflagItemQuery(&q, &xflag, 0);
    comboItemOverride(&o, &q);
    if (o.gi == GI_NONE)
        return FALSE;
    return TRUE;
}

static int EnButte_CanTransform(Actor_EnButte* this, GameState_Play* play)
{
    if (this->actor.params == 0)
        return TRUE;
    if (EnButte_IsShuffled(this, play))
        return TRUE;
    return FALSE;
}

void EnButte_ResetTransformationEffect(void)
{
    sTransformationEffectScale = 0.0f;
    sTransformationEffectAlpha = 0;
}

void EnButte_UpdateTransformationEffect(void)
{
    sTransformationEffectScale += 0.003f;
    sTransformationEffectAlpha += 4000;
}

void EnButte_DrawTransformationEffect(Actor_EnButte* this, GameState_Play* play)
{
    static Vec3f D_809CE3C4 = { 0.0f, 0.0f, -3.0f };
    Vec3f sp5C;
    s32 alpha;
    Vec3s camDir;

    OPEN_DISPS(play->gs.gfx);
    Gfx_SetupDL25_Xlu(play->gs.gfx);

    alpha = Math_SinS(sTransformationEffectAlpha) * 250;
    alpha = CLAMP(alpha, 0, 255);

    camDir = Camera_GetCamDir(GET_ACTIVE_CAM(play));
    Matrix_RotateY(BINANG_TO_RAD(camDir.y), MAT_SET);
    Matrix_RotateX(BINANG_TO_RAD(camDir.x), MAT_MUL);
    Matrix_RotateZ(BINANG_TO_RAD(camDir.z), MAT_MUL);
    Matrix_MultVec3f(&D_809CE3C4, &sp5C);
    Matrix_SetTranslateRotateYXZ(this->actor.focus.pos.x + sp5C.x, this->actor.focus.pos.y + sp5C.y, this->actor.focus.pos.z + sp5C.z, &camDir);
    Matrix_Scale(sTransformationEffectScale, sTransformationEffectScale, sTransformationEffectScale, MAT_MUL);
    gSPMatrix(POLY_XLU_DISP++, GetMatrixMV(play->gs.gfx), G_MTX_NOPUSH | G_MTX_LOAD | G_MTX_MODELVIEW);
    gDPSetPrimColor(POLY_XLU_DISP++, 0x80, 0x80, 200, 200, 180, alpha);
    gDPSetEnvColor(POLY_XLU_DISP++, 200, 200, 210, 255);
    gSPDisplayList(POLY_XLU_DISP++, SEGMENTED_TO_VIRTUAL(GAMEPLAY_KEEP_DL_EFFECT_FLASH1));

    CLOSE_DISPS();
}

static InitChainEntry sInitChain[] = {
    ICHAIN_VEC3F_DIV1000(scale, 10, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneForward, 700, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneScale, 20, ICHAIN_CONTINUE),
    ICHAIN_F32(uncullZoneDownward, 600, ICHAIN_STOP),
};

void EnButte_Init(Actor_EnButte* this, GameState_Play* play)
{
    Actor_ProcessInitChain(&this->actor, sInitChain);

    if (EnButte_CanTransform(this, play))
    {
        this->actor.uncullZoneScale = 200.0f;
    }

    SkelAnime_Init(play, &this->skelAnime, (void*)GAMEPLAY_FIELD_KEEP_SKEL_BUTTERFLY, (void*)GAMEPLAY_FIELD_KEEP_ANIM_BUTTERFLY, this->jointTable, this->morphTable, 8);
    Collider_InitJntSph(play, &this->collider);
    Collider_SetJntSph(play, &this->collider, &this->actor, &sColliderInit, this->colliderItems);
    this->actor.colChkInfo.mass = 0;
    this->unk_25C = Rand_ZeroOne() * 0xFFFF;
    this->unk_25E = Rand_ZeroOne() * 0xFFFF;
    this->unk_260 = Rand_ZeroOne() * 0xFFFF;
    Animation_Change(&this->skelAnime, (void*)GAMEPLAY_FIELD_KEEP_ANIM_BUTTERFLY, 1.0f, 0.0f, 0.0f, ANIMMODE_LOOP_INTERP, 0.0f);
    EnButte_SetupFlyAround(this);
    this->actor.shape.rot.x -= 0x2320;
    this->drawSkelAnime = TRUE;
}

void EnButte_Destroy(Actor_EnButte* this, GameState_Play* play)
{
    Collider_DestroyJntSph(play, &this->collider);
}

void func_809CD56C(Actor_EnButte* this) {
    static f32 D_809CE3E0[] = { 50.0f, 80.0f, 100.0f };
    static f32 D_809CE3EC[] = { 30.0f, 40.0f, 50.0f };

    this->actor.shape.yOffset += Math_SinS(this->unk_25C) * D_809CE3E0[this->flightParamsIdx] +
                                 Math_SinS(this->unk_25E) * D_809CE3EC[this->flightParamsIdx];
    this->actor.shape.yOffset = CLAMP(this->actor.shape.yOffset, -2000.0f, 2000.0f);
}

void func_809CD634(Actor_EnButte* this) {
    static f32 D_809CE3F8[] = { 15.0f, 20.0f, 25.0f };
    static f32 D_809CE404[] = { 7.5f, 10.0f, 12.5f };

    this->actor.shape.yOffset += Math_SinS(this->unk_25C) * D_809CE3F8[this->flightParamsIdx] +
                                 Math_SinS(this->unk_25E) * D_809CE404[this->flightParamsIdx];
    this->actor.shape.yOffset = CLAMP(this->actor.shape.yOffset, -500.0f, 500.0f);
}

void EnButte_Turn(Actor_EnButte* this) {
    s16 target = this->actor.world.rot.y + 0x8000;
    s16 diff = target - this->actor.shape.rot.y;

    Math_ScaledStepToS(&this->actor.shape.rot.y, target, ABS(diff) >> 3);
    this->actor.shape.rot.x = (s16)(sinf(this->unk_260) * 600.0f) - 0x2320;
}

void EnButte_SetupFlyAround(Actor_EnButte* this) {
    EnButte_SelectFlightParams(this, &sFlyAroundParams[this->flightParamsIdx]);
    this->actionFunc = EnButte_FlyAround;
}

void EnButte_FlyAround(Actor_EnButte* this, GameState_Play* play) {
    EnButteFlightParams* flightParams = &sFlyAroundParams[this->flightParamsIdx];
    s16 yaw;
    f32 distSqFromHome;
    f32 maxDistSqFromHome;
    f32 minAnimSpeed;
    f32 animSpeed;
    s16 rotStep;
    Actor_Player* player;

    player = GET_PLAYER(play);
    distSqFromHome = Math3D_Dist2DSq(this->actor.world.pos.x, this->actor.world.pos.z, this->actor.home.pos.x,
                                     this->actor.home.pos.z);
    func_809CD56C(this);
    Math_SmoothStepToF(&this->actor.speed, flightParams->speedXZTarget, flightParams->speedXZScale,
                       flightParams->speedXZStep, 0.0f);

    if (this->unk_257 == 1) {
        maxDistSqFromHome = SQ(100.0f);
        rotStep = 1000;
    } else {
        maxDistSqFromHome = SQ(35.0f);
        rotStep = 600;
    }

    minAnimSpeed = 0.0f;
    this->posYTarget = this->actor.home.pos.y;

    if ((this->flightParamsIdx != 0) && ((distSqFromHome > maxDistSqFromHome) || (this->timer < 4))) {
        yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->actor.home.pos);
        if (Math_ScaledStepToS(&this->actor.world.rot.y, yaw, flightParams->rotYStep) == 0) {
            minAnimSpeed = 0.5f;
        }
    } else if ((this->unk_257 == 0) && (this->actor.child != NULL) && (this->actor.child != &this->actor)) {
        yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &this->actor.child->world.pos);
        if (Math_ScaledStepToS(&this->actor.world.rot.y, yaw, rotStep) == 0) {
            minAnimSpeed = 0.3f;
        }
    } else if (this->unk_257 == 1) {
        yaw = this->actor.yawTowardsPlayer + 0x8000 + (s16)((Rand_ZeroOne() - 0.5f) * 0x6000);
        if (Math_ScaledStepToS(&this->actor.world.rot.y, yaw, rotStep) == 0) {
            minAnimSpeed = 0.4f;
        }
    } else {
        this->actor.world.rot.y += (s16)(sinf(this->unk_25C) * 100.0f);
    }

    EnButte_Turn(this);

    animSpeed = this->actor.speed / 2.0f + Rand_ZeroOne() * 0.2f + (1.0f - Math_SinS(this->unk_260)) * 0.15f +
                (1.0f - Math_SinS(this->unk_25E)) * 0.3f + minAnimSpeed;
    this->skelAnime.playSpeed = CLAMP(animSpeed, 0.2f, 1.5f);
    SkelAnime_Update(&this->skelAnime);

    if (this->timer <= 0) {
        EnButte_SelectFlightParams(this, &sFlyAroundParams[this->flightParamsIdx]);
    }

    if (EnButte_CanTransform(this, play) && (player->heldItemAction == 0x06 /*PLAYER_IA_DEKU_STICK/*/) &&
        (this->swordDownTimer <= 0) &&
        ((Math3D_Dist2DSq(player->actor.world.pos.x, player->actor.world.pos.z, this->actor.home.pos.x,
                          this->actor.home.pos.z) < SQ(120.0f)) ||
         (this->actor.xzDistToPlayer < 60.0f))) {
        EnButte_SetupFollowLink(this);
        this->unk_257 = 2;
    } else if (this->actor.xzDistToPlayer < 120.0) {
        this->unk_257 = 1;
    } else {
        this->unk_257 = 0;
    }
}

void EnButte_SetupFollowLink(Actor_EnButte* this) {
    EnButte_SelectFlightParams(this, &sFollowLinkParams[this->flightParamsIdx]);
    this->actionFunc = EnButte_FollowLink;
}

void EnButte_FollowLink(Actor_EnButte* this, GameState_Play* play) {
    static s32 D_809CE410 = 1500;
    EnButteFlightParams* flightParams = &sFollowLinkParams[this->flightParamsIdx];
    f32 distSqFromHome;
    Vec3f swordTip;
    f32 animSpeed;
    f32 minAnimSpeed;
    f32 distSqFromSword;
    s16 yaw;
    Actor_Player* player;

    player = GET_PLAYER(play);
    func_809CD634(this);
    Math_SmoothStepToF(&this->actor.speed, flightParams->speedXZTarget, flightParams->speedXZScale,
                       flightParams->speedXZStep, 0.0f);
    minAnimSpeed = 0.0f;

    if ((this->flightParamsIdx != 0) && (this->timer < 12)) {
        swordTip.x = player->meleeWeaponInfo[0].tip.x + Math_SinS(player->actor.shape.rot.y) * 10.0f;
        swordTip.y = player->meleeWeaponInfo[0].tip.y;
        swordTip.z = player->meleeWeaponInfo[0].tip.z + Math_CosS(player->actor.shape.rot.y) * 10.0f;

        yaw = Math_Vec3f_Yaw(&this->actor.world.pos, &swordTip) + (s16)(Rand_ZeroOne() * D_809CE410);
        if (Math_ScaledStepToS(&this->actor.world.rot.y, yaw, 2000) != 0) {
            if (play->gameplayFrames % 2) {
                this->actor.world.rot.y += (s16)(sinf(this->unk_25C) * 60.0f);
            }
        } else {
            minAnimSpeed = 0.3f;
        }
    }

    this->posYTarget = MAX(player->actor.world.pos.y + 30.0f, player->meleeWeaponInfo[0].tip.y);

    EnButte_Turn(this);

    animSpeed = this->actor.speed / 2.0f + Rand_ZeroOne() * 0.2f + (1.0f - Math_SinS(this->unk_260)) * 0.15f +
                (1.0f - Math_SinS(this->unk_25E)) * 0.3f + minAnimSpeed;
    this->skelAnime.playSpeed = CLAMP(animSpeed, 0.2f, 1.5f);
    SkelAnime_Update(&this->skelAnime);

    if (this->timer <= 0) {
        EnButte_SelectFlightParams(this, &sFollowLinkParams[this->flightParamsIdx]);
        D_809CE410 = -D_809CE410;
    }

    distSqFromHome = Math3D_Dist2DSq(this->actor.world.pos.x, this->actor.world.pos.z, this->actor.home.pos.x,
                                     this->actor.home.pos.z);
    if (!((player->heldItemAction == 0x06) && (fabsf(player->actor.speed) < 1.8f) &&
          (this->swordDownTimer <= 0) && (distSqFromHome < SQ(320.0f)))) {
        EnButte_SetupFlyAround(this);
    } else if (distSqFromHome > SQ(240.0f) || sQuickTransform) {
        distSqFromSword = Math3D_Dist2DSq(player->meleeWeaponInfo[0].tip.x, player->meleeWeaponInfo[0].tip.z,
                                          this->actor.world.pos.x, this->actor.world.pos.z);
        if (distSqFromSword < SQ(60.0f)) {
            EnButte_SetupTransformIntoFairy(this);
        }
    }
}

void EnButte_SetupTransformIntoFairy(Actor_EnButte* this)
{
    this->timer = 9;
    this->actor.flags |= ACTOR_FLAG_OOT_4;
    this->skelAnime.playSpeed = 1.0f;
    EnButte_ResetTransformationEffect();
    this->actionFunc = EnButte_TransformIntoFairy;
}

void EnButte_TransformIntoFairy(Actor_EnButte* this, GameState_Play* play)
{
    SkelAnime_Update(&this->skelAnime);
    EnButte_UpdateTransformationEffect();

    if (this->timer == 5)
    {
        SfxSource_PlaySfxAtFixedWorldPos(play, &this->actor.world.pos, 60, NA_SE_EV_BUTTERFRY_TO_FAIRY);
    }
    else if (this->timer == 4)
    {
        g.actorIndex = this->actor.actorIndex;
        g.actorSliceId = this->actor.params;
        Actor_Spawn(&play->actorCtx, play, AC_EN_ELF, this->actor.focus.pos.x, this->actor.focus.pos.y, this->actor.focus.pos.z, 0, this->actor.shape.rot.y, 0, 0x0002);
        g.actorIndex = 0xff;
        g.actorSliceId = 0;
        this->drawSkelAnime = FALSE;
    }
    else if (this->timer <= 0)
    {
        EnButte_SetupWaitToDie(this);
    }
}

void EnButte_SetupWaitToDie(Actor_EnButte* this) {
    this->timer = 64;
    this->actionFunc = EnButte_WaitToDie;
    this->actor.draw = NULL;
}

void EnButte_WaitToDie(Actor_EnButte* this, GameState_Play* play) {
    if (this->timer <= 0) {
        Actor_Kill(&this->actor);
    }
}

void EnButte_Update(Actor_EnButte* this, GameState_Play* play)
{
    sQuickTransform = (u8)EnButte_IsShuffled(this, play);

    if ((this->actor.child != NULL) && (this->actor.child->update == NULL) && (this->actor.child != &this->actor)) {
        this->actor.child = NULL;
    }

    if (this->timer > 0) {
        this->timer--;
    }

    this->unk_25C += 0x222;
    this->unk_25E += 0x1000;
    this->unk_260 += 0x600;

    if (EnButte_CanTransform(this, play)) {
        if (GET_PLAYER(play)->meleeWeaponState == 0) {
            if (this->swordDownTimer > 0) {
                if (sQuickTransform)
                    this->swordDownTimer = 0;
                else
                    this->swordDownTimer--;
            }
        } else {
            this->swordDownTimer = 80;
        }
    }

    this->actionFunc(this, play);

    if (this->actor.update != NULL) {
        Actor_MoveWithGravity(&this->actor);
        Math_StepToF(&this->actor.world.pos.y, this->posYTarget, 0.6f);
        if (this->actor.xyzDistToPlayerSq < 5000.0f) {
            CollisionCheck_SetOC(play, &play->colChkCtx, &this->collider.base);
        }
        Actor_SetFocus(&this->actor, this->actor.shape.yOffset * this->actor.scale.y);
    }
}

void EnButte_Draw(Actor_EnButte* this, GameState_Play* play)
{
    if (this->drawSkelAnime) {
        Gfx_SetupDL25_Opa(play->gs.gfx);
        SkelAnime_DrawOpa(play, this->skelAnime.skeleton, this->skelAnime.jointTable, NULL, NULL, NULL);
        Collider_UpdateSpheres(0, &this->collider);
    }

    if (EnButte_CanTransform(this, play) && (this->actionFunc == EnButte_TransformIntoFairy)) {
        EnButte_DrawTransformationEffect(this, play);
    }
}

static ActorInit EnButte_ActorInit =
{
    AC_EN_BUTTE,
    ACTORCAT_ITEMACTION,
    FLAGS,
    OBJECT_GAMEPLAY_FIELD_KEEP,
    sizeof(Actor_EnButte),
    (ActorFunc)EnButte_Init,
    (ActorFunc)EnButte_Destroy,
    (ActorFunc)EnButte_Update,
    (ActorFunc)EnButte_Draw,
};

OVL_ACTOR_INFO(AC_EN_BUTTE, EnButte_ActorInit);

#include <combo.h>
#include <combo/config.h>

void EnBomBowlMan_AfterHandler(void)
{
    if (gSave.info.inventory.upgrades.bombBag || (Config_Flag(CFG_OOT_BOMBCHU_BAG) && gOotSave.info.inventory.items[ITS_OOT_BOMBCHU] == ITEM_OOT_BOMBCHU_10))
        SetEventChk(EV_OOT_CHK_BOMBCHU_BOWLING);
}

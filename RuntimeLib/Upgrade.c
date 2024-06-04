#include "Runtime.h"
#include "Upgrade.h"

Int32 RTItemUpgradeNormal(
    RTRuntimeRef Runtime,
    RTItemSlotRef Item,
    Int32 UpgradeType,
    Int32* UpgradePoint,
    Int32* Seed
) {
    RTItemDataRef ItemData = RTRuntimeGetItemDataByIndex(Runtime, Item->Item.ID);
    if (!ItemData) return RUNTIME_UPGRADE_RESULT_ERROR;
    
    RTDataUpgradeRateRef UpgradeRate = RTRuntimeDataUpgradeRateGet(Runtime->Context, UpgradeType);
    if (!UpgradeRate) return RUNTIME_UPGRADE_RESULT_ERROR;

    RTDataUpgradeRateValueRef UpgradeRateValue = RTRuntimeDataUpgradeRateValueGet(UpgradeRate, Item->Item.UpgradeLevel);
    if (!UpgradeRateValue) return RUNTIME_UPGRADE_RESULT_ERROR;

    Int32 TotalRate = 0;
    for (Int32 Index = 0; Index < RUNTIME_DATA_UPGRADE_RATE_VALUE_COUNT; Index += 1) {
        TotalRate += UpgradeRateValue->Rates[Index];
    }

    /*
    Int32 UpgradePointRate = 3000 * (*UpgradePoint) / (3000 + (*UpgradePoint));
    TotalRate += UpgradePointRate;
    */

    Int32 Value = RandomRange(Seed, 0, MAX(0, TotalRate - 1));
    Int32 CurrentRate = 0;

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_UPGRADE_RATE_TYPE_UPGRADE_1];
    if (Value < CurrentRate) {
        *UpgradePoint = 0;
        Item->Item.UpgradeLevel += 1;
        return RUNTIME_UPGRADE_RESULT_UPGRADE_1;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_UPGRADE_RATE_TYPE_UPGRADE_2];
    if (Value < CurrentRate) {
        *UpgradePoint = 0;
        Item->Item.UpgradeLevel += 2;
        return RUNTIME_UPGRADE_RESULT_UPGRADE_2;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_UPGRADE_RATE_TYPE_DOWNGRADE_0];
    if (Value < CurrentRate) {
        // TODO: Calculate the item upgrade level based point addition
        *UpgradePoint += Item->Item.UpgradeLevel * 100;
        return RUNTIME_UPGRADE_RESULT_DOWNGRADE_0;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_UPGRADE_RATE_TYPE_DOWNGRADE_1];
    if (Value < CurrentRate) {
        // TODO: Calculate the item upgrade level based point addition
        *UpgradePoint += Item->Item.UpgradeLevel * 100;
        Item->Item.UpgradeLevel -= 1;
        return RUNTIME_UPGRADE_RESULT_DOWNGRADE_1;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_UPGRADE_RATE_TYPE_DOWNGRADE_2];
    if (Value < CurrentRate) {
        // TODO: Calculate the item upgrade level based point addition
        *UpgradePoint += Item->Item.UpgradeLevel * 100;
        Item->Item.UpgradeLevel -= 2;
        return RUNTIME_UPGRADE_RESULT_DOWNGRADE_2;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_UPGRADE_RATE_TYPE_DOWNGRADE_RESET];
    if (Value <= CurrentRate) {
        // TODO: Calculate the item upgrade level based point addition
        *UpgradePoint += Item->Item.UpgradeLevel * 100;
        Item->Item.UpgradeLevel -= 3;
        return RUNTIME_UPGRADE_RESULT_DOWNGRADE_3;
    }

    Warn("Random range calculation should always result in perfect ranged matches!");
    return RUNTIME_UPGRADE_RESULT_ERROR;
}

Int32 RTItemUpgradeDivine(
    RTRuntimeRef Runtime,
    RTItemSlotRef Item,
    Int32* Seed
) {
    RTItemDataRef ItemData = RTRuntimeGetItemDataByIndex(Runtime, Item->Item.ID);
    if (!ItemData) return RUNTIME_DIVINE_UPGRADE_RESULT_ERROR;

    RTDataDivineUpgradeMainRef UpgradeMain = RTRuntimeDataDivineUpgradeMainGet(Runtime->Context, ItemData->ItemGrade, ItemData->ItemType);
    if (!UpgradeMain) return RUNTIME_DIVINE_UPGRADE_RESULT_ERROR;

    RTDataDivineUpgradeGroupRateRef UpgradeGroupRate = RTRuntimeDataDivineUpgradeGroupRateGet(Runtime->Context, UpgradeMain->Group);
    if (!UpgradeGroupRate) return RUNTIME_DIVINE_UPGRADE_RESULT_ERROR;

    RTDataDivineUpgradeGroupRateLevelRef UpgradeRateValue = RTRuntimeDataDivineUpgradeGroupRateLevelGet(UpgradeGroupRate, Item->Item.DivineLevel);
    if (!UpgradeRateValue) return RUNTIME_DIVINE_UPGRADE_RESULT_ERROR;

    Int32 TotalRate = 0;
    for (Int32 Index = 0; Index < RUNTIME_DATA_UPGRADE_RATE_VALUE_COUNT; Index += 1) {
        TotalRate += UpgradeRateValue->Rates[Index];
    }

    Int32 Value = RandomRange(Seed, 0, MAX(0, TotalRate - 1));
    Int32 CurrentRate = 0;

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_DIVINE_UPGRADE_RATE_TYPE_UPGRADE_1];
    if (Value < CurrentRate) {
        Item->Item.DivineLevel += 1;
        return RUNTIME_DIVINE_UPGRADE_RESULT_UPGRADE;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_DIVINE_UPGRADE_RATE_TYPE_DOWNGRADE_0];
    if (Value < CurrentRate) {
        return RUNTIME_DIVINE_UPGRADE_RESULT_DESTROYED;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_DIVINE_UPGRADE_RATE_TYPE_DOWNGRADE_RESET];
    if (Value < CurrentRate) {
        if (Item->Item.DivineLevel < 6) {
            Item->Item.DivineLevel = 0;
            return RUNTIME_DIVINE_UPGRADE_RESULT_DESTROYED;
        }

        return RUNTIME_DIVINE_UPGRADE_RESULT_DOWNGRADE;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_DIVINE_UPGRADE_RATE_TYPE_DOWNGRADE_1];
    if (Value < CurrentRate) {
        Item->Item.DivineLevel -= 1;
        return RUNTIME_DIVINE_UPGRADE_RESULT_DOWNGRADE;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_DIVINE_UPGRADE_RATE_TYPE_DOWNGRADE_2];
    if (Value < CurrentRate) {
        Item->Item.DivineLevel -= 2;
        return RUNTIME_DIVINE_UPGRADE_RESULT_DOWNGRADE;
    }

    CurrentRate += UpgradeRateValue->Rates[RUNTIME_DIVINE_UPGRADE_RATE_TYPE_DOWNGRADE_3];
    if (Value <= CurrentRate) {
        Item->Item.DivineLevel -= 3;
        return RUNTIME_DIVINE_UPGRADE_RESULT_DOWNGRADE;
    }

    Warn("Random range calculation should always result in perfect ranged matches!");
    return RUNTIME_DIVINE_UPGRADE_RESULT_ERROR;
}

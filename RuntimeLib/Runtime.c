#include "Character.h"
#include "Force.h"
#include "Mob.h"
#include "PartyManager.h"
#include "Runtime.h"
#include "Script.h"
#include "World.h"
#include "WorldManager.h"
#include "NotificationProtocol.h"
#include "NotificationManager.h"

RTRuntimeRef RTRuntimeCreate(
    AllocatorRef Allocator,
    Index MaxPartyCount,
    Void* UserData
) {
    RTRuntimeRef Runtime = (RTRuntimeRef)AllocatorAllocate(Allocator, sizeof(struct _RTRuntime));
    if (!Runtime) Fatal("Memory allocation failed!");
    memset(Runtime, 0, sizeof(struct _RTRuntime));

    for (Int32 X = 0; X < RUNTIME_MOVEMENT_MAX_DISTANCE_LENGTH; X++) {
        for (Int32 Y = 0; Y < RUNTIME_MOVEMENT_MAX_DISTANCE_LENGTH; Y++) {
            Int32 Index = X + Y * RUNTIME_MOVEMENT_MAX_DISTANCE_LENGTH;
            Runtime->MovementDistanceCache[Index] = (Float32)sqrt((Float64)X * (Float64)X + (Float64)Y * (Float64)Y);
        }
    }

    Runtime->Context = RTRuntimeDataContextCreate();
    Runtime->PartyManager = RTPartyManagerCreate(Allocator, MaxPartyCount);
    Runtime->ScriptManager = RTScriptManagerCreate(Runtime, RUNTIME_MEMORY_MAX_SCRIPT_COUNT);
    Runtime->WorldManager = RTWorldManagerCreate(
        Runtime,
        RUNTIME_MEMORY_MAX_WORLD_DATA_COUNT,
        RUNTIME_MEMORY_MAX_GLOBAL_WORLD_CONTEXT_COUNT,
        RUNTIME_MEMORY_MAX_PARTY_WORLD_CONTEXT_COUNT,
        RUNTIME_MEMORY_MAX_CHARACTER_COUNT
    );
    Runtime->NotificationManager = RTNotificationManagerCreate(Runtime);
    Runtime->SkillDataPool = MemoryPoolCreate(Allocator, sizeof(struct _RTCharacterSkillData), RUNTIME_MEMORY_MAX_CHARACTER_SKILL_DATA_COUNT);
    Runtime->ForceEffectFormulaPool = MemoryPoolCreate(Allocator, sizeof(struct _RTForceEffectFormula), RUNTIME_FORCE_EFFECT_COUNT);
    Runtime->UserData = UserData;
    return Runtime;
}

Void RTRuntimeDestroy(
    RTRuntimeRef Runtime
) {
    MemoryPoolDestroy(Runtime->ForceEffectFormulaPool);
    MemoryPoolDestroy(Runtime->SkillDataPool);
    RTNotificationManagerDestroy(Runtime->NotificationManager);
    RTScriptManagerDestroy(Runtime->ScriptManager);
    RTRuntimeDataContextDestroy(Runtime->Context);
    RTWorldManagerDestroy(Runtime->WorldManager);
    AllocatorDeallocate(Runtime->Allocator, Runtime);
}

Bool RTRuntimeLoadData(
    RTRuntimeRef Runtime,
    CString RuntimeDataPath,
    CString ServerDataPath
) {
    RTRuntimeInitForceEffectFormulas(Runtime);
 
    return RTRuntimeDataContextLoad(Runtime->Context, RuntimeDataPath, ServerDataPath);
}

Void RTRuntimeUpdate(
    RTRuntimeRef Runtime
) {
    RTWorldManagerUpdate(Runtime->WorldManager);
    /* Movement Debugging
    for (Int32 Index = 0; Index < Runtime->Characters.Count; Index++) {
        RTCharacterRef Character = (RTCharacterRef)ArrayGetElementAtIndex(&Runtime->Characters, Index);
        if (Character->Movement.Flags & RUNTIME_MOVEMENT_IS_DEAD_RECKONING) {
            RTMovementUpdateDeadReckoning(Runtime, &Character->Movement);
        }
    }
    */
}

RTWorldContextRef RTRuntimeGetWorldByID(
    RTRuntimeRef Runtime,
    Int32 WorldID
) {
    return RTWorldContextGetGlobal(Runtime->WorldManager, WorldID);
}

RTWorldContextRef RTRuntimeGetWorldByParty(
    RTRuntimeRef Runtime,
    RTEntityID PartyID
) {
    return RTWorldContextGetParty(Runtime->WorldManager, PartyID);
}

RTWorldContextRef RTRuntimeGetWorldByCharacter(
    RTRuntimeRef Runtime,
    RTCharacterRef Character
) {
    if (!RTEntityIsNull(Character->PartyID)) {
        RTWorldContextRef WorldContext = RTWorldContextGetParty(Runtime->WorldManager, Character->PartyID);
        if (WorldContext && WorldContext->WorldData->WorldIndex == Character->Info.Position.WorldID) return WorldContext;
    }

    return RTWorldContextGetGlobal(Runtime->WorldManager, Character->Info.Position.WorldID);
}

RTNpcRef RTRuntimeGetNpcByWorldNpcID(
    RTRuntimeRef Runtime,
    Int32 WorldID,
    Int32 NpcID
) {
    for (Int32 Index = 0; Index < Runtime->NpcCount; Index++) {
        RTNpcRef Npc = &Runtime->Npcs[Index];
        if (Npc->WorldID == WorldID && Npc->ID == NpcID) return Npc;
    }

    return NULL;
}

RTWarpRef RTRuntimeGetWarpByWorldNpcID(
    RTRuntimeRef Runtime,
    Int32 WorldID,
    Int32 NpcID,
    Int32 WarpIndex
) {
    for (Int32 Index = 0; Index < Runtime->WarpCount; Index++) {
        RTWarpRef Warp = &Runtime->Warps[Index];
        if (Warp->WorldID == WorldID && Warp->NpcID == NpcID && Warp->Index == WarpIndex) return Warp;
    }

    return NULL;
}

RTWarpRef RTRuntimeGetWarpByIndex(
    RTRuntimeRef Runtime,
    Int32 WorldID,
    Int32 WarpIndex
) {
    if (WarpIndex < 0 || WarpIndex >= Runtime->WarpCount) return NULL;

    return &Runtime->Warps[WarpIndex];
}

RTPartyRef RTRuntimeGetParty(
    RTRuntimeRef Runtime,
    RTEntityID Entity
) {
    return RTPartyManagerGetParty(Runtime->PartyManager, Entity);
}

RTWorldItemRef RTRuntimeGetItem(
    RTRuntimeRef Runtime,
    RTEntityID Entity
) {
    assert(Entity.EntityType == RUNTIME_ENTITY_TYPE_ITEM);

    RTWorldContextRef World = RTRuntimeGetWorldByID(Runtime, Entity.WorldIndex);
    if (!World) return NULL;

    return RTWorldGetItemByEntity(Runtime, World, Entity);
}

RTItemDataRef RTRuntimeGetItemDataByIndex(
    RTRuntimeRef Runtime,
    Int32 ItemIndex
) {
    UInt32 ItemID = (ItemIndex & RUNTIME_ITEM_MASK_INDEX);
    for (Int32 Index = 0; Index < Runtime->ItemDataCount; Index++) {
        RTItemDataRef Item = &Runtime->ItemData[Index];
        if (Item->ItemID == ItemID) {
            return Item;
        }
    }

    return NULL;
}

RTQuestDataRef RTRuntimeGetQuestByIndex(
    RTRuntimeRef Runtime,
    Int32 QuestIndex
) {
    for (Int32 Index = 0; Index < Runtime->QuestDataCount; Index++) {
        RTQuestDataRef Quest = &Runtime->QuestData[Index];
        if (Quest->ID == QuestIndex) {
            return Quest;
        }
    }

    return NULL;
}

RTQuestRewardItemSetDataRef RTRuntimeGetQuestRewardItemSetByIndex(
    RTRuntimeRef Runtime,
    Int32 ItemSetIndex
) {
    if (ItemSetIndex < 1) return NULL;

    for (Int32 Index = 0; Index < Runtime->QuestRewardItemSetDataCount; Index++) {
        RTQuestRewardItemSetDataRef ItemSet = &Runtime->QuestRewardItemSetData[Index];
        if (ItemSet->ID == ItemSetIndex) {
            return ItemSet;
        }
    }

    return NULL;
}

RTQuestRewardItemDataRef RTRuntimeGetQuestRewardItemByIndex(
    RTRuntimeRef Runtime,
    Int32 ItemSetIndex,
    Int32 ItemIndex,
    Int32 BattleStyleIndex
) {
    RTQuestRewardItemSetDataRef ItemSet = RTRuntimeGetQuestRewardItemSetByIndex(Runtime, ItemSetIndex);
    if (!ItemSet) return NULL;

    for (Int32 Index = 0; Index < ItemSet->Count; Index++) {
        RTQuestRewardItemDataRef Item = &ItemSet->Items[Index];
        if (Item->Index == ItemIndex && (Item->BattleStyleIndex == BattleStyleIndex || Item->BattleStyleIndex == 0)) {
            return Item;
        }
    }

    return NULL;
}

RTShopDataRef RTRuntimeGetShopByWorldNpcID(
    RTRuntimeRef Runtime,
    Int32 WorldID,
    Int32 NpcID
) {
    for (Int32 Index = 0; Index < Runtime->ShopDataCount; Index++) {
        RTShopDataRef Shop = &Runtime->ShopData[Index];
        if (Shop->WorldID == WorldID && Shop->NpcID == NpcID) {
            return Shop;
        }
    }

    return NULL;
}

RTShopDataRef RTRuntimeGetShopByIndex(
    RTRuntimeRef Runtime,
    Int32 ShopIndex
) {
    for (Int32 Index = 0; Index < Runtime->ShopDataCount; Index++) {
        RTShopDataRef Shop = &Runtime->ShopData[Index];
        if (Shop->Index == ShopIndex) {
            return Shop;
        }
    }

    return NULL;
}

RTTrainerDataRef RTRuntimeGetTrainerByWorldNpcID(
    RTRuntimeRef Runtime,
    Int32 WorldID,
    Int32 NpcID
) {
    for (Int32 Index = 0; Index < Runtime->TrainerDataCount; Index++) {
        RTTrainerDataRef Trainer = &Runtime->TrainerData[Index];
        if (Trainer->WorldID == WorldID && Trainer->NpcID == NpcID) {
            return Trainer;
        }
    }

    return NULL;
}

RTCharacterSkillDataRef RTRuntimeGetCharacterSkillDataByID(
    RTRuntimeRef Runtime,
    Int32 SkillID
) {
    return MemoryPoolFetch(Runtime->SkillDataPool, SkillID);
}

RTSkillLevelDataRef RTRuntimeGetSkillLevelDataByID(
    RTRuntimeRef Runtime,
    Int32 SkillID,
    Int32 SkillLevel
) {
    RTCharacterSkillDataRef SkillData = RTRuntimeGetCharacterSkillDataByID(Runtime, SkillID);
    if (!SkillData) return NULL;

    for (Int32 Index = 0; Index < SkillData->SkillLevelCount; Index++) {
        if (SkillData->SkillLevels[Index].StartLevel <= SkillLevel && SkillLevel <= SkillData->SkillLevels[Index].EndLevel) {
            return &SkillData->SkillLevels[Index];
        }
    }

    return NULL;
}

RTBattleStyleLevelFormulaDataRef RTRuntimeGetBattleStyleLevelFormulaData(
    RTRuntimeRef Runtime,
    Int32 BattleStyleIndex
) {
    assert(RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MIN <= BattleStyleIndex && BattleStyleIndex <= RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MAX);

    return &Runtime->BattleStyleLevelFormulaData[BattleStyleIndex - 1];

}

RTBattleStyleClassFormulaDataRef RTRuntimeGetBattleStyleClassFormulaData(
    RTRuntimeRef Runtime,
    Int32 BattleStyleIndex
) {
    assert(RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MIN <= BattleStyleIndex && BattleStyleIndex <= RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MAX);

    return &Runtime->BattleStyleClassFormulaData[BattleStyleIndex - 1];
}

RTBattleStyleSlopeDataRef RTRuntimeGetBattleStyleSlopeData(
    RTRuntimeRef Runtime,
    Int32 SlopeID,
    Int32 Penalty
) {
    RTBattleStyleSlopeFormulaDataRef Formula = RTRuntimeGetBattleStyleSlopeFormulaData(Runtime, SlopeID);
    assert(Formula && Formula->SlopeCount > 0);

    Int32 SlopePenalty = 0;
    Int32 SlopeIndex = 0;
    for (Int32 Index = 0; Index < Formula->SlopeCount; Index++) {
        if (Formula->Slopes[Index].Penalty <= Penalty && Formula->Slopes[Index].Penalty >= SlopePenalty) {
            SlopePenalty = Formula->Slopes[Index].Penalty;
            SlopeIndex = Index;
        }
    }

    return &Formula->Slopes[SlopeIndex];
}

RTBattleStyleSlopeFormulaDataRef RTRuntimeGetBattleStyleSlopeFormulaData(
    RTRuntimeRef Runtime,
    Int32 SlopeID
) {
    for (Int32 Index = 0; Index < Runtime->SlopeFormulaDataCount; Index++) {
        if (Runtime->BattleStyleSlopeFormulaData[Index].SlopeID == SlopeID) {
            return &Runtime->BattleStyleSlopeFormulaData[Index];
        }
    }

    return NULL;
}

RTBattleStyleStatsFormulaDataRef RTRuntimeGetBattleStyleStatsFormulaData(
    RTRuntimeRef Runtime,
    Int32 BattleStyleIndex
) {
    assert(RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MIN <= BattleStyleIndex && BattleStyleIndex <= RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MAX);

    return &Runtime->BattleStyleStatsFormulaData[BattleStyleIndex - 1];
}

RTBattleStyleSkillRankDataRef RTRuntimeGetBattleStyleSkillRankData(
    RTRuntimeRef Runtime,
    Int32 BattleStyleIndex,
    Int32 SkillRank
) {
    assert(RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MIN <= BattleStyleIndex && BattleStyleIndex <= RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MAX);

    RTBattleStyleSkillFormulaDataRef FormulaData = &Runtime->BattleStyleSkillFormulaData[BattleStyleIndex - 1];
    for (Int32 Index = 0; Index < FormulaData->SkillRankCount; Index++) {
        if (FormulaData->SkillRanks[Index].SkillRank == SkillRank) {
            return &FormulaData->SkillRanks[Index];
        }
    }

    return NULL;
}

RTBattleStyleRankDataRef RTRuntimeGetBattleStyleRankData(
    RTRuntimeRef Runtime,
    Int32 BattleStyleIndex,
    Int32 Level
) {
    assert(RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MIN <= BattleStyleIndex && BattleStyleIndex <= RUNTIME_DATA_CHARACTER_BATTLE_STYLE_INDEX_MAX);

    RTBattleStyleRankFormulaDataRef FormulaData = &Runtime->BattleStyleRankFormulaData[BattleStyleIndex - 1];
    for (Int32 Index = 0; Index < FormulaData->RankCount; Index++) {
        if (FormulaData->Ranks[Index].Level == Level) {
            return &FormulaData->Ranks[Index];
        }
    }

    return NULL;
}

RTWorldContextRef RTRuntimeOpenDungeon(
    RTRuntimeRef Runtime,
    RTCharacterRef Character,
    Index WorldIndex,
    Index DungeonIndex
) {
    RTWorldDataRef WorldData = RTWorldDataGet(Runtime->WorldManager, WorldIndex);
    assert(
        WorldData->Type == RUNTIME_WORLD_TYPE_QUEST_DUNGEON ||
        WorldData->Type == RUNTIME_WORLD_TYPE_DUNGEON
    );

    // TODO: Cleanup previous dungeon, for now we assert to avoid to open a dungeon in a dungeon?
    RTWorldContextRef CurrentWorld = RTRuntimeGetWorldByCharacter(Runtime, Character);
    assert(CurrentWorld->WorldData->Type != RUNTIME_WORLD_TYPE_QUEST_DUNGEON);

    if (RTWorldContextPartyIsFull(Runtime->WorldManager)) return NULL;

    if (RTEntityIsNull(Character->PartyID)) {
        RTPartyRef Party = RTPartyManagerCreateParty(
            Runtime->PartyManager, 
            Character->CharacterIndex, 
            Character->ID, 
            RUNTIME_PARTY_TYPE_SOLO_DUNGEON
        );
        Character->PartyID = Party->ID;
    }

    RTWorldContextRef PartyWorld = RTWorldContextCreateParty(Runtime->WorldManager, WorldIndex, DungeonIndex, Character->PartyID);
    return PartyWorld;
}

Void RTRuntimeCloseDungeon(
    RTRuntimeRef Runtime,
    RTCharacterRef Character
) {
    assert(!RTEntityIsNull(Character->PartyID) && Character->PartyID.EntityType == RUNTIME_ENTITY_TYPE_PARTY);

    RTWorldContextRef WorldContext = RTWorldContextGetParty(Runtime->WorldManager, Character->PartyID);
    if (!WorldContext) return;

    RTPartyRef Party = RTRuntimeGetParty(Runtime, Character->PartyID);
    assert(Party);

    if (Party->PartyType == RUNTIME_PARTY_TYPE_SOLO_DUNGEON) {
        assert(WorldContext->DungeonIndex != Character->Info.Position.DungeonIndex);

        RTWorldContextDestroyParty(Runtime->WorldManager, Character->PartyID);
        RTPartyManagerDestroyParty(Runtime->PartyManager, Party);
        Character->PartyID = kEntityIDNull;
    }
    else {
        // TODO: Check if any party member is still in dungeon and do proper cleanup...
        assert(false && "Implementation missing!");
    }
}

RTDungeonDataRef RTRuntimeGetDungeonDataByID(
    RTRuntimeRef Runtime,
    Index DungeonID
) {
    for (Int32 Index = 0; Index < Runtime->DungeonDataCount; Index++) {
        RTDungeonDataRef DungeonData = &Runtime->DungeonData[Index];
        if (DungeonData->DungeonID == DungeonID) {
            return DungeonData;
        }
    }

    return NULL;
}

RTMissionDungeonPatternPartDataRef RTRuntimeGetPatternPartByID(
    RTRuntimeRef Runtime,
    Int32 PatternPartID
) {
    for (Int32 Index = 0; Index < Runtime->MissionDungeonPatternPartDataCount; Index++) {
        RTMissionDungeonPatternPartDataRef PatternPartData = &Runtime->MissionDungeonPatternPartData[Index];
        if (PatternPartData->ID == PatternPartID) return PatternPartData;
    }

    return NULL;
}

Void RTRuntimeBroadcastCharacterData(
    RTRuntimeRef Runtime,
    RTCharacterRef Character,
    UInt32 Type
) {
    NOTIFICATION_DATA_CHARACTER_DATA* Notification = RTNotificationInit(CHARACTER_DATA);
    Notification->Type = Type;

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_HPPOTION) {
        Notification->HPAfterPotion = Character->Attributes.Values[RUNTIME_ATTRIBUTE_HP_CURRENT];
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_MPPOTION) {
        // TODO: Check if this is also padded like HPAfterPotion
        Notification->MP = (UInt32)Character->Attributes.Values[RUNTIME_ATTRIBUTE_MP_CURRENT];
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_HP) {
        Notification->HP = Character->Attributes.Values[RUNTIME_ATTRIBUTE_HP_CURRENT];
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_MP) {
        Notification->MP = (UInt32)Character->Attributes.Values[RUNTIME_ATTRIBUTE_MP_CURRENT];
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_SP) {
        Notification->SP = (UInt32)Character->Attributes.Values[RUNTIME_ATTRIBUTE_SP_CURRENT];
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_SP_INCREASE) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_SP_DECREASE) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_EXP) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_RANK) {
        Notification->SkillRank = Character->Info.Skill.Rank;
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_LEVEL) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_SP_DECREASE_EX) {
        Fatal("Implementation missing!");
    }

    assert(Notification->Type != NOTIFICATION_CHARACTER_DATA_TYPE_BUFF_POTION);

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_REPUTATION) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_GUIDITEMFX) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_RESURRECTION) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_PENALTY_EXP) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_DAMAGE_CELL) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_DEFFICIENCY) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_AUTH_HP_POTION) {
        Fatal("Implementation missing!");
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_RAGE) {
        Notification->Rage = (UInt32)Character->Attributes.Values[RUNTIME_ATTRIBUTE_RAGE_CURRENT];
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_OVERLORD_LEVEL) {
        Notification->Level = Character->Info.Overlord.Level;
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_HONOR_MEDAL) {
        Notification->HonorMedalGrade = RTCharacterGetHonorMedalGrade(Runtime, Character, 0);
        Notification->HonorPoints = Character->Info.Honor.Point;
    }

    if (Notification->Type == NOTIFICATION_CHARACTER_DATA_TYPE_BP) {
        Notification->BP = (UInt32)Character->Attributes.Values[RUNTIME_ATTRIBUTE_BP_CURRENT];
    }

    RTNotificationDispatchToCharacter(Notification, Character);
}

#include "Runtime.h"
#include "Transform.h"

Bool RTCharacterTransformIsLocked(
    RTRuntimeRef Runtime,
    RTCharacterRef Character,
    Int32 TransformIndex
) {
    for (Int32 Index = 0; Index < Character->Data.TransformInfo.Count; Index += 1) {
        if (Character->Data.TransformInfo.Slots[Index].TransformIndex == TransformIndex) return false;
    }

    return true;
}

Bool RTCharacterTransformUnlock(
    RTRuntimeRef Runtime,
    RTCharacterRef Character,
    Int32 TransformIndex
) {
    if (Character->Data.TransformInfo.Count >= RUNTIME_CHARACTER_MAX_TRANSFORM_SLOT_COUNT) return false;
    if (!RTCharacterTransformIsLocked(Runtime, Character, TransformIndex)) return false;

    RTDataTransformRef Transform = RTRuntimeDataTransformGet(Runtime->Context, TransformIndex);
    if (!Transform) return false;

    Character->Data.TransformInfo.Slots[Character->Data.TransformInfo.Count].TransformIndex = Transform->TransformIndex;
    Character->Data.TransformInfo.Count += 1;
    Character->SyncMask.TransformInfo = true;
    return true;
}
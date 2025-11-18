#include "internal.h"

static u64 DoubleUntilFits(u64 Min, u64 Num) {
    while(Num < Min) {
        Num *= 2;
    }
    return Num;
}

int ArrayRequireRoom(array *Array, u64 Count, u64 InstanceByteCount, u64 InitialCapacity) {
    u64 NeededCapacity = Array->Count + Count;
    if(NeededCapacity < Array->Capacity) {
        return 0;
    }

    if(Array->Capacity == 0) {
        u64 NewCapacity = DoubleUntilFits(NeededCapacity, InitialCapacity);
        void *NewData = calloc(NewCapacity, InstanceByteCount);
        if(NewData == 0) {
            return 1;
        }
        Array->Data = NewData;
        Array->Capacity = InitialCapacity;

        return 0;
    }

    u64 NewCapacity = DoubleUntilFits(NeededCapacity, 2*Array->Capacity);
    void *NewData = calloc(NewCapacity, InstanceByteCount);
    if(NewData == 0) {
        return 1;
    }
    // NOTE(blackedout): Copy the whole array to allow for cases like free lists
    memcpy(NewData, Array->Data, Array->Capacity*InstanceByteCount);
    Array->Capacity = NewCapacity;
    free(Array->Data);
    Array->Data = NewData;

    return 0;
}

void ArrayClear(array *Array, u64 InstanceByteCount) {
    Array->Count = 0;
    memset(Array->Data, 0, Array->Capacity*InstanceByteCount);
}

void SetFlagsEnabledU32(u32 *InOut, u32 Mask, int DoEnable) {
    if(DoEnable) {
        *InOut |= Mask;
    } else {
        *InOut &= ~Mask;
    }
}

void SetFlagsU32(u32 *InOut, u32 Mask, u32 Bits) {
    *InOut = ((*InOut) & ~Mask) | Bits;
}
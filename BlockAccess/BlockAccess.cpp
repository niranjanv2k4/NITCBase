#include "BlockAccess.h"

#include <cstring>

RecId BlockAccess::linearSearch(int relId, char *attrName, Attribute attrVal, int op){

    RecId prevRecId;
    int ret = RelCacheTable::getSearchIndex(relId, &prevRecId);

    int block = -1, slot = -1;
    if(prevRecId.block == -1 && prevRecId.slot == -1){

        RelCatEntry relCatBuf;

        int ret = RelCacheTable::getRelCatEntry(relId, &relCatBuf);
        if(ret != SUCCESS)
            return RecId{-1, -1};

        block = relCatBuf.firstBlk;
        slot = 0;
    }
    else {
        block = prevRecId.block;
        slot = prevRecId.slot + 1;
    }

    while(block != -1){

        RecBuffer currBlockBuff(block);

        struct HeadInfo head;
        ret = currBlockBuff.getHeader(&head);
        if(ret != SUCCESS)
            return RecId{-1, -1};

        Attribute record[head.numAttrs];

        int ret = currBlockBuff.getRecord(record, slot);
        if(ret != SUCCESS)
            return RecId{-1, -1};


        unsigned char slotMap[head.numSlots];
        ret = currBlockBuff.getSlotMap(slotMap);
        if(ret != SUCCESS)
            return RecId{-1, -1};

        if(slot >= head.numSlots){
            block = head.rblock;
            slot = 0;
            continue;
        }

        if(slotMap[slot] == SLOT_UNOCCUPIED){
            slot++;
            continue;
        }

        AttrCatEntry attrCatEntry;
        ret = AttrCacheTable::getAttrCatEntry(relId, attrName, &attrCatEntry);
        if(ret != SUCCESS)
            return RecId{-1, -1};

        int cmpVal = compareAttrs(record[attrCatEntry.offset], attrVal, attrCatEntry.attrType);

        if (
            (op == NE && cmpVal != 0) ||    // if op is "not equal to"
            (op == LT && cmpVal < 0) ||     // if op is "less than"
            (op == LE && cmpVal <= 0) ||    // if op is "less than or equal to"
            (op == EQ && cmpVal == 0) ||    // if op is "equal to"
            (op == GT && cmpVal > 0) ||     // if op is "greater than"
            (op == GE && cmpVal >= 0)       // if op is "greater than or equal to"
        ) {
            RecId temp;
            temp.block = block;
            temp.slot = slot;

            RelCacheTable::setSearchIndex(relId, &temp);

            return temp;
        }

        slot++;
    }

    return RecId{-1, -1};
}

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

int BlockAccess::renameRelation(char* oldName, char* newName){

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute newRelationName;
    strcpy(newRelationName.sVal, newName);

    RecId relCatId = BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, newRelationName, EQ);
    if(relCatId.block != -1 || relCatId.slot != -1)
        return E_RELEXIST;

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute oldRelationName;
    strcpy(oldRelationName.sVal, oldName);

    relCatId = BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, oldRelationName, EQ);
    if(relCatId.block == -1 && relCatId.slot == -1)
        return E_RELNOTEXIST;

    RecBuffer relCatBuffer(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBuffer.getRecord(relCatRecord, relCatId.slot);

    strcpy(relCatRecord[RELCAT_REL_NAME_INDEX].sVal, newName);

    relCatBuffer.setRecord(relCatRecord, relCatId.slot);
    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    for(int i = 0; i < relCatRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal; i++){
        
        RecId attrCatId = BlockAccess::linearSearch(ATTRCAT_RELID, (char*)ATTRCAT_ATTR_RELNAME, oldRelationName, EQ);
        
        RecBuffer attrCatBuffer(attrCatId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

        attrCatBuffer.getRecord(attrCatRecord, attrCatId.slot);

        strcpy(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal, newName);

        attrCatBuffer.setRecord(attrCatRecord, attrCatId.slot);
    }

    return SUCCESS;    
}

int BlockAccess::renameAttribute(char* relName, char *oldName, char *newName){

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId relCatId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);

    if(relCatId.block == -1 && relCatId.slot == -1)
        return E_RELNOTEXIST;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    RecId attrToRenameRecId{-1, -1};
    Attribute attrCatEntryRecord[ATTRCAT_NO_ATTRS];

    while(true) {
        RecId current = linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if(current.block == -1 && current.slot == -1)
            break;

        RecBuffer attrCatBuffer(current.block);
        
        attrCatBuffer.getRecord(attrCatEntryRecord, current.slot);

        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, oldName) == 0){
            attrToRenameRecId = current;
            break;
        }

        if(strcmp(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName) == 0){
            return E_ATTREXIST;
        }
    } 

    if(attrToRenameRecId.block == -1 && attrToRenameRecId.slot == -1)
        return E_ATTRNOTEXIST;

    RecBuffer attrCatBuffer(attrToRenameRecId.block);
    attrCatBuffer.getRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    strcpy(attrCatEntryRecord[ATTRCAT_ATTR_NAME_INDEX].sVal, newName);

    attrCatBuffer.setRecord(attrCatEntryRecord, attrToRenameRecId.slot);

    return SUCCESS;
}
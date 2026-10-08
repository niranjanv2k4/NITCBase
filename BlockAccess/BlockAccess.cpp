#include "BlockAccess.h"

#include <cstring>

#include <bits/stdc++.h>
using namespace std;

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

int BlockAccess::insert(int relId, Attribute *record){

    
    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(relId, &relCatEntry);

    int blockNum = relCatEntry.firstBlk;

    RecId recId = {-1, -1};

    int numOfSlots = relCatEntry.numSlotsPerBlk;
    int numOfAttributes = relCatEntry.numAttrs;
    int prevBlockNum = -1;

    while(blockNum != -1){

        RecBuffer blockBuffer(blockNum);

        struct HeadInfo head;
        blockBuffer.getHeader(&head);

        unsigned char slotMap[numOfSlots];
        blockBuffer.getSlotMap(slotMap);

        for(int i = 0; i < numOfSlots; i++){
            if(slotMap[i] == SLOT_UNOCCUPIED){
                recId.block = blockNum;
                recId.slot = i;
                break;
            }
        }
        
        if(recId.block != -1 && recId.slot != -1)
            break;

        prevBlockNum = blockNum;
        blockNum = head.rblock;

    } 

    if(recId.block == -1 && recId.slot == -1){

        if(relId == RELCAT_RELID)
            return E_MAXRELATIONS;

        RecBuffer newRecordBlock;
        int blockNum = newRecordBlock.getBlockNum();

        if(blockNum == E_DISKFULL)
            return E_DISKFULL;

        recId.block = blockNum;
        recId.slot = 0;

        struct HeadInfo head;
        head.blockType = REC;
        head.pblock = -1;
        head.lblock = -1;
        head.rblock = -1;
        head.numEntries = 0;
        head.numSlots = numOfSlots;
        head.numAttrs = numOfAttributes;

        newRecordBlock.setHeader(&head);

        unsigned char slotMap[numOfSlots];
        memset(slotMap, SLOT_UNOCCUPIED, numOfSlots);
        newRecordBlock.setSlotMap(slotMap);

        if(prevBlockNum != -1){

            RecBuffer prevBlockBuffer(prevBlockNum);
            struct HeadInfo prevHead;
            prevBlockBuffer.getHeader(&prevHead);

            prevHead.rblock = recId.block;

            prevBlockBuffer.setHeader(&prevHead);

        }
        else {

            relCatEntry.firstBlk = blockNum;
            RelCacheTable::setRelCatEntry(relId, &relCatEntry);

        }

        relCatEntry.lastBlk = blockNum;
        RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    }

    RecBuffer blockBuffer(recId.block);
    blockBuffer.setRecord(record, recId.slot);

    unsigned char slotMap[numOfSlots];
    blockBuffer.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_OCCUPIED;
    blockBuffer.setSlotMap(slotMap);

    struct HeadInfo head;
    blockBuffer.getHeader(&head);
    head.numEntries++;
    blockBuffer.setHeader(&head);

    relCatEntry.numRecs++;
    RelCacheTable::setRelCatEntry(relId, &relCatEntry);

    return SUCCESS;

}

int BlockAccess::search(int relId, Attribute *record, char attrName[ATTR_SIZE], Attribute attrVal, int op){

    RecId recId;

    recId = linearSearch(relId, attrName, attrVal, op);
    if(recId.block == -1 && recId.slot == -1)
        return E_NOTFOUND;

    RecBuffer buffer(recId.block);
    buffer.getRecord(record, recId.slot);

    return SUCCESS;

}

int BlockAccess::deleteRelation(char relName[ATTR_SIZE]){

    if(strcmp(relName, RELCAT_RELNAME) == 0 || strcmp(relName, ATTRCAT_RELNAME) == 0)
        return E_NOTPERMITTED;

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute relNameAttr;
    strcpy(relNameAttr.sVal, relName);

    RecId recId = BlockAccess::linearSearch(RELCAT_RELID, (char *)RELCAT_ATTR_RELNAME, relNameAttr, EQ);
    if(recId.block == -1 && recId.slot == -1)
        return E_RELNOTEXIST;

    Attribute relCatEntryRecord[RELCAT_NO_ATTRS];
    RecBuffer relCatBuffer(recId.block);
    relCatBuffer.getRecord(relCatEntryRecord, recId.slot);

    int currBlk = (int)relCatEntryRecord[RELCAT_FIRST_BLOCK_INDEX].nVal;
    int numOfAttributes = (int)relCatEntryRecord[RELCAT_NO_ATTRIBUTES_INDEX].nVal;

    while(currBlk != -1){

        HeadInfo head;
        RecBuffer currBlockBuffer(currBlk);
        currBlockBuffer.getHeader(&head);

        currBlk = head.rblock;
        currBlockBuffer.releaseBlock();

    }

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);

    int numOfAttributesDeleted = 0;

    while(true){

        RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, relNameAttr, EQ);
        if(attrCatRecId.block == -1 && attrCatRecId.slot == -1)
            break;

        numOfAttributesDeleted++;

        RecBuffer attrCatBlock(attrCatRecId.block);
        HeadInfo attrCatBlockHeader;
        attrCatBlock.getHeader(&attrCatBlockHeader);

        unsigned char slotMap[attrCatBlockHeader.numSlots];
        attrCatBlock.getSlotMap(slotMap);
        slotMap[attrCatRecId.slot] = SLOT_UNOCCUPIED;
        attrCatBlock.setSlotMap(slotMap);

        attrCatBlockHeader.numEntries--;

        if(attrCatBlockHeader.numEntries == 0){

            RecBuffer prevBlock(attrCatBlockHeader.lblock);
            HeadInfo prevBlockHeader;
            prevBlock.getHeader(&prevBlockHeader);

            prevBlockHeader.rblock = attrCatBlockHeader.rblock;

            prevBlock.setHeader(&prevBlockHeader);

            if(attrCatBlockHeader.rblock != -1){

                RecBuffer nextBlock(attrCatBlockHeader.rblock);
                HeadInfo nextBlockHeader;
                nextBlock.getHeader(&nextBlockHeader);

                nextBlockHeader.lblock = attrCatBlockHeader.lblock;

                nextBlock.setHeader(&nextBlockHeader); 
            }
            else {
                RelCatEntry relCatEntry;
                RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &relCatEntry);
                relCatEntry.lastBlk = attrCatBlockHeader.lblock;
                RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &relCatEntry);
            }

            attrCatBlock.releaseBlock();
        }
    }

    HeadInfo relCatHeader;
    RecBuffer relCatBlock(RELCAT_BLOCK);

    relCatBlock.getHeader(&relCatHeader);
    relCatHeader.numEntries--;
    relCatBlock.setHeader(&relCatHeader);

    unsigned char slotMap[relCatHeader.numSlots];

    relCatBlock.getSlotMap(slotMap);
    slotMap[recId.slot] = SLOT_UNOCCUPIED;
    relCatBlock.setSlotMap(slotMap);

    RelCatEntry relCatEntry;
    RelCacheTable::getRelCatEntry(RELCAT_RELID, &relCatEntry);
    relCatEntry.numRecs--;
    RelCacheTable::setRelCatEntry(RELCAT_RELID, &relCatEntry);

    RelCatEntry attrRelCatEntry;
    RelCacheTable::getRelCatEntry(ATTRCAT_RELID, &attrRelCatEntry);
    attrRelCatEntry.numRecs -= numOfAttributesDeleted;
    RelCacheTable::setRelCatEntry(ATTRCAT_RELID, &attrRelCatEntry);

    return SUCCESS;
}
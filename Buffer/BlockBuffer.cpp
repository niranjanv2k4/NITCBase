#include "BlockBuffer.h"

#include <cstdlib>
#include <cstring>

#include <bits/stdc++.h>
using namespace std;

BlockBuffer::BlockBuffer(int blockNum){
    this->blockNum = blockNum;
}

RecBuffer::RecBuffer(int blockNum) : BlockBuffer::BlockBuffer(blockNum){}

int BlockBuffer::getHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS){
        return ret;
    }

    memcpy(&head->pblock, bufferPtr + 4, 4);
    memcpy(&head->lblock, bufferPtr + 8, 4);
    memcpy(&head->rblock, bufferPtr + 12, 4);
    memcpy(&head->numEntries, bufferPtr + 16, 4);
    memcpy(&head->numAttrs, bufferPtr + 20, 4);
    memcpy(&head->numSlots, bufferPtr + 24, 4);

    return SUCCESS;
}

int RecBuffer::getRecord(union Attribute *rec, int slotNum){

    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS){
        return ret;
    }

    struct HeadInfo head;
    this->getHeader(&head);

    int attrCount = head.numAttrs;
    int slotCount = head.numSlots;
    
    int recordSize = attrCount * ATTR_SIZE;
    unsigned char *slotPointer = bufferPtr + HEADER_SIZE + slotCount + (recordSize * slotNum);
    memcpy(rec, slotPointer, recordSize);

    return SUCCESS;
}

int BlockBuffer::loadBlockAndGetBufferPtr(unsigned char **buffPtr){

    int bufferNum = StaticBuffer::getBufferNum(this->blockNum);

    if(bufferNum == E_BLOCKNOTINBUFFER) {

        bufferNum = StaticBuffer::getFreeBuffer(this->blockNum);

        if(bufferNum == E_OUTOFBOUND) 
            return E_OUTOFBOUND;

        Disk::readBlock(StaticBuffer::blocks[bufferNum], this->blockNum);
    }
    else{
        for(int i = 0; i < BUFFER_CAPACITY; i++)
            if(StaticBuffer::metainfo[i].free == false)
                StaticBuffer::metainfo[i].timeStamp++;

        StaticBuffer::metainfo[bufferNum].timeStamp = 0;
    }

    *buffPtr = StaticBuffer::blocks[bufferNum];

    return SUCCESS;
}

int RecBuffer::getSlotMap(unsigned char *slotMap){
    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS)
        return ret;

    struct HeadInfo head;
    this->getHeader(&head);

    int slotCount = head.numSlots;
    unsigned char *slotMapInBuffer = bufferPtr + HEADER_SIZE;

    memcpy(slotMap, slotMapInBuffer, slotCount);

    return SUCCESS;
}

int compareAttrs(Attribute attr1, Attribute attr2, int attrType){
    int res = attrType == NUMBER ? attr1.nVal - attr2.nVal : strcmp(attr1.sVal, attr2.sVal);
    return res == 0 ? 0 : res / abs(res);
}

int RecBuffer::setRecord(union Attribute *rec, int slotNum){

    unsigned char *bufferPtr;

    int ret = this->loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS)
        return ret;

    struct HeadInfo header;
    BlockBuffer::getHeader(&header);
    
    int numOfAttrs = header.numAttrs;
    int numOfSlots = header.numSlots;

    if(slotNum < 0 || slotNum >= numOfSlots)
        return E_OUTOFBOUND;

    bufferPtr = bufferPtr + HEADER_SIZE + numOfSlots + slotNum * numOfAttrs * ATTR_SIZE;

    memcpy(bufferPtr, rec, ATTR_SIZE * numOfAttrs);

    StaticBuffer::setDirtyBit(this->blockNum);

    return SUCCESS;
}

int BlockBuffer::setHeader(struct HeadInfo *head){

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS)
        return ret;

    struct HeadInfo *bufferHeader = (struct HeadInfo *)bufferPtr;
    bufferHeader->blockType = head->blockType;
    bufferHeader->pblock = head->pblock;
    bufferHeader->lblock = head->lblock;
    bufferHeader->rblock = head->rblock;
    bufferHeader->numEntries = head->numEntries;
    bufferHeader->numAttrs = head->numAttrs;
    bufferHeader->numSlots = head->numSlots;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if(ret != SUCCESS)
        return ret;

    return SUCCESS;

}

int BlockBuffer::setBlockType(int blockType){

    unsigned char *bufferPtr;
    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS)
        return ret;

    *((int32_t *)bufferPtr) = blockType;

    StaticBuffer::blockAllocMap[this->blockNum] = blockType;

    ret = StaticBuffer::setDirtyBit(this->blockNum);
    if(ret != SUCCESS)
        return ret;

    return SUCCESS;

}

int BlockBuffer::getFreeBlock(int blockType){

    int freeBlock = -1;

    for(int i = 0; i < DISK_BLOCKS; i++){
        if(StaticBuffer::blockAllocMap[i] == UNUSED_BLK){
            freeBlock = i;
            break;
        }
    }

    if(freeBlock == -1)
        return E_DISKFULL;

    this->blockNum = freeBlock;

    int freeBuffer = StaticBuffer::getFreeBuffer(freeBlock);

    struct HeadInfo head;
    head.blockType = blockType;
    head.pblock = -1;
    head.lblock = -1;
    head.rblock = -1;
    head.numEntries = 0;
    head.numAttrs = 0;
    head.numAttrs = 0;

    this->setHeader(&head);

    this->setBlockType(blockType);

    return freeBlock;

}

BlockBuffer::BlockBuffer(char blockType){
    
    if(blockType == 'R')
        getFreeBlock(REC);

}

RecBuffer::RecBuffer() : BlockBuffer('R') {}

int RecBuffer::setSlotMap(unsigned char *slotMap){

    unsigned char *bufferPtr;

    int ret = loadBlockAndGetBufferPtr(&bufferPtr);
    if(ret != SUCCESS)
        return ret;

    struct HeadInfo head;
    this->getHeader(&head);

    memcpy(bufferPtr + HEADER_SIZE, slotMap, head.numSlots);
    
    return StaticBuffer::setDirtyBit(this->blockNum);

}

int BlockBuffer::getBlockNum(){
    return this->blockNum;
}
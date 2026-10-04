#include "StaticBuffer.h"

unsigned char StaticBuffer::blocks[BUFFER_CAPACITY][BLOCK_SIZE];
struct BufferMetaInfo StaticBuffer::metainfo[BUFFER_CAPACITY];
unsigned char StaticBuffer::blockAllocMap[DISK_BLOCKS];

StaticBuffer::StaticBuffer(){

    Disk::readBlock(blockAllocMap + 0 * BLOCK_SIZE, 0);
    Disk::readBlock(blockAllocMap + 1 * BLOCK_SIZE, 1);
    Disk::readBlock(blockAllocMap + 2 * BLOCK_SIZE, 2);
    Disk::readBlock(blockAllocMap + 3 * BLOCK_SIZE, 3);

    for(int i = 0; i < BUFFER_CAPACITY; i++){
        metainfo[i].free = true;
        metainfo[i].dirty = false;
        metainfo[i].timeStamp = -1;
        metainfo[i].blockNum = -1;
    }

}

StaticBuffer::~StaticBuffer() {

    Disk::writeBlock(blockAllocMap + 0 * BLOCK_SIZE, 0);
    Disk::writeBlock(blockAllocMap + 1 * BLOCK_SIZE, 1);
    Disk::writeBlock(blockAllocMap + 2 * BLOCK_SIZE, 2);
    Disk::writeBlock(blockAllocMap + 3 * BLOCK_SIZE, 3);

    for(int i = 0; i < BUFFER_CAPACITY; i++){
        if(metainfo[i].free == false && metainfo[i].dirty == true){
            Disk::writeBlock(blocks[i], metainfo[i].blockNum);
        }
    }

}

int StaticBuffer::getFreeBuffer(int blockNum){

    if(blockNum < 0 || blockNum > DISK_BLOCKS){
        return E_OUTOFBOUND;
    }

    for(int i = 0; i < BUFFER_CAPACITY; i++){
        if(metainfo[i].free == false){
            metainfo[i].timeStamp++;
        }
    }

    int allocatedBuffer = -1;

    for(int i = 0; i < BUFFER_CAPACITY; i++){
        if(metainfo[i].free){
            allocatedBuffer = i;
            break;
        }
    }

    if(allocatedBuffer == -1){
        int max_timeStamp = 0;

        for(int i = 0; i < BUFFER_CAPACITY; i++){
            if(metainfo[i].free == false && metainfo[i].timeStamp > max_timeStamp){
                max_timeStamp = metainfo[i].timeStamp;
                allocatedBuffer = i;
                break;
            }
        }

        if(metainfo[allocatedBuffer].dirty){
            Disk::writeBlock(blocks[allocatedBuffer], metainfo[allocatedBuffer].blockNum);
        }
    }

    metainfo[allocatedBuffer].free = false;
    metainfo[allocatedBuffer].dirty = false;
    metainfo[allocatedBuffer].blockNum = blockNum;
    metainfo[allocatedBuffer].timeStamp = 0;

    return allocatedBuffer;
}

int StaticBuffer::getBufferNum(int blockNum){

    if(blockNum < 0 || blockNum > DISK_BLOCKS){
        return E_OUTOFBOUND;
    }

    for(int i = 0; i < BUFFER_CAPACITY; i++){
        if(metainfo[i].blockNum == blockNum){
            return i;
        }
    }

    return E_BLOCKNOTINBUFFER;
}

int StaticBuffer::setDirtyBit(int blockNum){

    int bufferNum = getBufferNum(blockNum);

    if(bufferNum == E_BLOCKNOTINBUFFER || bufferNum == E_OUTOFBOUND)
        return bufferNum;

    metainfo[bufferNum].dirty = true;

    return SUCCESS;
}
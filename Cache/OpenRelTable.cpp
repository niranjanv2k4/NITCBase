#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>

#include <bits/stdc++.h>
using namespace std;

OpenRelTableMetaInfo OpenRelTable::tableMetaInfo[MAX_OPEN];

OpenRelTable::OpenRelTable(){

    for(int i = 0; i < MAX_OPEN; i++){
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
        OpenRelTable::tableMetaInfo[i].free = true;
    }

    RecBuffer relCatBlock(RELCAT_BLOCK);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBlock.getRecord(relCatRecord, RELCAT_SLOTNUM_FOR_RELCAT);

    RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &relCacheEntry.relCatEntry);
    relCacheEntry.recId.block = RELCAT_BLOCK;
    relCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_RELCAT;

    RelCacheTable::relCache[RELCAT_RELID] = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[RELCAT_RELID]) = relCacheEntry;



    Attribute attrCatRelRecord[ATTRCAT_NO_ATTRS];
    relCatBlock.getRecord(attrCatRelRecord, RELCAT_SLOTNUM_FOR_ATTRCAT);

    RelCacheEntry attrCatRelCacheEntry;
    RelCacheTable::recordToRelCatEntry(attrCatRelRecord, &attrCatRelCacheEntry.relCatEntry);
    attrCatRelCacheEntry.recId.block = ATTRCAT_BLOCK;
    attrCatRelCacheEntry.recId.slot = RELCAT_SLOTNUM_FOR_ATTRCAT;

    RelCacheTable::relCache[ATTRCAT_RELID] = (RelCacheEntry *)malloc(sizeof(RelCacheEntry));
    *(RelCacheTable::relCache[ATTRCAT_RELID]) = attrCatRelCacheEntry;

    RecBuffer attrCatBlock(ATTRCAT_BLOCK);
    
    AttrCacheEntry *head = nullptr;

    for(int i = RELCAT_NO_ATTRS - 1; i >= 0; i--){

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, i);

        AttrCacheEntry *attrCacheEntry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);

        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = i;

        attrCacheEntry->next = head;
        head = attrCacheEntry;
    }

    AttrCacheTable::attrCache[RELCAT_RELID] = head;

    head = nullptr;

    for(int i = RELCAT_NO_ATTRS + ATTRCAT_NO_ATTRS - 1; i >= RELCAT_NO_ATTRS; i--){

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, i);

        AttrCacheEntry* attrCacheEntry = (struct AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &attrCacheEntry->attrCatEntry);

        attrCacheEntry->recId.block = ATTRCAT_BLOCK;
        attrCacheEntry->recId.slot = i;

        attrCacheEntry->next = head;
        head = attrCacheEntry;
    }
    
    AttrCacheTable::attrCache[ATTRCAT_RELID] = head;

    OpenRelTable::tableMetaInfo[RELCAT_RELID].free = false;
    strcpy(OpenRelTable::tableMetaInfo[RELCAT_RELID].relName, "RELATIONCAT");

    OpenRelTable::tableMetaInfo[ATTRCAT_RELID].free = false;
    strcpy(OpenRelTable::tableMetaInfo[ATTRCAT_RELID].relName, "ATTRIBUTECAT");
}


OpenRelTable::~OpenRelTable(){

    for(int relId = 2; relId < MAX_OPEN; relId++){
        if(!tableMetaInfo[relId].free)
            OpenRelTable::closeRel(relId);
    }

    free(RelCacheTable::relCache[RELCAT_RELID]);
    free(RelCacheTable::relCache[ATTRCAT_RELID]);

    AttrCacheEntry *current = AttrCacheTable::attrCache[RELCAT_RELID];


    while(current != nullptr){
        AttrCacheEntry *next = current->next;
        free(current);
        current = next;
    }

    current = AttrCacheTable::attrCache[ATTRCAT_RELID];

    while(current != nullptr){
        AttrCacheEntry* next = current->next;
        free(current);
        current = next;
    }
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

    for(int relId = 0; relId < MAX_OPEN; relId++)
        if(!OpenRelTable::tableMetaInfo[relId].free && strcmp(relName, OpenRelTable::tableMetaInfo[relId].relName) == 0)
            return relId;

    return E_RELNOTOPEN;
}

int OpenRelTable::getFreeOpenRelTableEntry(){

    for(int relId = 2; relId < MAX_OPEN; relId++)
        if(OpenRelTable::tableMetaInfo[relId].free)
            return relId;

    return E_CACHEFULL;
}

int OpenRelTable::openRel(char relName[ATTR_SIZE]){

    int relId = OpenRelTable::getRelId(relName);
    if(relId != E_RELNOTOPEN)
        return relId;

    relId = OpenRelTable::getFreeOpenRelTableEntry();

    if(relId == E_CACHEFULL)
        return E_CACHEFULL;

    RelCacheTable::resetSearchIndex(RELCAT_RELID);

    Attribute name;
    strcpy(name.sVal, relName);

    RecId relcatRecId = BlockAccess::linearSearch(RELCAT_RELID, (char*)RELCAT_ATTR_RELNAME, name, EQ);

    if(relcatRecId.block == -1 && relcatRecId.slot == -1)
        return E_RELNOTEXIST;

    RecBuffer relCatBlock(relcatRecId.block);
    Attribute relCatRecord[RELCAT_NO_ATTRS];

    relCatBlock.getRecord(relCatRecord, relcatRecId.slot);

    RelCacheEntry relCacheEntry;
    RelCacheTable::recordToRelCatEntry(relCatRecord, &(relCacheEntry.relCatEntry));

    relCacheEntry.recId = relcatRecId;

    RelCacheTable::relCache[relId] = (RelCacheEntry*)malloc(sizeof(RelCacheEntry));
    *RelCacheTable::relCache[relId] = relCacheEntry;

    AttrCacheEntry* listhead = nullptr;

    RelCacheTable::resetSearchIndex(ATTRCAT_RELID);
    
    RecId attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, name, EQ);

    do{

        RecBuffer attrCatBlock(attrCatRecId.block);
        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];

        attrCatBlock.getRecord(attrCatRecord, attrCatRecId.slot);

        AttrCacheEntry* attrCacheEntry = (AttrCacheEntry*)malloc(sizeof(AttrCacheEntry));
        attrCacheEntry->next = nullptr;

        AttrCacheTable::recordToAttrCatEntry(attrCatRecord, &(attrCacheEntry->attrCatEntry));

        attrCacheEntry->recId = attrCatRecId;

        if(listhead != nullptr)
            attrCacheEntry->next = listhead;
        
        listhead = attrCacheEntry;

        attrCatRecId = BlockAccess::linearSearch(ATTRCAT_RELID, (char *)ATTRCAT_ATTR_RELNAME, name, EQ);

    } while(attrCatRecId.block != -1 && attrCatRecId.slot != -1);

    AttrCacheTable::attrCache[relId] = listhead;

    OpenRelTable::tableMetaInfo[relId].free = false;
    strcpy(OpenRelTable::tableMetaInfo[relId].relName, relName);

    return relId;
}


int OpenRelTable::closeRel(int relId){

    if(relId == RELCAT_RELID || relId == ATTRCAT_RELID)
        return E_NOTPERMITTED;

    if(relId < 2 || relId >= MAX_OPEN)
        return E_OUTOFBOUND;

    if(OpenRelTable::tableMetaInfo[relId].free)
        return E_RELNOTOPEN;
    
    if(RelCacheTable::relCache[relId]->dirty){

        Attribute relCatBuffer[RELCAT_NO_ATTRS];
        RelCacheTable::relCatEntryToRecord(&(RelCacheTable::relCache[relId]->relCatEntry), relCatBuffer);

        RecBuffer relCatBlock(RelCacheTable::relCache[relId]->recId.block);
        relCatBlock.setRecord(relCatBuffer, RelCacheTable::relCache[relId]->recId.slot);

    }

    free(RelCacheTable::relCache[relId]);
    RelCacheTable::relCache[relId] = nullptr;

    while(AttrCacheTable::attrCache[relId]){
        AttrCacheEntry* temp = AttrCacheTable::attrCache[relId];
        AttrCacheTable::attrCache[relId] = AttrCacheTable::attrCache[relId]->next;
        free(temp);
    }

    AttrCacheTable::attrCache[relId] == nullptr;

    OpenRelTable::tableMetaInfo[relId].free = true;

    return SUCCESS;
}
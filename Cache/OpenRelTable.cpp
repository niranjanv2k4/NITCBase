#include "OpenRelTable.h"

#include <cstring>
#include <cstdlib>

OpenRelTable::OpenRelTable(){

    for(int i = 0; i < MAX_OPEN; i++){
        RelCacheTable::relCache[i] = nullptr;
        AttrCacheTable::attrCache[i] = nullptr;
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


    int studentsRelCatSlot = -1;
    Attribute studentsRecord[RELCAT_NO_ATTRS];

    HeadInfo relCatHeader;
    relCatBlock.getHeader(&relCatHeader);

    for(int slotNum = 0; slotNum <= relCatHeader.numSlots; slotNum++){
        relCatBlock.getRecord(studentsRecord, slotNum);

        if(strcmp(studentsRecord[RELCAT_REL_NAME_INDEX].sVal, "Students") == 0){
            studentsRelCatSlot = slotNum;
            break;
        }
    }

    if(studentsRelCatSlot != -1){
        RelCacheEntry studentsRelCacheEntry;

        RelCacheTable::recordToRelCatEntry(
            studentsRecord, 
            &studentsRelCacheEntry.relCatEntry
        );

        studentsRelCacheEntry.recId.block = RELCAT_BLOCK;
        studentsRelCacheEntry.recId.slot = studentsRelCatSlot;

        RelCacheTable::relCache[2] = (struct RelCacheEntry *)std::malloc(sizeof(RelCacheEntry));
        *(RelCacheTable::relCache[2]) = studentsRelCacheEntry;
    }

    /**** setting up Students relation in the Attribute Cache Table ****/
    HeadInfo attrCatHeader;
    attrCatBlock.getHeader(&attrCatHeader);

    AttrCacheEntry *studentsHead = nullptr;

    for (int slotNum = attrCatHeader.numSlots - 1; slotNum >= 0; slotNum--){

        Attribute attrCatRecord[ATTRCAT_NO_ATTRS];
        attrCatBlock.getRecord(attrCatRecord, slotNum);

        if (strcmp(attrCatRecord[ATTRCAT_REL_NAME_INDEX].sVal,"Students") != 0){
            continue;
        }

        AttrCacheEntry *entry = (AttrCacheEntry *)malloc(sizeof(AttrCacheEntry));

        AttrCacheTable::recordToAttrCatEntry(
            attrCatRecord,
            &entry->attrCatEntry
        );

        entry->recId.block = ATTRCAT_BLOCK;
        entry->recId.slot = slotNum;

        entry->next = studentsHead;
        studentsHead = entry;
    }

    AttrCacheTable::attrCache[2] = studentsHead;
}


OpenRelTable::~OpenRelTable(){

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

    current = AttrCacheTable::attrCache[2];

    while (current != nullptr){
        AttrCacheEntry *next = current->next;
        free(current);
        current = next;
    }
}

int OpenRelTable::getRelId(char relName[ATTR_SIZE]) {

    // if relname is RELCAT_RELNAME, return RELCAT_RELID
    // if relname is ATTRCAT_RELNAME, return ATTRCAT_RELID

    if(strcmp(relName, RELCAT_RELNAME) == 0)
        return RELCAT_RELID;

    if(strcmp(relName, ATTRCAT_RELNAME) == 0)
        return ATTRCAT_RELID;

    if(strcmp(relName, "Students") == 0)
        return 2;

    return E_RELNOTOPEN;
}
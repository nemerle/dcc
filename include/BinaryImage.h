#pragma once
#include <stdint.h>
#include <vector>
struct PROG /* Loaded program image parameters  */
{
    int16_t     initCS=0;
    int16_t     initIP=0;     /* These are initial load values    */
    int16_t     initSS=0;     /* Probably not of great interest   */
    uint16_t    initSP=0;
    bool        fCOM=false;       /* Flag set if COM program (else EXE)*/
    int         cReloc=0;     /* No. of relocation table entries  */
    std::vector<uint32_t> relocTable; /* Ptr. to relocation table         */
    uint8_t *   map=nullptr;        /* Memory bitmap ptr                */
    int         cProcs=0;     /* Number of procedures so far      */
    int         offMain=0;    /* The offset  of the main() proc   */
    uint16_t    segMain=0;    /* The segment of the main() proc   */
    bool        bSigs=false;      /* True if signatures loaded        */
    int         cbImage=0;    /* Length of image in bytes         */
    uint8_t *   Imagez=nullptr;      /* Allocated by loader to hold entire program image */
    int         addressingMode=0;
public:
    const uint8_t *image() const {return Imagez;}
    void displayLoadInfo();
};


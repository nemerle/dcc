#pragma once
#include <stdint.h>
#include <vector>
struct PROG /* Loaded program image parameters  */
{
    int16_t     initCS;
    int16_t     initIP;     /* These are initial load values    */
    int16_t     initSS;     /* Probably not of great interest   */
    uint16_t    initSP;
    bool        fCOM;       /* Flag set if COM program (else EXE)*/
    int         cReloc;     /* No. of relocation table entries  */
    std::vector<uint32_t> relocTable; /* Ptr. to relocation table         */
    uint8_t *   map;        /* Memory bitmap ptr                */
    int         cProcs;     /* Number of procedures so far      */
    int         offMain;    /* The offset  of the main() proc   */
    uint16_t    segMain;    /* The segment of the main() proc   */
    int         cbImage;    /* Length of image in bytes         */
    uint8_t *   Imagez;      /* Allocated by loader to hold entire program image */
public:
    const uint8_t *image() const {return Imagez;}
    void displayLoadInfo();
};


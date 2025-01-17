#ifndef TPL_PATTERNCOLLECTOR_H
#define TPL_PATTERNCOLLECTOR_H
#include "PatternCollector.h"

#include <stdio.h>
#include <stdint.h>
#include <vector>

struct TPL_PatternCollector : public PatternCollector {
protected:
    uint16_t cmap;
    uint16_t pmap;
    uint16_t csegBase;
    uint16_t unitBase;
    uint16_t offStCseg;
    uint16_t skipPmap;
    int count = 0;
    int	cAllocSym = 0;
    int unitNum = 0;
    char version;
    char charProc;
    char charFunc;
    uint16_t csegoffs[100];
    uint16_t csegIdx;
    std::vector<long int> positionStack;

    void enterSym(FILE *f,const char *name, uint16_t pmapOffset);
    void allocSym(int sym_count);
    void readCmapOffsets(FILE *f);
    void enterSystemUnit(FILE *f);
    void readString(FILE *f);
    void unknown(FILE *f,unsigned j, unsigned k);
    void nextUnit(FILE *f);
    void setVersionSpecifics(void);
    void savePos(FILE *f);
    void restorePos(FILE *f);
    void enterUnitProcs(FILE *f);
public:
    /* Read the .tpl file, and put the keys into the array *keys[]. Returns the count */
    int readSyms(FILE *f);
};

#endif // TPL_PATTERNCOLLECTOR_H


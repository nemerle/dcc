#ifndef CHKLIB_H
#define CHKLIB_H

#include "Command.h"
#include "Enums.h"
#include "perfhlib.h"

#include <QtCore/QFile>
#include <QtCore/QString>
#include <vector>

class Function;

// This will create a PatternLocator instance load it and pass it to project instance.
struct LoadPatternLibrary : public Command {
    LoadPatternLibrary() : Command("Load patterns for the file",eProject) {}
    bool execute(CommandContext *ctx) override;
};

class PatternLocator {
    std::vector<hlType> pArg;                /* Points to the array of param types */
    QString pattern_id;
    int     numFunc=0;                /* Number of func names actually stored */
    int     numArg=0;                 /* Number of param names actually stored */
public:
    struct HT *             ht  =nullptr;  //!< The hash table
    struct PH_FUNC_STRUCT * pFunc=nullptr; //!< Points to the array of func names


    PatternLocator(QString name) : pattern_id(name) {}
    ~PatternLocator();

    bool load();
    int searchPList(const char * name);

    bool LibCheck(Function & pProc);
private:
    bool readProtoFile();
    PerfectHash g_pattern_hasher;
    int numKeys=0;                        /* Number of hash table entries (keys) */
    int numVert=0;                        /* Number of vertices in the graph (also size of g[]) */
    unsigned PatLen=0;                    /* Size of the keys (pattern length) */
    unsigned SymLen=0;                    /* Max size of the symbols, including null */
    /* Pointers to start of T1, T2 */
    uint16_t *  T1base = nullptr;
    uint16_t *  T2base = nullptr;
    uint16_t *  g = nullptr;            /* g[] */

};
extern bool checkStartup(struct STATE &state);
#endif // CHKLIB_H

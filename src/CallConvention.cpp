#include "CallConvention.h"

#include "Procedure.h"

#include <QtCore/QTextStream>
#include <ostream>
#include <cassert>
static void calculateReturnLocations(Function *func) {
    switch(func->getReturnType()) {
    case TYPE_LONG_SIGN:
    case TYPE_LONG_UNSIGN:
        func->getFunctionType()->setReturnLocation(LONGID_TYPE(rDX,rAX));
        break;
    case TYPE_WORD_SIGN:
    case TYPE_WORD_UNSIGN:
        func->getFunctionType()->setReturnLocation(rAX);
        break;
    case TYPE_BYTE_SIGN:
    case TYPE_BYTE_UNSIGN:
        func->getFunctionType()->setReturnLocation(rAL);
        break;
    }
}
static void calculateArgLocations_allOnStack(Function *func) {
    FunctionType *type = func->type;
    int stack_offset=2;
    if(func->args.size() == type->ContainedTys.size())
        return;
    func->args.resize(type->ContainedTys.size());
    func->args.numArgs=0;
    for(Type & argtype : type->ContainedTys) {
        STKSYM &arg(func->args[func->args.numArgs]);
        arg.label= stack_offset;
        arg.size = TypeContainer::typeSize(argtype.dcc_type);
        arg.type = argtype.dcc_type;
        arg.setArgName(func->args.numArgs);
        stack_offset+=arg.size;
        func->args.maxOff=stack_offset;
        func->args.numArgs++;
    }
    func->cbParam = stack_offset;
}

static void rebuildArguments_FromStackLayout(Function *func) {

    STKFRAME &stk(func->args);
    std::map<int,const STKSYM *> arg_locations;
    FunctionType *f;

    for(const STKSYM & s: stk) {
        if(s.label>0) {
            arg_locations[s.label] = &s;
        }
    }

    if(arg_locations.empty())
        return;

    std::vector<Type> argtypes;
    auto stack_loc_iter = arg_locations.begin();
    for(int i=stack_loc_iter->first; i<=arg_locations.rbegin()->first; ) {
        int till_next_loc=stack_loc_iter->first-i;
        if(till_next_loc==0) {
            int entry_size=stack_loc_iter->second->size;
            argtypes.push_back({stack_loc_iter->second->type});
            i+=entry_size;
            ++stack_loc_iter;
        } else {
            if(till_next_loc>=4) {
                argtypes.push_back({TYPE_LONG_SIGN});
                i+=4;
            } else if(till_next_loc>=2) {
                argtypes.push_back({TYPE_WORD_SIGN});
                i+=2;
            } else {
                argtypes.push_back({TYPE_BYTE_SIGN});
                i+=1;
            }
        }
    }
    f = FunctionType::get({func->type->getReturnType()},argtypes,func->type->isVarArg());
    f->retVal = func->type->retVal;
    delete func->type;
    func->type = f;
}
CConv *CConv::create(CC_Type v)
{
    static C_CallingConvention *c_call      = nullptr;
    static Pascal_CallingConvention *p_call = nullptr;
    static Unknown_CallingConvention *u_call= nullptr;
    if(nullptr==c_call)
        c_call = new C_CallingConvention;
    if(nullptr==p_call)
        p_call = new Pascal_CallingConvention;
    if(nullptr==u_call)
        u_call = new Unknown_CallingConvention;
    switch(v) {
    case UNKNOWN: return u_call;
    case C: return c_call;
    case PASCAL: return p_call;
    }
    assert(false);
    return nullptr;
}

void C_CallingConvention::writeComments(QTextStream & ostr)
{
    ostr << " * C calling convention.\n";
}

void C_CallingConvention::calculateStackLayout(Function *func)
{
    calculateReturnLocations(func);
    rebuildArguments_FromStackLayout(func);
    calculateArgLocations_allOnStack(func);
}
void Pascal_CallingConvention::writeComments(QTextStream & ostr)
{
    ostr << " * Pascal calling convention.\n";
}
void Pascal_CallingConvention::calculateStackLayout(Function *func)
{
    calculateReturnLocations(func);
    //TODO: pascal args are passed in reverse order ?
    rebuildArguments_FromStackLayout(func);
    calculateArgLocations_allOnStack(func);
}
void Unknown_CallingConvention::writeComments(QTextStream & ostr)
{
    ostr << " * Unknown calling convention.\n";
}
void Unknown_CallingConvention::calculateStackLayout(Function *func)
{
    calculateReturnLocations(func);
    rebuildArguments_FromStackLayout(func);
    calculateArgLocations_allOnStack(func);
}

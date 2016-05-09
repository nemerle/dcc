#pragma once
#include "src/ui/RenderTags.h"

class IStructuredTextTarget {
public:
    virtual void TAGbegin(TAG_TYPE t,void *data)=0;
    virtual void TAGend(TAG_TYPE t)=0;
    virtual void prtt(const QString &v)=0;
    virtual void delChars(int v) = 0;

    virtual void addEOL() = 0; // some targets might want to disable newlines
    void addSpace(int n=1) {
        while(n--)
            prtt(" ");
    }
    void addTaggedString(TAG_TYPE t, QString v) {
        this->TAGbegin(t,nullptr);
        this->prtt(v);
        this->TAGend(t);
    }
    void addTaggedString(TAG_TYPE t, QString str, void *value) {
        this->TAGbegin(t,value);
        this->prtt(str);
        this->TAGend(t);
    }
};

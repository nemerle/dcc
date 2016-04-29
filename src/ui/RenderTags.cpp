#include "RenderTags.h"

QColor RenderTag_2_Color(TAG_TYPE tag_type)
{
    switch (tag_type)
    {
        case XT_invalid  : return QColor(255,255,255);
        case XT_blank    : return QColor(255,255,255);
        case XT_Symbol   : return QColor(57,109,165);
        case XT_Function : return QColor(255,255,255);
        case XT_Keyword  : return QColor(255,255,0);
        case XT_Class    : return QColor(255,255,0);
        case XT_K1       : return QColor(163,70,255);
        case XT_Comment  : return QColor(0,245,255);
        case XT_DataType : return QColor(100,222,192);
        case XT_Number   : return QColor(0,255,0);
        case XT_AsmStack : return QColor(0,70,255);
        case XT_AsmOffset: return QColor(70,180,70);
        case XT_AsmLabel : return QColor(255,180,70);
        case XT_FuncName : return QColor(255,0,255);
    }
    return QColor(255,255,255);
}

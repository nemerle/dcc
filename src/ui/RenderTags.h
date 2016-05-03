#pragma once

#include <QtGui/QColor>
enum TAG_TYPE
{
           XT_invalid = 0,
           XT_blank,
           XT_Symbol,
           XT_Function,
           XT_Keyword,      //Keywords, such as struct, union, for, while
           XT_Class,        //For comound types (struct/class/union)
           XT_K1,           //Braces {} []
           XT_Comment,      //Comments
           XT_DataType,     //
           XT_Number,       //
           XT_AsmStack,     //stack values
           XT_AsmOffset,    //seg:offset
           XT_AsmLabel,     //label name
           XT_FuncName,
};
struct tColorPair
{
        QColor color1;
        QColor color2;
};
extern tColorPair tbl_color[];

QColor RenderTag_2_Color(TAG_TYPE tag_type);

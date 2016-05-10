#ifndef PARSER_H
#define PARSER_H
class Function;
struct CALL_GRAPH;
struct STATE;
void FollowCtrl(Function & func, CALL_GRAPH * pcallGraph, STATE *pstate);
#endif // PARSER_H

#ifndef AUTOMATEDPLANNER_H
#define AUTOMATEDPLANNER_H

class Project;
class Function;
class Command;

class AutomatedPlanner
{
public:
    AutomatedPlanner();

    void planFor(Project & project);
    void planFor(Function & func);
protected:
    void addAction(Function &func,Command *cmd);
    void addAction(Project &func,Command *cmd);
};

#endif // AUTOMATEDPLANNER_H

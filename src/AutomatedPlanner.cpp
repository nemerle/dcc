#include "AutomatedPlanner.h"

#include "project.h"
#include "FollowControlFlow.h"

#include <QtCore/QDebug>
/**
 * @class AutomatedPlanner
 * @brief Class responsible for building command lists
 *
 * The goal for top level [Project] plan is to build a fully decompiled representation of source binaries
 */

AutomatedPlanner::AutomatedPlanner()
{

}
/**
 * @brief Given a state of a project, add actions that will advance the decompilation
 * @param project
 */
void AutomatedPlanner::planFor(Project &project) {
    // TODO: For now this logic is sprinkled all over the place, should move it here
    // IF NO BINARY IMAGE LOADED - > add SelectImage/SelectProject command
    // IF NO LOADER SELECTED -> add SelectLoader command
    // ...
}

void AutomatedPlanner::planFor(Function & func) {
    if(func.doNotDecompile())
        return; // for functions marked as non-decompileable we don't add any commands
    //TODO: Consider cases where commands are queued, but we can still plan some additional steps
    bool function_has_commands = Project::get()->hasCommands(func.shared_from_this());
    if(function_has_commands) {
        qDebug() << "Function "<<func.name<<"still has some commands queued, planning skipped";
    }
    switch(func.nStep) {
    case eNotDecoded:
        addAction(func,new FollowControlFlow(func.state));
    }
}

void AutomatedPlanner::addAction(Function & func, Command * cmd)
{
    Project::get()->addCommand(func.shared_from_this(),cmd);
}

void AutomatedPlanner::addAction(Project & func, Command * cmd)
{
    func.addCommand(cmd);
}

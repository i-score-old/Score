/*
 * A CSP container
 * Copyright © 2013, Théo de la Hogue, Clément Bossut
 *
 * License: This code is licensed under the terms of the "New BSD License"
 * http://creativecommons.org/licenses/BSD/
 */

/*!
 * \class CSP
 *
 *  The CSP class is a container for the constraint satisfaction problem which ask solutions to the solver
 *
 */

#include "CSP.h"
#include "CSPTypes.hpp"


CSP::CSP(ScenarioPtr pScenario)
{
    this->pScenario = pScenario;
}

TTErr CSP::addBox(TimeProcessPtr pBox)
{
    // TODO : add methods length, start, (end,) to TimeProcess
    int beginID = solver.addIntVar(1, pScenario->length(), pBox->start(), BEGIN_VAR_TYPE);
    int lengthID = solver.addIntVar(10, pScenario->length(), pBox->length(), LENGTH_VAR_TYPE);
    
    int endID = solver.addIntVar(1, pScenario->length(), pBox->end(), BEGIN_VAR_TYPE);
    int endLengthID = solver.addIntVar(0, pScenario->length(), 0, LENGTH_VAR_TYPE);
    
    // solver.addIntVar(min, max, val, weight)
    // Here, we have in fact no min and max.
    
    // mins are arbitrary values taken from the original CSP.cpp
    // max is the length of the parent Scenario
    // val is explicit
    // weights are "types" extracted from CSPTypes.hpp
    
    // The endLengthID is a weird value, but the solver seem to need it
    // TODO : Verify what I just said
    
    // TODO : What to do with these IDs ? Keep'em here or give'em to their associated TimeProcess
    
    return kTTErrNone;
}

TTErr CSP::removeBox(TimeProcessPtr pBox)
{
    // TODO : remove the variables from the solver, wherever the IDs are stored (read upward)
    
    // TODO : remove the associated intervals (or whatever is associated)
    
    return kTTErrGeneric;
}

TTErr CSP::addEvent(TimeEventPtr pEvent)
{
    // TODO : no sé que hacer, must start condition implementation first I think
    
    return kTTErrGeneric;
}

TTErr CSP::removeEvent(TimeEventPtr pEvent)
{
    // TODO : same thought as above
    
    return kTTErrGeneric;
}

TTerr CSP::addRelation(IntervalPtr pRel)
{
    // WARNING : TTEngines used to verify that the relation starts and ends at different points
    
    
    
    return kTTErrNone;
}

/** @file
 *
 * @ingroup scoreExtension
 *
 * @brief Loop class is a time process class to iterate other time processes
 *
 * @details The Loop class allows to ... @n@n
 *
 * @see TimePluginLib, TTTimeProcess, TimeContainer
 *
 * @authors Théo de la Hogue & Clément Bossut
 *
 * @copyright Copyright © 2013, Théo de la Hogue & Clément Bossut @n
 * This code is licensed under the terms of the "CeCILL-C" @n
 * http://www.cecill.info
 */

#ifndef __LOOP_H__
#define __LOOP_H__

#include "TimePluginLib.h"

#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>

/**	The Loop class allows to ...
 
 @see TimePluginLib, TTTimeProcess
 */
class Loop : public TimeContainerPlugin
{
	TTCLASS_SETUP(Loop)
	
    TTAddressItemPtr            mNamespace;                     ///< the namespace workspace of the loop
    
private :

    TTList                      mPatternProcesses;              ///< all registered time processes to execute at each iteration
    
    TTObject                    mPatternStartEvent;             ///< the event object which handles the start of the pattern execution
    
    TTObject                    mPatternEndEvent;               ///< the event object which handles the end of the pattern execution
    
    TTObject                    mPatternCondition;              ///< the condition object which handles next pattern iteration or the end of the loop

    /** Get parameters names needed by this time process
     @param	value           the returned parameter names
     @return                kTTErrNone */
	TTErr   getParameterNames(TTValue& value);
    
    /** Get all time processes objects
     @param value           all time processes objects
     @return                kTTErrGeneric if no process */
    TTErr   getTimeProcesses(TTValue& value);
    
    /** Get all time events objects
     @param value           all time events objects
     @return                kTTErrGeneric if no event */
    TTErr   getTimeEvents(TTValue& value);
    
    /** Get all time conditions objects
     @param value           all time conditions objects
     @return                kTTErrGeneric if no condition */
    TTErr   getTimeConditions(TTValue& value);
	
    /** Specific compilation method used to pre-processed data in order to accelarate Process method.
     the compiled attribute allows to know if the process needs to be compiled or not.
     @return                an error code returned by the compile method */
    TTErr   Compile();
    
    /** Specific process method on start
     @return                an error code returned by the process end method */
    TTErr   ProcessStart();
    
    /** Specific process method on end
     @return                an error code returned by the process end method */
    TTErr   ProcessEnd();
    
    /** Specific process method
     @param	inputValue      position of the scheduler
     @param	outputValue     return an error of the processing
     @return                an error code returned by the process method */
    TTErr   Process(const TTValue& inputValue, TTValue& outputValue);
    
    /** Specific process method for pause/resume
     @param	inputValue      boolean paused state of the scheduler
     @param	outputValue     return an error of the processing
     @return                an error code returned by the process paused method */
    TTErr   ProcessPaused(const TTValue& inputValue, TTValue& outputValue);
    
    /** Specific go to method to set the process at a date
     @param	inputValue      a date where to go relative to the duration of the time process, an optional boolean to temporary mute the process 
     @param	outputValue     nothing
     @return                an error code if the operation fails */
    TTErr   Goto(const TTValue& inputValue, TTValue& outputValue);
    
    /** needed to be handled by a TTXmlHandler
     @param	inputValue      ..
     @param	outputValue     ..
     @return                .. */
	TTErr	WriteAsXml(const TTValue& inputValue, TTValue& outputValue);
	TTErr	ReadFromXml(const TTValue& inputValue, TTValue& outputValue);
    
    /** Add a time process to the loop pattern
     @param	inputValue      a time process object
     @param	outputValue     nothing
     @return                an error code if the operation fails */
    TTErr   PatternAttach(const TTValue& inputValue, TTValue& outputValue);
    
    /** Remove a time process from the loop pattern
     @param	inputValue      a time process object
     @param	outputValue     nothing
     @return                an error code if the operation fails */
    TTErr   PatternDetach(const TTValue& inputValue, TTValue& outputValue);
};

typedef Loop* LoopPtr;

#endif // __LOOP_H__

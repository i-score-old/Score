/** @file
 *
 * @ingroup scoreLibrary
 *
 * @brief a class to define a process
 *
 * @see TTTimeEvent
 *
 * @authors Théo de la Hogue & Clément Bossut
 *
 * @copyright Copyright © 2013, Théo de la Hogue & Clément Bossut @n
 * This code is licensed under the terms of the "CeCILL-C" @n
 * http://www.cecill.info
 */

#include "TTTimeProcess.h"

#define thisTTClass         TTTimeProcess
#define thisTTClassName     "TimeProcess"
#define thisTTClassTags     "time, process"

/****************************************************************************************************/

TT_BASE_OBJECT_CONSTRUCTOR,
mContainer(NULL),
mName(kTTSymEmpty),
mDurationMin(0),
mDurationMax(0),
mMute(NO),
mVerticalPosition(0),
mVerticalSize(1),
mScheduler(NULL),
mStartEvent(NULL),
mStartEventCallback(NULL),
mEndEvent(NULL),
mEndEventCallback(NULL)
{
    TT_ASSERT("Correct number of args to create TTTimeProcess", arguments.size() == 1);
    
    TTValue    args;
    TTErr      err;
    TTValuePtr startEventBaton, endEventBaton;
    
    if (arguments.size() == 1)
        mContainer = arguments[0];
    
    // the rigid state handles the DurationMin and DurationMax attribute
    registerAttribute(kTTSym_rigid, kTypeBoolean, NULL, (TTGetterMethod)& TTTimeProcess::getRigid, (TTSetterMethod)& TTTimeProcess::setRigid);
    
    addAttribute(Name, kTypeSymbol);
    
    addAttributeWithSetter(DurationMin, kTypeUInt32);
    addAttributeWithSetter(DurationMax, kTypeUInt32);
    
    addAttribute(Mute, kTypeBoolean);
    
    addAttributeWithSetter(Color, kTypeLocalValue);
    addAttribute(VerticalPosition, kTypeUInt32);
    addAttribute(VerticalSize, kTypeUInt32);
    
    addAttribute(Scheduler, kTypeObject);
    addAttributeProperty(Scheduler, readOnly, YES);
    addAttributeProperty(Scheduler, hidden, YES);
    
    addAttribute(StartEvent, kTypeObject);
    addAttributeProperty(StartEvent, readOnly, YES);
    addAttributeProperty(StartEvent, hidden, YES);
    
    addAttribute(EndEvent, kTypeObject);
    addAttributeProperty(EndEvent, readOnly, YES);
    addAttributeProperty(EndEvent, hidden, YES);
    
    // the attributes below are not related to any TTTimeProcess member
    // but we need to declare them as attribute to ease the use of the class
    registerAttribute(TTSymbol("startDate"), kTypeUInt32, NULL, (TTGetterMethod)& TTTimeProcess::getStartDate, (TTSetterMethod)& TTTimeProcess::setStartDate);
    registerAttribute(TTSymbol("endDate"), kTypeUInt32, NULL, (TTGetterMethod)& TTTimeProcess::getEndDate, (TTSetterMethod)& TTTimeProcess::setEndDate);
    registerAttribute(TTSymbol("startCondition"), kTypeBoolean, NULL, (TTGetterMethod)& TTTimeProcess::getStartCondition, (TTSetterMethod)& TTTimeProcess::setStartCondition);
    registerAttribute(TTSymbol("endCondition"), kTypeBoolean, NULL, (TTGetterMethod)& TTTimeProcess::getEndCondition, (TTSetterMethod)& TTTimeProcess::setEndCondition);
    registerAttribute(kTTSym_duration, kTypeUInt32, NULL, (TTGetterMethod)& TTTimeProcess::getDuration);
    
    addMessage(ProcessStart);
    addMessage(ProcessEnd);
    addMessageWithArguments(Process);
    
    addMessageWithArguments(Move);
    addMessageWithArguments(Limit);
    
    addMessage(Play);
    addMessage(Stop);
    addMessage(Pause);
    addMessage(Resume);
    
	// needed to be handled by a TTXmlHandler
	addMessageWithArguments(WriteAsXml);
	addMessageProperty(WriteAsXml, hidden, YES);
	addMessageWithArguments(ReadFromXml);
	addMessageProperty(ReadFromXml, hidden, YES);
    
    // Create a start event callback to be notified and start the process execution
    mStartEventCallback = NULL;
    TTObjectBaseInstantiate(TTSymbol("callback"), &mStartEventCallback, kTTValNONE);
    
    startEventBaton = new TTValue(TTObjectBasePtr(this));
    
    mStartEventCallback->setAttributeValue(kTTSym_baton, TTPtr(startEventBaton));
    mStartEventCallback->setAttributeValue(kTTSym_function, TTPtr(&TTTimeProcessStartEventHappenCallback));
    
    // Create a end event callback to be notified and end the process execution
    mEndEventCallback = NULL;
    TTObjectBaseInstantiate(TTSymbol("callback"), &mEndEventCallback, kTTValNONE);
    
    endEventBaton = new TTValue(TTObjectBasePtr(this));
    
    mEndEventCallback->setAttributeValue(kTTSym_baton, TTPtr(endEventBaton));
    mEndEventCallback->setAttributeValue(kTTSym_function, TTPtr(&TTTimeProcessEndEventHappenCallback));
    
    // Creation of a scheduler based on the System scheduler plugin
    // Prepare callback argument to be notified of :
    //      - the progression
    args = TTValue((TTPtr)&TTTimeProcessSchedulerCallback);
    args.append((TTPtr)this);   // we have to store this as a pointer for Scheduler
    
    err = TTObjectBaseInstantiate(TTSymbol("System"), TTObjectBaseHandle(&mScheduler), args);
    
	if (err) {
        mScheduler = NULL;
		logError("TimeProcess failed to load the EcoMachine Scheduler");
    }
    
    // generate a random name
    mName = mName.random();
    
    // set default color to white
    mColor.append(255);
    mColor.append(255);
    mColor.append(255);
}

TTTimeProcess::~TTTimeProcess()
{
    setStartEvent(NULL);
    
    // Don't release start event here because it can be used by another time process
    
    // Release start event callback
    if (mStartEventCallback) {
        delete (TTValuePtr)TTCallbackPtr(mStartEventCallback)->getBaton();
        TTObjectBaseRelease(TTObjectBaseHandle(&mStartEventCallback));
        mStartEventCallback = NULL;
    }
    
    // Don't release end event here because it can be used by another time process
    
    setEndEvent(NULL);
    
    // Release end event callback
    if (mEndEventCallback) {
        delete (TTValuePtr)TTCallbackPtr(mEndEventCallback)->getBaton();
        TTObjectBaseRelease(TTObjectBaseHandle(&mEndEventCallback));
        mEndEventCallback = NULL;
    }
    
    // Release scheduler
    if (mScheduler) {
        TTObjectBaseRelease(TTObjectBaseHandle(&mScheduler));
        mScheduler = NULL;
    }
}
/*
TTErr TTTimeProcess::WriteAsXml(const TTValue& inputValue, TTValue& outputValue)
{
    TTXmlHandlerPtr     aXmlHandler = NULL;

	aXmlHandler = TTXmlHandlerPtr((TTObjectBasePtr)inputValue[0]);
    
    // Write the name
    xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "name", BAD_CAST mName.c_str());
    
    // Write the start event name
    xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "start", BAD_CAST TTTimeEventPtr(mStartEvent)->mName.c_str());
    
    // Write the end event name
    xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "end", BAD_CAST TTTimeEventPtr(mEndEvent)->mName.c_str());
    
    return kTTErrNone;
}

TTErr TTTimeProcess::ReadFromXml(const TTValue& inputValue, TTValue& outputValue)
{
    return kTTErrGeneric;
}
 */

TTErr TTTimeProcess::getRigid(TTValue& value)
{
    value = mDurationMin && mDurationMin && mDurationMin == mDurationMax;
    
    return kTTErrNone;
}

TTErr TTTimeProcess::setRigid(const TTValue& value)
{
    TTValue v, duration;
    
    if (value.size()) {
        
        if (value[0].type() == kTypeBoolean) {
            
            v = TTObjectBasePtr(this);
            
            // rigid means Limit(duration, duration)
            if (TTBoolean(value[0])) {
                
                if (!getDuration(duration)) {
                    
                    v.append(duration[0]);
                    v.append(duration[0]);
                }
            }
            // non rigid means Limit(durationMin, durationMax)
            else {
                v.append(mDurationMin);
                v.append(mDurationMax);
            }
            
            // if the time process is handled by a scenario
            if (mContainer)
                return mContainer->sendMessage(TTSymbol("TimeProcessLimit"), v, kTTValNONE);
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::setDurationMin(const TTValue& value)
{
    if (value.size()) {
        
        if (value[0].type() == kTypeUInt32) {
            
            // set minimal duration
            mDurationMin = TTUInt32(value[0]);
            
            // tell to the container the limit changes
            if (mContainer) {
                
                TTValue v = TTObjectBasePtr(this);
                v.append(mDurationMin);
                v.append(mDurationMax);
                return mContainer->sendMessage(TTSymbol("TimeProcessLimit"), v, kTTValNONE);
            }
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::setDurationMax(const TTValue& value)
{
    if (value.size()) {
        
        if (value[0].type() == kTypeUInt32) {
            
            // set maximal duration
            mDurationMax = TTUInt32(value[0]);
            
            // tell to the container the limit changes
            if (mContainer) {
                
                TTValue v = TTObjectBasePtr(this);
                v.append(mDurationMin);
                v.append(mDurationMax);
                return mContainer->sendMessage(TTSymbol("TimeProcessLimit"), v, kTTValNONE);
            }
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::getStartDate(TTValue& value)
{
    value = TTTimeEventPtr(mStartEvent)->mDate;
    return kTTErrNone;
}

TTErr TTTimeProcess::setStartDate(const TTValue& value)
{
    if (value.size()) {
            
        if (value[0].type() == kTypeUInt32) {
                
            // tell to the container the process date changes
            if (mContainer) {
                
                TTValue v = TTObjectBasePtr(this);
                v.append(TTUInt32(value[0]));
                v.append(mStartDate);
                return mContainer->sendMessage(TTSymbol("TimeProcessMove"), v, kTTValNONE);
            }
            
            // or set the start event date directly
            else
                mStartEvent->setAttributeValue(kTTSym_date, value);
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::getStartCondition(TTValue& value)
{
    value = mStartCondition;
    return kTTErrNone;
}

TTErr TTTimeProcess::setStartCondition(const TTValue& value)
{
    if (value.size() == 1) {
        
        if (value[0].type() == kTypeObject) {
            
            mStartCondition = value[0];
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::getEndDate(TTValue& value)
{
    value = mEndDate;
    return kTTErrNone;
}

TTErr TTTimeProcess::setEndDate(const TTValue& value)
{
    if (value.size()) {
            
        if (value[0].type() == kTypeUInt32) {
                
            // tell to the container the process date changes
            if (mContainer) {
                
                TTValue v = TTObjectBasePtr(this);
                v.append(mStartDate);
                v.append(TTUInt32(value[0]));
                return mContainer->sendMessage(TTSymbol("TimeProcessMove"), v, kTTValNONE);
            }
            
            // or set the end event date directly
            else
                mEndEvent->setAttributeValue(kTTSym_date, value);
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::getEndCondition(TTValue& value)
{
    value = mEndCondition;
    return kTTErrNone;
}

TTErr TTTimeProcess::setEndCondition(const TTValue& value)
{
    if (value.size() == 1) {
        
        if (value[0].type() == kTypeBoolean) {
            
            mEndCondition = value[0];
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::getDuration(TTValue& value)
{
    // the end must be after the start
    if (mEndDate >= mStartDate) {
            
        value = TTValue( TTUInt32( abs(mDuration) ) );
        
        return kTTErrNone;
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::setColor(const TTValue& value)
{
    mColor = value;
    
    return kTTErrNone;
}

TTErr TTTimeProcess::getIntermediateEvents(TTValue& value)
{
    mIntermediateEvents.assignToValue(value);
    
    return kTTErrNone;
}

TTErr TTTimeProcess::Move(const TTValue& inputValue, TTValue& outputValue)
{
    if (inputValue.size() == 2) {
        
        if (inputValue[0].type() == kTypeUInt32 && inputValue[1].type() == kTypeUInt32) {
            
            if (TTUInt32(inputValue[0]) <= TTUInt32(inputValue[1])) {
                
                // if the time process is handled by a container
                if (mContainer) {
                    
                    TTValue v = TTObjectBasePtr(this);
                    v.append(TTUInt32(inputValue[0]));
                    v.append(TTUInt32(inputValue[1]));
                    return mContainer->sendMessage(TTSymbol("TimeProcessMove"), v, kTTValNONE);
                }
            }
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::Limit(const TTValue& inputValue, TTValue& outputValue)
{
    if (inputValue.size() == 2) {
        
        if (inputValue[0].type() == kTypeUInt32 && inputValue[1].type() == kTypeUInt32) {
            
            // set minimal and maximal duration
            mDurationMin = TTUInt32(inputValue[0]);
            mDurationMax = TTUInt32(inputValue[1]);
            
            // if the time process is handled by a scenario
            if (mContainer) {
                
                TTValue v = TTObjectBasePtr(this);
                v.append(mDurationMin);
                v.append(mDurationMax);
                return mContainer->sendMessage(TTSymbol("TimeProcessLimit"), v, kTTValNONE);
            }
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcess::Play()
{    
    return mStartEvent->sendMessage(kTTSym_Happen);
}

TTErr TTTimeProcess::Stop()
{
    return mEndEvent->sendMessage(kTTSym_Happen);
}

TTErr TTTimeProcess::Pause()
{
    mScheduler->sendMessage(kTTSym_Pause);
    
    return ProcessPaused(kTTBoolYes, kTTValNONE);
}

TTErr TTTimeProcess::Resume()
{
    mScheduler->sendMessage(kTTSym_Resume);
    
    return ProcessPaused(kTTBoolNo, kTTValNONE);
}

TTTimeEventPtr TTTimeProcess::getStartEvent()
{
    return (TTTimeEventPtr)mStartEvent;
}

TTTimeEventPtr TTTimeProcess::getEndEvent()
{
    return (TTTimeEventPtr)mEndEvent;
}

TTErr TTTimeProcess::setStartEvent(TTTimeEventPtr aTimeEvent)
{
    TTMessagePtr    aMessage;
    TTErr           err;
    
    if (mStartEvent) {
        
        // Stop start event happening observation
        err = mStartEvent->findMessage(kTTSym_Happen, &aMessage);
        
        if(!err)
            aMessage->unregisterObserverForNotifications(*mStartEventCallback);
    }
    
    // Replace the start event by the new one
    mStartEvent = aTimeEvent;
    
    // Observe start event happening
    if (mStartEvent) {
        
        err = mStartEvent->findMessage(kTTSym_Happen, &aMessage);
    
        if(!err)
            return aMessage->registerObserverForNotifications(*mStartEventCallback);
    }
    
    return kTTErrNone;
}

TTErr TTTimeProcess::setEndEvent(TTTimeEventPtr aTimeEvent)
{
    TTMessagePtr    aMessage;
    TTErr           err;
    
    if (mEndEvent) {
        
        // Stop end event happening observation
        err = mEndEvent->findMessage(kTTSym_Happen, &aMessage);
        
        if(!err)
            aMessage->unregisterObserverForNotifications(*mEndEventCallback);
    }
    
    // Replace the end event by the new one
    mEndEvent = aTimeEvent;
    
    // Observe end event happening
    if (mEndEvent) {
        
        err = mEndEvent->findMessage(kTTSym_Happen, &aMessage);
    
        if(!err)
            return aMessage->registerObserverForNotifications(*mEndEventCallback);
    }
    
    return kTTErrNone;
}

#if 0
#pragma mark -
#pragma mark Some Methods
#endif

TTErr TTTimeProcessStartEventHappenCallback(TTPtr baton, TTValue& data)
{
    TTTimeProcessPtr    aTimeProcess;
    TTValuePtr          b;
    TTValue             v;
    TTUInt32            start, end;
    
	// unpack baton (a time process)
	b = (TTValuePtr)baton;
	aTimeProcess = TTTimeProcessPtr((TTObjectBasePtr)(*b)[0]);
    
    // if the time process not muted
	if (!aTimeProcess->mMute) {
        
        // close start event listening
        aTimeProcess->mStartEvent->setAttributeValue(kTTSym_ready, NO);
        
        // use the specific start process method of the time process
        aTimeProcess->ProcessStart();
        
        // set the internal active flag
        aTimeProcess->active = YES;
        
        // launch the scheduler
        aTimeProcess->mStartEvent->getAttributeValue(kTTSym_date, v);
        start = v[0];
        
        aTimeProcess->mEndEvent->getAttributeValue(kTTSym_date, v);
        end = v[0];
        
        if (end > start) {
            
            v = TTFloat64(end - start);
            
            aTimeProcess->mScheduler->setAttributeValue(kTTSym_duration, v);
            aTimeProcess->mScheduler->sendMessage(kTTSym_Go);
            
            return kTTErrNone;
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeProcessEndEventHappenCallback(TTPtr baton, TTValue& data)
{
    TTTimeProcessPtr    aTimeProcess;
    TTValuePtr          b;
    
	// unpack baton (a time process)
	b = (TTValuePtr)baton;
	aTimeProcess = TTTimeProcessPtr((TTObjectBasePtr)(*b)[0]);
    
    // if the time process not muted, stop the scheduler
    // note : the ProcessStart method is called inside TTTimeProcessSchedulerCallback
	if (!aTimeProcess->mMute) {
        
        // stop the scheduler
        aTimeProcess->mScheduler->sendMessage(kTTSym_Stop);
        
        // set the internal active flag
        aTimeProcess->active = NO;
        
        // close end trigger listening
        aTimeProcess->mEndEvent->setAttributeValue(kTTSym_ready, NO);
        
        // use the specific process end method of the time process
        aTimeProcess->ProcessEnd();
        
        return kTTErrNone;
    }
    
    return kTTErrGeneric;
}

void TTTimeProcessSchedulerCallback(TTPtr object, TTFloat64 progression, TTFloat64 realTime)
{
	TTTimeProcessPtr	aTimeProcess = (TTTimeProcessPtr)object;
    
    // use the specific process method
    if (aTimeProcess->active)
       aTimeProcess->Process(TTValue(progression, realTime), kTTValNONE);
    else
        std::cout << "TTTimeProcessSchedulerCallback : avoid last scheduler tick" << std::endl;
}
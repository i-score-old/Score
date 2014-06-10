/** @file
 *
 * @ingroup scoreLibrary
 *
 * @brief a class to define an event
 *
 * @see TTTimeProcess
 *
 * @authors Théo de la Hogue & Clément Bossut
 *
 * @copyright Copyright © 2013, Théo de la Hogue & Clément Bossut @n
 * This code is licensed under the terms of the "CeCILL-C" @n
 * http://www.cecill.info
 */

#include "TTTimeEvent.h"

#define thisTTClass         TTTimeEvent
#define thisTTClassName     "TimeEvent"
#define thisTTClassTags     "time, event"

/****************************************************************************************************/

TT_BASE_OBJECT_CONSTRUCTOR,
mContainer(NULL),
mName(kTTSymEmpty),
mDate(0),
mStatus(kTTSym_eventWaiting),
mMute(NO),
mState(NULL),
mCondition(NULL)
{
    TTValue none;
    
    TT_ASSERT("Correct number of args to create TTTimeEvent", arguments.size() == 1 || arguments.size() == 2);
    
    if (arguments.size() >= 1)
        mDate = arguments[0];

    if (arguments.size() == 2)
        mContainer = arguments[1];
    
    addAttribute(Container, kTypeObject);

    addAttribute(Name, kTypeSymbol);
   	addAttributeWithSetter(Date, kTypeUInt32);
    addAttribute(Mute, kTypeBoolean);
    addAttribute(State, kTypeObject);
    addAttributeWithSetter(Condition, kTypeObject);
    addAttributeWithSetter(Status, kTypeSymbol);
    
    addMessage(Trigger);
    addMessage(Happen);
    addMessage(Dispose);
    addMessageWithArguments(StateAddressGetValue);
    addMessageWithArguments(StateAddressSetValue);
    addMessageWithArguments(StateAddressClear);
    
	// needed to be handled by a TTXmlHandler
	addMessageWithArguments(WriteAsXml);
	addMessageProperty(WriteAsXml, hidden, YES);
	addMessageWithArguments(ReadFromXml);
	addMessageProperty(ReadFromXml, hidden, YES);
	
    // create a script
    TTObjectBaseInstantiate(kTTSym_Script, TTObjectBaseHandle(&mState), none);
    
    // generate a random name
    mName = mName.random();
}

TTTimeEvent::~TTTimeEvent()
{
    if (mState) {
        TTObjectBaseRelease(TTObjectBaseHandle(&mState));
        mState = NULL;
    }
}

TTErr TTTimeEvent::setDate(const TTValue& value)
{
    TTUInt32 newDate = value[0];
    
    // filter repetitions
    if (newDate != mDate) {
        
        // set the internal date value
        mDate = newDate;
        
        // notify each date attribute observers
        sendNotification(kTTSym_EventDateChanged, TTObjectBasePtr(this));
    }
    
    return kTTErrNone;
}

TTErr TTTimeEvent::setCondition(const TTValue& value)
{
    if (value.size() == 1) {
        
        if (value[0].type() == kTypeObject) {
            
            // set the condition object
            mCondition = value[0];
            
            // tell the container the event is becoming (or not) conditioned
            if (mContainer) {
                
                TTValue none, v = TTObjectBasePtr(this);
                v.append(mCondition);
                return mContainer->sendMessage(TTSymbol("TimeEventCondition"), v, none);
            }
        }
    }
    else
        mCondition = NULL;
    
    return kTTErrNone;
}

TTErr TTTimeEvent::setStatus(const TTValue& value)
{
    TTSymbol    lastStatus = mStatus;
    TTValue     v = TTObjectBasePtr(this);
    
    // set status
    mStatus = value[0];

    // notify each attribute observers
    v.append(mStatus);
    v.append(lastStatus);
    
    sendNotification(kTTSym_EventStatusChanged, v);
    
    return kTTErrNone;
}

TTErr TTTimeEvent::Trigger()
{
    // an event needs to be into a running container to be triggered
    if (mContainer) {
        
        TTValue none, v;
        
        mContainer->getAttributeValue("running", v);
        TTBoolean running = v[0];
        
        if (running) {
            
            // if not pending : do nothing
            if (mStatus != kTTSym_eventPending)
                return kTTErrGeneric;
            
            // if the event muted
            if (mMute)
                return kTTErrNone;
            
            v = TTObjectBasePtr(this);
            return mContainer->sendMessage(TTSymbol("TimeEventTrigger"), v, none);
        }
        
        return kTTErrGeneric;
    }
    
    // otherwise make it happens now
    return Happen();
}

TTErr TTTimeEvent::Dispose()
{
    // an event needs to be into a running container to be disposed
    if (mContainer) {
        
        TTValue none, v;
        
        mContainer->getAttributeValue("running", v);
        TTBoolean running = v[0];
        
        if (running) {
            
            // if already happened or disposed : do nothing
            if (mStatus == kTTSym_eventDisposed ||
                mStatus == kTTSym_eventHappened)
                return kTTErrGeneric;
            
            // change the status before
            setStatus(kTTSym_eventDisposed);
            
            v = TTObjectBasePtr(this);
            return mContainer->sendMessage(TTSymbol("TimeEventDispose"), v, none);
        }
    }
    
    return kTTErrNone;
}

TTErr TTTimeEvent::Happen()
{
    TTErr err = kTTErrNone;

    // if the event is not muted
    if (!mMute) {
    
        // recall the state
        err = mState->sendMessage(kTTSym_Run);
    }
    
    setStatus(kTTSym_eventHappened);
    return err;
}

TTErr TTTimeEvent::StateAddressGetValue(const TTValue& inputValue, TTValue& outputValue)
{
    TTValue         v;
    TTAddress       address;
    TTListPtr       lines;
    TTDictionaryBasePtr aLine;
    TTErr           err;
    
    if (inputValue.size() == 1) {
        
        if (inputValue[0].type() == kTypeSymbol) {
            
            address = inputValue[0];
            
            // get the lines of the state
            mState->getAttributeValue(kTTSym_lines, v);
            lines = TTListPtr(TTPtr(v[0]));
            
            // find the line at address
            err = lines->find(&TTScriptFindAddress, (TTPtr)&address, v);
            
            if (err)
                return err;
            
            aLine = TTDictionaryBasePtr((TTPtr)v[0]);
            
            // get the value
            aLine->getValue(outputValue);
            
            return  kTTErrNone;
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeEvent::StateAddressSetValue(const TTValue& inputValue, TTValue& outputValue)
{
    TTValue         v, command;
    TTAddress       address;
    TTValuePtr      aValue;
    TTListPtr       lines;
    TTDictionaryBasePtr aLine;
    TTErr           err;
    
    if (inputValue.size() == 2) {
        
        if (inputValue[0].type() == kTypeSymbol && inputValue[1].type() == kTypePointer) {
            
            address = inputValue[0];
            aValue = TTValuePtr(TTPtr(inputValue[1]));
            
            // get the lines of the state
            mState->getAttributeValue(kTTSym_lines, v);
            lines = TTListPtr(TTPtr(v[0]));
            
            // find the line at address
            err = lines->find(&TTScriptFindAddress, (TTPtr)&address, v);
            
            // if the line doesn't exist : append it to the state
            if (err) {
                
                command = *aValue;
                command.prepend(address);
                
                mState->sendMessage(TTSymbol("AppendCommand"), command, v);
            }
            else {
            
                aLine = TTDictionaryBasePtr((TTPtr)v[0]);
            
                // set the value
                aLine->setValue(*aValue);
            }
            
            return  kTTErrNone;
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeEvent::StateAddressClear(const TTValue& inputValue, TTValue& outputValue)
{
    TTValue none;

    if (inputValue.size() == 1) {
        
        if (inputValue[0].type() == kTypeSymbol) {
            
            // remove the lines of the state
            return mState->sendMessage(TTSymbol("RemoveCommand"), inputValue, none);
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeEvent::WriteAsXml(const TTValue& inputValue, TTValue& outputValue)
{
	TTXmlHandlerPtr	aXmlHandler = NULL;
    TTValue         v;
    TTString        s;
	
	aXmlHandler = TTXmlHandlerPtr((TTObjectBasePtr)inputValue[0]);
	
    // Write the date
    v = mDate;
    v.toString();
    s = TTString(v[0]);
    xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "date", BAD_CAST s.data());
    
    // Write the mute
    v = mMute;
    v.toString();
    s = TTString(v[0]);
    xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "mute", BAD_CAST s.data());
    
    // Write the name of the condition object
    if (mCondition) {
        
        mCondition->getAttributeValue(kTTSym_name, v);
        v.toString();
        s = TTString(v[0]);
        xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "condition", BAD_CAST s.data());
    }
    
    // Write the state
    v = TTObjectBasePtr(mState);
    aXmlHandler->setAttributeValue(kTTSym_object, v);
    aXmlHandler->sendMessage(kTTSym_Write);
    
	return kTTErrNone;
}

TTErr TTTimeEvent::ReadFromXml(const TTValue& inputValue, TTValue& outputValue)
{
	TTXmlHandlerPtr	aXmlHandler = NULL;
    TTValue         v;
	
	aXmlHandler = TTXmlHandlerPtr((TTObjectBasePtr)inputValue[0]);
    
    // Event node
    if (aXmlHandler->mXmlNodeName == kTTSym_event) {
        
        // get the date
        if (!aXmlHandler->getXmlAttribute(kTTSym_date, v, NO)) {
            
            if (v.size() == 1) {
                
                if (v[0].type() == kTypeUInt32) {
                    
                    this->setDate(v);
                }
            }
        }
        
        // get the mute
        if (!aXmlHandler->getXmlAttribute(kTTSym_mute, v, NO)) {
            
            if (v.size() == 1) {
                
                if (v[0].type() == kTypeInt32) {
                    
                    this->mMute = v[0];
                }
            }
        }
        
        // get the condition object name
        if (!aXmlHandler->getXmlAttribute(kTTSym_condition, v, YES)) {
            
            if (v.size() == 1) {
                
                if (v[0].type() == kTypeSymbol) {
                    
                    // do nothing
                    // the time condition will be registered after so we can't link the time event to an unexisting object
                }
            }
        }
    }
    
    // Command node
    if (aXmlHandler->mXmlNodeName == kTTSym_command) {
        
        // Pass the xml handler to the current state to fill his data structure
        v = TTObjectBasePtr(mState);
        aXmlHandler->setAttributeValue(kTTSym_object, v);
        return aXmlHandler->sendMessage(kTTSym_Read);
    }
	
	return kTTErrNone;
}

#if 0
#pragma mark -
#pragma mark Some Methods
#endif

TTBoolean TTSCORE_EXPORT TTTimeEventCompareDate(TTValue& v1, TTValue& v2)
{
    TTObjectBasePtr timeEvent1 = v1[0];
    TTObjectBasePtr timeEvent2 = v2[0];
    
    return TTTimeEventPtr(timeEvent1)->mDate < TTTimeEventPtr(timeEvent2)->mDate;
}

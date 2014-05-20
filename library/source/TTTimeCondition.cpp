/** @file
 *
 * @ingroup scoreLibrary
 *
 * @brief a class to define a condition and a set of different cases
 *
 * @see TTTimeCondition
 *
 * @authors Théo de la Hogue & Clément Bossut
 *
 * @copyright Copyright © 2013, Théo de la Hogue & Clément Bossut @n
 * This code is licensed under the terms of the "CeCILL-C" @n
 * http://www.cecill.info
 */

#include "TTTimeCondition.h"

#define thisTTClass         TTTimeCondition
#define thisTTClassName     "TimeCondition"
#define thisTTClassTags     "time, condition"

/****************************************************************************************************/

TT_BASE_OBJECT_CONSTRUCTOR,
mContainer(NULL),
mActive(NO),
mReady(NO),
mPendingCounter(0)
{
    TT_ASSERT("Correct number of args to create TTTimeCondition", arguments.size() == 1);
    
    if (arguments.size() == 1)
        mContainer = arguments[0];

    addAttribute(Name, kTypeSymbol);
    
    addAttributeWithSetter(Active, kTypeBoolean);
    
    addAttribute(Ready, kTypeBoolean);
    addAttributeProperty(Ready, readOnly, YES);
    
    addAttributeWithGetterAndSetter(DisposeExpression, kTypeSymbol);
    
    registerAttribute(TTSymbol("expressions"), kTypeLocalValue, NULL, (TTGetterMethod)& TTTimeCondition::getExpressions, NULL);
    registerAttribute(TTSymbol("events"), kTypeLocalValue, NULL, (TTGetterMethod)& TTTimeCondition::getEvents, NULL);

    addMessageWithArguments(EventAdd);
    addMessageWithArguments(EventRemove);
    addMessageWithArguments(EventExpression);
    addMessageWithArguments(EventDefault);
    addMessageWithArguments(ExpressionFind);
    addMessageWithArguments(DefaultFind);
    addMessageWithArguments(ExpressionTest);
    
	// needed to be handled by a TTXmlHandler
	addMessageWithArguments(WriteAsXml);
	addMessageProperty(WriteAsXml, hidden, YES);
	addMessageWithArguments(ReadFromXml);
	addMessageProperty(ReadFromXml, hidden, YES);
    
    // needed to be notified by events
    addMessageWithArguments(EventDateChanged);
    addMessageWithArguments(EventStatusChanged);
	
    // generate a random name
    mName = mName.random();
}

TTTimeCondition::~TTTimeCondition()
{
    TTValue v;
    
    // disable condition
    mActive = NO;
    
    // update each event condition
    v = TTObjectBasePtr(NULL);
    for (TTCaseMapIterator it = mCases.begin() ; it != mCases.end() ; it++)
        TTObjectBasePtr(it->first)->setAttributeValue(kTTSym_condition, v);
    
    // destroy all receivers;
    deleteReceivers();
}

TTErr TTTimeCondition::setActive(const TTValue& value)
{
    TTBoolean newActive = value[0];
    
    // filter repetitions
    if (newActive != mActive) {
        
        // if the condition is ready to be active
        if (newActive && mReady) {
            
            mActive = YES;
            
            // create the receivers
            TTCaseMapIterator it;
            
            // for each trigger case
            for(it = mCases.begin() ; it != mCases.end() ; it++)
                addReceiver(it->second.trigger.getAddress());
            
            // for dispose case
            addReceiver(mDispose.getAddress());

            return kTTErrNone;
        }
        
        if (!newActive) {
            
            mActive = NO;
         
            deleteReceivers();
            
            return kTTErrNone;
        }
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeCondition::getExpressions(TTValue& value)
{
    value.clear();

    // for each event, append the associated expressions to the result
    for (TTCaseMapIterator it = mCases.begin() ; it != mCases.end() ; it++) {
        value.append(it->second.trigger);
    }

    // append the dispose expression
    value.append(mDispose);
    
    return kTTErrNone;
}

TTErr TTTimeCondition::getEvents(TTValue& value)
{
    value.clear();

    // for each case, append the event to the result
    for (TTCaseMapIterator it = mCases.begin() ; it != mCases.end() ; it++) {
        value.append((TTObjectBasePtr)it->first); // cast to TTObjectBasePtr to associate the type kTypeObject and not kTypePointer
    }

    return kTTErrNone;
}

TTErr TTTimeCondition::EventAdd(const TTValue& inputValue, TTValue& outputValue)
{
    TTTimeEventPtr  event = NULL;
    Comportment     aComportment;
    TTValue         v;

    switch (inputValue.size()) {/* TODO : don't know how to pass Comportment as TTValue
            
        // if we have two arguments
        case 2 :
            
            // if the second argument isn't a symbol
            if (inputValue[1].type() != kTypeSymbol)
                
                // return an error TODO : should warn the user
                return kTTErrInvalidType;
            
            // if it's a symbol : convert it to an expression
            ExpressionParseFromValue(inputValue[1], anExpression);

            // add receivers for the address if needed
            if (anExpression.getAddress() != kTTAdrsEmpty)
                addReceiver(anExpression.getAddress());
*/
        // if we have one or two arguments
        case 1 :
            
            // if the first argument isn't an object
            if (inputValue[0].type() != kTypeObject)
                
                // return en error TODO : should warn the user
                return kTTErrInvalidType;

            // if it's an object : convert it to an event
            event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
            
            // insert the event with an expression
            mCases.insert({{event, aComportment}});

            // increment the pending counter
            mPendingCounter++;

            // set the event to waiting
            event->setAttributeValue(kTTSym_status, kTTSym_eventWaiting); // CB TODO : Why ?
            
            // tell the event it is conditioned
            v = TTObjectBasePtr(this);
            event->setAttributeValue(kTTSym_condition, v);
            
            // observe the event
            event->registerObserverForNotifications(*this);
            
            // return no error
            return kTTErrNone;
        
        // if there is less than 1 or more than 2 arguments
        default :
            
            // return an error TODO : should warn the user
            return kTTErrWrongNumValues;
    }
    
    // never evaluated
    return kTTErrGeneric;
}

TTErr TTTimeCondition::EventRemove(const TTValue& inputValue, TTValue& outputValue)
{
    TTTimeEventPtr      event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
    TTCaseMapIterator   it = mCases.find(event);
    
    // if the event exists
    if (it != mCases.end()) {
        
        TTValue     v;
        
        // remove the case
        mCases.erase(it);

        // decrement the unready counter
        mPendingCounter--;
        
        // tell the event it is not conditioned anymore
        v = TTObjectBasePtr(NULL);
        event->setAttributeValue(kTTSym_condition, v);
        
        // don't observe the event anymore
        event->unregisterObserverForNotifications(*this);
        
        return kTTErrNone;
    }

    return kTTErrValueNotFound;
}

TTErr TTTimeCondition::EventExpression(const TTValue& inputValue, TTValue& outputValue)
{
    TTTimeEventPtr      event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
    TTCaseMapIterator   it = mCases.find(event);
    
    // if the event exists
    if (it != mCases.end()) {
        
        // replace the old expression by the new one
        Expression newExpression;
        
        ExpressionParseFromValue(inputValue[1], newExpression);
        
        mCases[it->first].trigger = newExpression;

        return kTTErrNone;
    }
    
    return kTTErrValueNotFound;
}

TTErr TTTimeCondition::EventDefault(const TTValue &inputValue, TTValue &outputValue)
{
    TTTimeEventPtr    event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
    TTCaseMapIterator it = mCases.find(event);

    // if the event exists
    if (it != mCases.end()) {

        // change its default comportment
        mCases[it->first].dflt = TTBoolean(inputValue[1]);

        return kTTErrNone;
    }

    return kTTErrValueNotFound;
}

TTErr TTTimeCondition::ExpressionFind(const TTValue& inputValue, TTValue& outputValue)
{
    TTTimeEventPtr      event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
    TTCaseMapIterator   it = mCases.find(event);

    // if the event exists
    if (it != mCases.end()) {
        
        outputValue = it->second.trigger;
        return kTTErrNone;
    }
    
    return kTTErrValueNotFound;
}

TTErr TTTimeCondition::DefaultFind(const TTValue& inputValue, TTValue& outputValue)
{
    TTTimeEventPtr      event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
    TTCaseMapIterator   it = mCases.find(event);

    // if the event exists
    if (it != mCases.end()) {

        outputValue = it->second.dflt;
        return kTTErrNone;
    }

    return kTTErrValueNotFound;
}

TTErr TTTimeCondition::getDisposeExpression(TTValue &value)
{
    value.clear();

    value.append(mDispose);

    return kTTErrNone;
}

TTErr TTTimeCondition::setDisposeExpression(const TTValue &value)
{
    ExpressionParseFromValue(value, mDispose);

    return kTTErrNone;
}

TTErr TTTimeCondition::ExpressionTest(const TTValue& inputValue, TTValue& outputValue)
{
    Expression      anExpression;
    TTObjectBasePtr aReceiver;
    TTValue         v;
    
    // parse the input value
    ExpressionParseFromValue(inputValue, anExpression);
    
    // get the receiver for the expression address
    if (!mReceivers.lookup(anExpression.getAddress(), v)) {
     
        aReceiver = v[0];
        
        // ask the value at this address
        return aReceiver->sendMessage(kTTSym_Get);
    }
    
    return kTTErrGeneric;
}

TTErr TTTimeCondition::WriteAsXml(const TTValue& inputValue, TTValue& outputValue)
{
	TTXmlHandlerPtr	aXmlHandler = NULL;
    TTObjectBasePtr event;
    TTValue         v, keys;
    TTSymbol        key, name;
    TTCaseMapIterator it;
	
	aXmlHandler = TTXmlHandlerPtr((TTObjectBasePtr)inputValue[0]);
    
    // Write the name
    xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "name", BAD_CAST mName.c_str());

    // Write the dispose expression
    xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "dispose", BAD_CAST mDispose.c_str());
    
    // Write each case
    for (it = mCases.begin(); it != mCases.end(); it++) {
        
        Comportment aComportment = it->second;
        
        event = it->first;
        
        // Start a case node
        xmlTextWriterStartElement((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "case");
        
        // Write the event name
        event->getAttributeValue(kTTSym_name, v);
        name = v[0];
        xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "event", BAD_CAST name.c_str());
        
        // Write the comportment
        xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "trigger", BAD_CAST aComportment.trigger.c_str());
        xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "default", BAD_CAST (aComportment.dflt ? "1" : "0"));

        // Close the case node
        xmlTextWriterEndElement((xmlTextWriterPtr)aXmlHandler->mWriter);
    }
    
	return kTTErrNone;
}

TTErr TTTimeCondition::ReadFromXml(const TTValue& inputValue, TTValue& outputValue)
{
	TTXmlHandlerPtr	aXmlHandler = NULL;
    TTValue         v, out;
	
	aXmlHandler = TTXmlHandlerPtr((TTObjectBasePtr)inputValue[0]);
    
    // Condition node
    if (aXmlHandler->mXmlNodeName == kTTSym_condition) {
        
        // Get the name
        if (!aXmlHandler->getXmlAttribute(kTTSym_name, v, YES)) {
            
            if (v.size() == 1) {
                
                if (v[0].type() == kTypeSymbol) {
                    
                    mName = v[0];
                }
            }
        }

        // Get the dispose expression
        if (!aXmlHandler->getXmlAttribute(TTSymbol("dispose"), v, YES)) {

            if (v.size() == 1) {

                if (v[0].type() == kTypeSymbol) {

                    ExpressionParseFromValue(v, mDispose);
                }
            }
        }
    }
    
    // Case node
    if (aXmlHandler->mXmlNodeName == kTTSym_case) {
        
        // get the event
        if (!aXmlHandler->getXmlAttribute(kTTSym_event, v, YES)) {
            
            // Find the event using his name from our container
            if (!mContainer->sendMessage(TTSymbol("TimeEventFind"), v, out)) {
                
                EventAdd(out, v); // TODO : better using the second argument

                // get the expressions
                if (!aXmlHandler->getXmlAttribute(TTSymbol("trigger"), v, YES)) {
                    out.append(v[0]);
                    EventExpression(out, v);
                    out.pop_back();
                }
                if (!aXmlHandler->getXmlAttribute(TTSymbol("default"), v, NO)) {
                    out.append(v[0] == 1);
                    EventDefault(out, v);
                }
            }
        }
    }
	
	return kTTErrNone;
}

TTErr TTTimeCondition::EventDateChanged(const TTValue& inputValue, TTValue& outputValue)
{
    TT_ASSERT("TTTimeCondition::EventDateChanged : inputValue is correct", inputValue.size() == 1 && inputValue[0].type() == kTypeObject);
    
    TTTimeEventPtr      event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
    TTCaseMapIterator   it = mCases.find(event);
    TTUInt32            date;
    TTValue             v;
    
    // if the event exists
    if (it != mCases.end()) {
        
        // get the date
        event->getAttributeValue(kTTSym_date, v);
        date = v[0];
        
        return kTTErrNone;
    }
    
    TTLogError("TTTimeCondition::EventDateChanged : wrong event\n");
    return kTTErrGeneric;
}

TTErr TTTimeCondition::EventStatusChanged(const TTValue& inputValue, TTValue& outputValue)
{
    TT_ASSERT("TTTimeCondition::EventStatusChanged : inputValue is correct", inputValue.size() == 3 && inputValue[0].type() == kTypeObject);
    
    TTTimeEventPtr          event = TTTimeEventPtr(TTObjectBasePtr(inputValue[0]));
    TTCaseMapIterator       it = mCases.find(event);
    TTSymbol                newStatus = inputValue[1], oldStatus = inputValue[2];
    TTValue                 v;

    TT_ASSERT("TTTimeCondition::EventStatusChanged : status effectively changed", newStatus != oldStatus);
    
    // if the event exists
    if (it != mCases.end()) {
        
        if (newStatus == kTTSym_eventPending && --mPendingCounter == 0) {
            setReady(YES);
        } else if (oldStatus == kTTSym_eventPending && mPendingCounter++ == 0 && mReady == YES) {
            setReady(NO);
            applyDefaults();
        }
        
        return kTTErrNone;
    }
    
    TTLogError("TTTimeCondition::EventStatusChanged : wrong event\n");
    return kTTErrGeneric;
}

TTErr TTTimeCondition::setReady(TTBoolean newReady)
{
    // filter repetitions
    if (newReady != mReady) {
        
        // set the ready value
        mReady = newReady;
        
        // notify each observers
        sendNotification(kTTSym_ConditionReadyChanged, mReady);
    }
    
    return kTTErrGeneric;
}

void TTTimeCondition::deleteReceivers()
{
  TTValue          v, keys;
  TTSymbol         key;
  TTObjectBasePtr  aReceiver;

  mReceivers.getKeys(keys); // CB TODO : if possible, more efficient to run through the table than through the keys
  for (TTUInt8 i = 0; i < keys.size(); i++) {

      key = keys[i];
      mReceivers.lookup(key, v);

      aReceiver = v[0];
      TTObjectBaseRelease(&aReceiver);
  }
    
  mReceivers.clear();
}

void TTTimeCondition::addReceiver(TTAddress anAddress)
{
    TTObjectBasePtr aReceiver;
    TTObjectBasePtr aReceiverCallback;
    TTValuePtr      aReceiverBaton;
    TTValue         v, none;
    
    // if there is no receiver for the expression address
    if (anAddress != kTTAdrsEmpty && mReceivers.lookup(anAddress, v)) {
        
        // No callback for the address
        v = TTValue((TTObjectBasePtr)NULL);
        
        // Create a receiver callback to get the expression address value back
        aReceiverCallback = NULL;
        TTObjectBaseInstantiate(TTSymbol("callback"), &aReceiverCallback, none);
        
        aReceiverBaton = new TTValue(TTObjectBasePtr(this));
        aReceiverBaton->append(anAddress);
        
        aReceiverCallback->setAttributeValue(kTTSym_baton, TTPtr(aReceiverBaton));
        aReceiverCallback->setAttributeValue(kTTSym_function, TTPtr(&TTTimeConditionReceiverReturnValueCallback));
        
        v.append(aReceiverCallback);
        
        aReceiver = NULL;
        TTObjectBaseInstantiate(kTTSym_Receiver, TTObjectBaseHandle(&aReceiver), v);
        
        // set the address of the receiver
        aReceiver->setAttributeValue(kTTSym_address, anAddress);
        
        v = TTObjectBasePtr(aReceiver);
        mReceivers.append(anAddress, v);
    }
}

void TTTimeCondition::applyDefaults()
{
  TTValue v;

  for (TTCaseMapIterator it = mCases.begin() ; it != mCases.end() ; it++) {
      it->first->getAttributeValue(kTTSym_status, v);
      if (v[0] != kTTSym_eventDisposed && v[0] != kTTSym_eventHappened) {
          it->first->sendMessage(it->second.dflt?kTTSym_Trigger:kTTSym_Dispose);
      }
  }
}

#if 0
#pragma mark -
#pragma mark Some Methods
#endif

TTErr TTTimeConditionReceiverReturnValueCallback(TTPtr baton, TTValue& data)
{
    TTValuePtr          b;
    TTTimeConditionPtr  aTimeCondition;
    TTAddress           anAddress;
    Expression          triggerExp;
    TTList              timeEventToTrigger;
    TTList              timeEventToDispose;
    TTValue             v;
	
	// unpack baton (condition, address)
	b = (TTValuePtr)baton;
	aTimeCondition = TTTimeConditionPtr(TTObjectBasePtr((*b)[0]));

    // only if the condition is ready
    if (!aTimeCondition->mReady)
        return kTTErrNone;

    anAddress = (*b)[1];

    // if the dispose expression is true
    if (anAddress == aTimeCondition->mDispose.getAddress() && aTimeCondition->mDispose.evaluate(data)) {

        aTimeCondition->setReady(NO);

        // dispose every event
        aTimeCondition->getEvents(v);
        for (TTElementIter it = v.begin() ; it != v.end() ; it++) {
            TTObjectBasePtr(*it)->sendMessage(kTTSym_Dispose);
        }

    // if didn't dispose
    } else {

        // for each event's expressions matching the incoming address
        for (TTCaseMapIterator it = aTimeCondition->mCases.begin(); it != aTimeCondition->mCases.end(); it++) {
        
            triggerExp = it->second.trigger;

            // if the test of the expression passes
            if (anAddress == triggerExp.getAddress() && triggerExp.evaluate(data)) {

                // append the event to the trigger list
                timeEventToTrigger.append(TTObjectBasePtr(it->first));
            } else {

                // append the event to the dispose list
                timeEventToDispose.append(TTObjectBasePtr(it->first));
            }
        }

        // if at least one event is in the trigger list
        if (!timeEventToTrigger.isEmpty()) {

            aTimeCondition->setReady(NO);

            // trigger all events of the trigger list
            for (timeEventToTrigger.begin(); timeEventToTrigger.end(); timeEventToTrigger.next())
              TTObjectBasePtr(timeEventToTrigger.current()[0])->sendMessage(kTTSym_Trigger);

            // dispose all the other events
            for (timeEventToDispose.begin(); timeEventToDispose.end(); timeEventToDispose.next())
              TTObjectBasePtr(timeEventToDispose.current()[0])->sendMessage(kTTSym_Dispose);
        }
    }

    return kTTErrNone;
}

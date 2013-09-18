/** @file
 *
 * @ingroup scoreExtension
 *
 * @brief Automation time process class manage interpolation between the start event state and end event state depending on the scheduler progression
 *
 * @see TimePluginLib, TTTimeProcess
 *
 * @authors Théo de la Hogue & Clément Bossut
 *
 * @copyright Copyright © 2013, Théo de la Hogue & Clément Bossut @n
 * This code is licensed under the terms of the "CeCILL-C" @n
 * http://www.cecill.info
 */

#include "Automation.h"

#define thisTTClass          Automation
#define thisTTClassName      "Automation"
#define thisTTClassTags      "time, process, automation"

extern "C" TT_EXTENSION_EXPORT TTErr TTLoadJamomaExtension_Automation(void)
{
	TTFoundationInit();
	Automation::registerClass();
    Curve::registerClass();
	return kTTErrNone;
}

TIME_PROCESS_CONSTRUCTOR
{
    TIME_PLUGIN_INITIALIZE
    
	TT_ASSERT("Correct number of args to create Automation", arguments.size() == 0);
    
    registerAttribute(TTSymbol("curveAddresses"), kTypeLocalValue, NULL, (TTGetterMethod)& Automation::getCurveAddresses);
    
    addMessageWithArguments(CurveAdd);
    addMessageWithArguments(CurveGet);
    addMessageWithArguments(CurveUpdate);
    addMessageWithArguments(CurveRemove);
    addMessage(Clear);
}

Automation::~Automation()
{
    Clear();
}

TTErr Automation::getParameterNames(TTValue& value)
{
    value.clear();
	//value.append(TTSymbol("aParameterName"));
	
	return kTTErrNone;
}

TTErr Automation::getCurveAddresses(TTValue& value)
{
    return mCurves.getKeys(value);
}

TTErr Automation::ProcessStart()
{
    TTObjectBasePtr state;
    TTValue         v;
    
    // théo : this a temporary part only here because we use TTScriptInterpolate (see in Automation::Process)
    
    // Flatten the start state to allow interpoaltion
    getStartEvent()->getAttributeValue(TTSymbol("state"), v);
    state = v[0];
    
    state->sendMessage(TTSymbol("Flatten"));
    
    // Flatten the end state to allow interpoaltion
    getEndEvent()->getAttributeValue(TTSymbol("state"), v);
    state = v[0];
    
    state->sendMessage(TTSymbol("Flatten"));
    
    return kTTErrNone;
}

TTErr Automation::ProcessEnd()
{
    // do nothing
    return kTTErrNone;
}

TTErr Automation::Process(const TTValue& inputValue, TTValue& outputValue)
{
    TTObjectBasePtr startState, endState;
    TTFloat64       progression;
    TTValue         v;
    
    if (inputValue.size() == 1) {
        
        if (inputValue[0].type() == kTypeFloat64) {
            
            progression = inputValue[0];
            
            getStartEvent()->getAttributeValue(TTSymbol("state"), v);
            startState = v[0];
            
            getEndEvent()->getAttributeValue(TTSymbol("state"), v);
            endState = v[0];
            
            // théo : this a temporary solution
            // process the interpolation between the start state and the end state
            return TTScriptInterpolate(TTScriptPtr(startState), TTScriptPtr(endState), progression);
        }
    }
    
    return kTTErrGeneric;
}

TTErr Automation::WriteAsXml(const TTValue& inputValue, TTValue& outputValue)
{
	TTXmlHandlerPtr	aXmlHandler = NULL;
    TTValue         v, keys;
    TTSymbol        name, key;
    TTUInt32        i;
	
	aXmlHandler = TTXmlHandlerPtr((TTObjectBasePtr)inputValue[0]);
    
    // Write the curves
    mCurves.getKeys(keys);
    for (i = 0; i < keys.size(); i++) {
        
        key = keys[i];
        mCurves.lookup(key, v);
        
        xmlTextWriterStartElement((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "curve");
        
        // Write the curve's address
        xmlTextWriterWriteAttribute((xmlTextWriterPtr)aXmlHandler->mWriter, BAD_CAST "address", BAD_CAST key.c_str());
        
        aXmlHandler->setAttributeValue(kTTSym_object, v);
        aXmlHandler->sendMessage(TTSymbol("Write"));
        
        xmlTextWriterEndElement((xmlTextWriterPtr)aXmlHandler->mWriter);
    }
    	
	return kTTErrNone;
}

TTErr Automation::ReadFromXml(const TTValue& inputValue, TTValue& outputValue)
{
	TTXmlHandlerPtr	aXmlHandler = NULL;
    TTObjectBasePtr curve = NULL;
    TTSymbol        address;
    TTValue         v;
    
	aXmlHandler = TTXmlHandlerPtr((TTObjectBasePtr)inputValue[0]);
    
    // Event node
    if (aXmlHandler->mXmlNodeName == TTSymbol("Automation")) {
        
        if (aXmlHandler->mXmlNodeStart)
            
            ; // TODO : should we clear the automation here ?
        
        return kTTErrNone;
    }
    
    // If there is a current curve
    if (aXmlHandler->mXmlNodeName == TTSymbol("curve")) {
        
        if (aXmlHandler->mXmlNodeStart) {
            
            TTObjectBaseInstantiate(TTSymbol("Curve"), TTObjectBaseHandle(&curve), kTTValNONE);
            
            // Get the address
            if (!aXmlHandler->getXmlAttribute(TTSymbol("address"), v, YES)) {
                
                if (v.size() == 1) {
                    
                    if (v[0].type() == kTypeSymbol) {
                        
                        address = v[0];
                        
                        // store the curve
                        mCurves.append(address, curve);
                    }
                }
            }
            
            // Pass the xml handler to the current curve to fill his data structure
            v = TTObjectBasePtr(curve);
            aXmlHandler->setAttributeValue(kTTSym_object, v);
            return aXmlHandler->sendMessage(TTSymbol("Read"));
        }
    }
	
	return kTTErrGeneric;
}

TTErr Automation::WriteAsText(const TTValue& inputValue, TTValue& outputValue)
{
	TTTextHandlerPtr	aTextHandler;
	
	aTextHandler = TTTextHandlerPtr((TTObjectBasePtr)inputValue[0]);
	
	// TODO : write the curves
	
	return kTTErrGeneric;
}

TTErr Automation::ReadFromText(const TTValue& inputValue, TTValue& outputValue)
{
	TTTextHandlerPtr aTextHandler;
	TTValue	v;
	
	aTextHandler = TTTextHandlerPtr((TTObjectBasePtr)inputValue[0]);
	
    // TODO : parse the curves
	
	return kTTErrGeneric;
}

TTErr Automation::CurveAdd(const TTValue& inputValue, TTValue& outputValue)
{
    TTValue         v, vStart, vEnd, parameters;
    TTAddress       address;
    TTObjectBasePtr curve = NULL;
    TTErr           err;
    
    if (inputValue.size() == 1) {
        
        if (inputValue[0].type() == kTypeSymbol) {
            
            address = inputValue[0];
            
            // if there is no curve at the address
            if (mCurves.lookup(address, v)) {
                
                // create a freehand function unit
                err = TTObjectBaseInstantiate(TTSymbol("Curve"), TTObjectBaseHandle(&curve), kTTValNONE);
                
                if (!err) {
                    
                    // get the start event state value for this address
                    getStartEvent()->sendMessage(TTSymbol("StateAddressGetValue"), address, vStart);

                    // get the end event state value for this address
                    getEndEvent()->sendMessage(TTSymbol("StateAddressGetValue"), address, vEnd);
                    
                    // prepare curve parameters
                    parameters.resize(6);
                    parameters[0] = TTFloat64(0.);
                    parameters[1] = TTFloat64(vStart[0]);
                    parameters[2] = TTFloat64(1);
                    
                    parameters[3] = TTFloat64(1.);
                    parameters[4] = TTFloat64(vEnd[0]);
                    parameters[5] = TTFloat64(1);
                    
                    curve->setAttributeValue(TTSymbol("parameters"), parameters);

                    // register the curve at address
                    return mCurves.append(address, curve);
                }
            }
        }
    }
    
    return kTTErrGeneric;
}

TTErr Automation::CurveGet(const TTValue& inputValue, TTValue& outputValue)
{
    TTAddress  address;

    if (inputValue.size() == 1) {
        
        if (inputValue[0].type() == kTypeSymbol) {
            
            address = inputValue[0];
            
            return mCurves.lookup(address, outputValue);
        }
    }
    
    return kTTErrGeneric;
}

TTErr Automation::CurveUpdate(const TTValue& inputValue, TTValue& outputValue)
{
    TTValue         v, vStart, vEnd, parameters;
    TTAddress       address;
    TTObjectBasePtr curve;
    
    // update all curves
    if (inputValue.size() == 0) {
        
        TTValue         keys;
        TTSymbol        key;
        TTUInt32        i;
        
        // update all curves
        mCurves.getKeys(keys);
        for (i = 0; i < keys.size(); i++) {
            
            key = keys[i];
            CurveUpdate(key, kTTValNONE);
        }
        
        return kTTErrNone;
    }
    
    if (inputValue.size() == 1) {
        
        if (inputValue[0].type() == kTypeSymbol) {
            
            address = inputValue[0];
            
            // if there is a curve at the address
            if (!mCurves.lookup(address, v)) {
                
                curve = v[0];
                
                // get the start event state value for this address
                getStartEvent()->sendMessage(TTSymbol("StateAddressGetValue"), address, vStart);
                
                // get the end event state value for this address
                getEndEvent()->sendMessage(TTSymbol("StateAddressGetValue"), address, vEnd);
                
                // get current curve parameters
                curve->getAttributeValue(TTSymbol("parameters"), parameters);
                
                // change the first point y value : x1 y1 b1 ...
                parameters[1] = TTFloat64(vStart[0]);
                
                // TODO : scale the other y points value ?
                
                // change the last point y value : ... xn yn bn
                parameters[parameters.size() - 3] = TTFloat64(vStart[0]);
                
                // set current curve parameters
                curve->setAttributeValue(TTSymbol("parameters"), parameters);
                
                return kTTErrNone;
            }
        }
    }
    
    return kTTErrGeneric;
}

TTErr Automation::CurveRemove(const TTValue& inputValue, TTValue& outputValue)
{
    TTValue         v;
    TTAddress       address;
    TTObjectBasePtr curve;
    
    if (inputValue.size() == 1) {
        
        if (inputValue[0].type() == kTypeSymbol) {
            
            address = inputValue[0];
            
            // if there is a curve at the address
            if (!mCurves.lookup(address, v)) {
                
                curve = v[0];
                
                // unregister the curve
                mCurves.remove(address);
                
                // delete curve
                return TTObjectBaseRelease(&curve);
            }
        }
    }
    
    return kTTErrGeneric;
}

TTErr Automation::Clear()
{
    TTValue         keys,v;
    TTSymbol        key;
    TTUInt32        i;
    TTObjectBasePtr curve;
    
    // delete all curves
    mCurves.getKeys(keys);
    for (i = 0; i < keys.size(); i++) {
        
        key = keys[i];
        mCurves.lookup(key, v);
        
        curve = v[0];
        
        TTObjectBaseRelease(&curve);
    }
    
    return kTTErrNone;
}

#if 0
#pragma mark -
#pragma mark Some Methods
#endif

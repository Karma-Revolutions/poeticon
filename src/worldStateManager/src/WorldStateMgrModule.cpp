/*
 * Copyright: (C) 2012-2015 POETICON++, European Commission FP7 project ICT-288382
 * Copyright: (C) 2014 VisLab, Institute for Systems and Robotics,
 *                Istituto Superior Técnico, Universidade de Lisboa, Lisbon, Portugal
 * Author: Giovanni Saponaro <gsaponaro@isr.ist.utl.pt>
 * CopyPolicy: Released under the terms of the GNU GPL v2.0
 *
 */

#include "WorldStateMgrModule.h"

bool WorldStateMgrModule::configure(ResourceFinder &rf)
{
    moduleName = rf.check("name", Value("wsm")).asString();
    setName(moduleName.c_str());

    inTargetsPortName = "/" + moduleName + "/target:i";
    inTargetsPort.open(inTargetsPortName.c_str());

    inAffPortName = "/" + moduleName + "/affDescriptor:i";
    inAffPort.open(inAffPortName.c_str());

    outFixationPortName = "/" + moduleName + "/fixation:o";
    outFixationPort.open(outFixationPortName.c_str());

    //outStatePortName = "/" + moduleName + "/state:o";
    //outStatePort.open(outStatePortName.c_str());
    
    opcPortName = "/" + moduleName + "/opc:io";
    opcPort.open(opcPortName.c_str());

    inAff = NULL;
    inTargets = NULL;
    state = STATE_WAIT_BLOBS;

    return true;
}

bool WorldStateMgrModule::interruptModule()
{
    inTargetsPort.interrupt();
    inAffPort.interrupt();
    outFixationPort.interrupt();
    //outStatePort.interrupt();
    opcPort.interrupt();

    return true;
}

bool WorldStateMgrModule::close()
{
    inTargetsPort.close();
    inAffPort.close();
    outFixationPort.close();
    //outStatePort.close();
    opcPort.close();

    return true;
}

bool WorldStateMgrModule::updateModule()
{
    switch(state)
    {
        case STATE_WAIT_BLOBS:
        {
            // wait for blobs data to arrive
            updateBlobs();

            // when something arrives, proceed
            if (inAff != NULL)
                state = STATE_READ_BLOBS;

            break;
        }

        case STATE_READ_BLOBS:
        {
            // if size>0 proceed, else go back one state
            if (sizeAff > 0)
                state = STATE_INIT_TRACKER;
            else
                state = STATE_WAIT_BLOBS;

            break;
        }

        case STATE_INIT_TRACKER:
        {
            doInitTracker();

            // proceed
            state = STATE_WAIT_TRACKER;

            break;
        }

        case STATE_WAIT_TRACKER:
        {
            // wait for tracker data to arrive
            updateTracker();

            // when something arrives, proceed
            if (inTargets != NULL)
                state = STATE_READ_TRACKER;

            break;
        }

        case STATE_READ_TRACKER:
        {
            // if size>0 proceed, else go back one state
            if (sizeTargets > 0)
                state = STATE_POPULATE_DB;
            else
                state = STATE_WAIT_TRACKER;

            break;
        }

        case STATE_POPULATE_DB:
        {
            // read new data, ensure validity
            updateBlobs();
            updateTracker();
            if (inAff==NULL || inTargets==NULL)
            {
                yWarning("no data");
                return false;
            }
            if (sizeAff != sizeTargets)
            {
                //yWarning("sizeAff=%d differs from sizeTargets=%d", sizeAff, sizeTargets);
                return false;
            }
            
            // prepare position property
            int a = 0; // TODO: cycle
            Bottle bPos;
            Bottle &pos_list = bPos.addList();
            pos_list.addString("pos");
            Bottle &pos_list_c = pos_list.addList();
            //pos_list_c.addDouble(inAff->get(a+1).asList()->get(0).asDouble()); // from blobs
            //pos_list_c.addDouble(inAff->get(a+1).asList()->get(1).asDouble());
            pos_list_c.addDouble(inTargets->get(a).asList()->get(1).asDouble()); // from tracker
            pos_list_c.addDouble(inTargets->get(a).asList()->get(2).asDouble());

            // prepare name property
            Bottle bName;
            Bottle &name_list = bName.addList();
            name_list.addString("name");
            name_list.addString("myObject"); // TODO: real name
           
            // prepare shape descriptors property
            Bottle bDesc;
            Bottle &desc_list = bDesc.addList();
            desc_list.addString("desc");
            Bottle &desc_list_c = desc_list.addList();
            desc_list_c.addDouble(inAff->get(a+1).asList()->get(23).asDouble()); // area
            desc_list_c.addDouble(inAff->get(a+1).asList()->get(24).asDouble());
            desc_list_c.addDouble(inAff->get(a+1).asList()->get(25).asDouble());
            desc_list_c.addDouble(inAff->get(a+1).asList()->get(26).asDouble());           
            desc_list_c.addDouble(inAff->get(a+1).asList()->get(27).asDouble());
            desc_list_c.addDouble(inAff->get(a+1).asList()->get(28).asDouble());

            // prepare is_hand property (true/false)
            bool isHandFlag = false; // TODO: real value
            Bottle bIsHand;
            Bottle &is_hand_list = bIsHand.addList();
            is_hand_list.addString("is_h");
            is_hand_list.addInt(isHandFlag); // 1=true, 0=false

            // prepare in_hand property (none/left/right)
            Bottle bInHand;
            Bottle &in_hand_list = bInHand.addList();
            in_hand_list.addString("in_h");
            in_hand_list.addVocab(Vocab::encode("none")); // TODO: real value

            // prepare on_top_of property
            Bottle bOnTopOf;
            Bottle &oto_list = bOnTopOf.addList();
            oto_list.addString("on_t");
            Bottle &oto_list_c = oto_list.addList();
            oto_list_c.addInt(0); // TODO: real list

            // prepare reachable_with property
            Bottle bReachW;
            Bottle &reaw_list = bReachW.addList();
            reaw_list.addString("re_w");
            Bottle &reaw_list_c = reaw_list.addList();
            reaw_list_c.addInt(0); // TODO: real list

            // prepare pullable_with property
            Bottle bPullW;
            Bottle &pullw_list = bPullW.addList();
            pullw_list.addString("pu_w");
            Bottle &pullw_list_c = pullw_list.addList();
            pullw_list_c.addInt(0); // TODO: real list

            // populate
            Bottle opcCmd, opcReply;
            opcCmd.addVocab(Vocab::encode("add"));
            opcCmd.addList() = bPos;
            opcCmd.addList() = bName;
            opcCmd.addList() = bDesc;
            opcCmd.addList() = bIsHand;
            opcCmd.addList() = bInHand;
            opcCmd.addList() = bOnTopOf;
            opcCmd.addList() = bReachW;
            opcCmd.addList() = bPullW;
            yDebug("Bottle opcCmd is: %s", opcCmd.toString().c_str());
            
            // keep database updated with current perception data
            
            

            break;
        }
    }

    return true;
}

double WorldStateMgrModule::getPeriod()
{
    return 0.0;
}

void WorldStateMgrModule::doInitTracker()
{
    yInfo("initializing multi-object tracking of %d objects:", sizeAff);

    Bottle fixation;
    double x=0.0, y=0.0;

    for(int a=0; a<sizeAff; a++)
    {
        x = inAff->get(a+1).asList()->get(0).asDouble();
        y = inAff->get(a+1).asList()->get(1).asDouble();

        fixation.clear();
        fixation.addDouble(x);
        fixation.addDouble(y);
		Time::delay(1.0); // fixes activeParticleTrack crash
        outFixationPort.write(fixation);

        yInfo("id %d: %f %f", a, x, y);
    }

    yInfo("done initializing tracker");
}

/*
void ShortTermMemModule::doWriteState()
{
    // read new data, ensure validity
    updateBlobs();
    updateTracker();
    if (inAff==NULL || inTargets==NULL)
    {
        yWarning("no data");
        return;
    }
    if (sizeAff != sizeTargets)
    {
        //yWarning("sizeAff=%d differs from sizeTargets=%d", sizeAff, sizeTargets);
        return;
    }

    Bottle &state = outStatePort.prepare();
    state.clear();
    for(int a=0; a<sizeAff; a++)
    {
        Bottle &obj = state.addList();
        obj.clear();

        // add ID of object from tracker
        obj.addInt(inTargets->get(a).asList()->get(0).asInt());

        // add other fields from tracker
        //obj.addDouble(inTargets->get(a).asList()->get(1).asDouble());
        //obj.addDouble(inTargets->get(a).asList()->get(2).asDouble());

        // add other fields from blobs
        obj.addDouble(inAff->get(a+1).asList()->get(0).asDouble());
        obj.addDouble(inAff->get(a+1).asList()->get(1).asDouble());
    }
    outStatePort.write();

    //yInfo("sent state of %d objects", sizeAff);
}
*/

void WorldStateMgrModule::updateBlobs()
{
    inAff = inAffPort.read();

    if (inAff != NULL)
    {
        // number of blobs
        sizeAff = static_cast<int>( inAff->get(0).asDouble() );
    }
}

void WorldStateMgrModule::updateTracker()
{
    inTargets = inTargetsPort.read();

    if (inTargets != NULL)
    {
        // number of tracked objects
        sizeTargets = inTargets->size();
    }
}

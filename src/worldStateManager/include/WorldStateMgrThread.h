/*
 * Copyright: (C) 2012-2015 POETICON++, European Commission FP7 project ICT-288382
 * Copyright: (C) 2014 VisLab, Institute for Systems and Robotics,
 *                Istituto Superior Técnico, Universidade de Lisboa, Lisbon, Portugal
 * Author: Giovanni Saponaro <gsaponaro@isr.ist.utl.pt>
 * CopyPolicy: Released under the terms of the GNU GPL v2.0
 *
 */

#ifndef __WSM_THREAD_H__
#define __WSM_THREAD_H__

#include <iomanip>
#include <sstream>
#include <yarp/os/Bottle.h>
#include <yarp/os/BufferedPort.h>
#include <yarp/os/Log.h>
#include <yarp/os/LogStream.h>
#include <yarp/os/Port.h>
#include <yarp/os/Property.h>
#include <yarp/os/RateThread.h>
#include <yarp/os/RpcClient.h>
#include <yarp/os/Time.h>
#include <yarp/os/Vocab.h>

// perception states
#define STATE_WAIT_BLOBS   0
#define STATE_READ_BLOBS   1
#define STATE_INIT_TRACKER 2
#define STATE_WAIT_TRACKER 3
#define STATE_READ_TRACKER 4
#define STATE_POPULATE_DB  5
#define STATE_UPDATE_DB    6

// playback states
#define STATE_PARSE_FILE 100
#define STATE_STEP_FILE  101
#define STATE_END_FILE   102

using namespace std;
using namespace yarp::os;

class WorldStateMgrThread : public RateThread
{
    private:
        string moduleName;
        string inTargetsPortName;
        string inAffPortName;
        string outFixationPortName;
        string opcPortName;
        string arePortName;
        BufferedPort<Bottle> inTargetsPort;
        BufferedPort<Bottle> inAffPort;
        Port outFixationPort;
        RpcClient opcPort;
        RpcClient arePort;
        bool closing;

        // perception and playback modes
        bool playbackMode;
        bool populated;

        // perception mode
        int perceptionState;
        Bottle *inAff;
        Bottle *inTargets;
        int sizeTargets, sizeAff;

        // playback mode
        int playbackState; // TODO: rename to avoid confusion with file "state##"
        string playbackFile;
        bool playbackPaused;
        Bottle findBottle;
        int sizePlaybackFile;
        int currPlayback;

    public:
        WorldStateMgrThread(const string &_moduleName,
                            const double _period,
                            bool _playbackMode=false);
        bool openPorts();
        void close();
        void interrupt();
        bool threadInit();
        void run();
        
        // perception and playback modes
        bool updateWorldState();
        bool doPopulateDB();
        
        // perception mode
        bool initPerceptionVars();
        bool initTracker();
        void fsmPerception();
        void refreshBlobs();
        void refreshTracker();
        void refreshPerception();
        bool refreshPerceptionAndValidate();
        bool isHandFree(const string &handName);

        // playback mode
        bool initPlaybackVars();
        bool setPlaybackFile(const string &_file);
        void fsmPlayback();
};

#endif
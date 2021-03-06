/*
 * Copyright: (C) 2012-2015 POETICON++, European Commission FP7 project ICT-288382
 * Copyright: (C) 2014 VisLab, Institute for Systems and Robotics,
 *                Instituto Superior Técnico, Universidade de Lisboa, Lisbon, Portugal
 * Author: Giovanni Saponaro <gsaponaro@isr.ist.utl.pt>
 * CopyPolicy: Released under the terms of the GNU GPL v2.0
 *
 */

#ifndef __WSM_THREAD_H__
#define __WSM_THREAD_H__

#include <iomanip>
#include <iostream> // __func__
#include <map>
#include <string>
#include <vector>

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
#include <yarp/sig/Vector.h>
#include <iCub/ctrl/filters.h>

#include "Helpers.h"

// perception mode states
#define STATE_PERCEPTION_WAIT_OPC         0
#define STATE_PERCEPTION_WAIT_BLOBS       1
#define STATE_PERCEPTION_READ_BLOBS       2
#define STATE_PERCEPTION_WAIT_TRACKER     3
#define STATE_PERCEPTION_INIT_TRACKER     4
#define STATE_PERCEPTION_READ_TRACKER     5
#define STATE_PERCEPTION_WAIT_ACTIVITYIF  6
#define STATE_PERCEPTION_POPULATE_DB      7
#define STATE_PERCEPTION_WAIT_CMD         8
#define STATE_PERCEPTION_UPDATE_DB        9

// playback mode states
#define STATE_DUMMY_PARSE    100
#define STATE_DUMMY_WAIT_OPC 101
#define STATE_DUMMY_WAIT_CMD 102
#define STATE_DUMMY_STEP     103
#define STATE_DUMMY_EOF      104
#define STATE_DUMMY_ERROR    105

// make sure __func__ is set correctly, http://stackoverflow.com/a/17528983
#if __STDC_VERSION__ < 199901L
# if __GNUC__ >= 2
#  define __func__ __FUNCTION__
# else
#  define __func__ "<unknown>"
# endif
#endif

using namespace std;
using namespace yarp::os;

typedef std::map<int,string> idLabelMap;

class WorldStateMgrThread : public RateThread
{
    private:
        string moduleName;
        string opcPortName;
        string inTargetsPortName;
        string inAffPortName;
        string inToolAffPortName;
        string activityPortName;
        string trackerPortName;

        RpcClient opcPort;
        BufferedPort<Bottle> inTargetsPort;
        BufferedPort<Bottle> inAffPort;
        BufferedPort<Bottle> inToolAffPort;
        RpcClient activityPort;
        RpcClient trackerPort;

        bool closing;

        // perception and playback modes
        bool playbackMode;
        int fsmState;
        bool toldUserConnectOPC;
        bool toldUserOPCConnected;
        int countFrom;
        bool withFilter;
        int filterOrder;

        // perception mode
        bool needUpdate;
        bool trackerInit;
        bool toldUserWaitBlobs;
        bool toldUserBlobsConnected;
        bool toldUserWaitTracker;
        bool toldUserTrackerConnected;
        bool toldUserWaitActivityIF;
        bool toldActivityGoHome;
        bool toldUserActivityIFConnected;
        Bottle *inAff;
        Bottle *inToolAff;
        Bottle *inTargets;
        int sizeTargets, sizeAff;
        std::vector<int> opcIDs;
        std::vector<int> trackIDs;
        idLabelMap opcMap;
        idLabelMap trackMap;
        idLabelMap wsMap;
        std::vector<iCub::ctrl::MedianFilter> posFilter;

        // playback mode
        string playbackFile;
        bool playbackPaused;
        bool toldUserEof;
        Bottle stateBottle;
        int sizePlaybackFile;
        int currPlayback;

    public:
        WorldStateMgrThread(const string &_moduleName,
                            const double _period,
                            const bool _playbackMode,
                            const int _countFrom,
                            const bool _withFilter);
        bool openPorts();
        void close();
        void interrupt();
        bool threadInit();
        void run();

        // perception and playback modes
        bool initCommonVars();
        bool dumpWorldState();
        bool updateWorldState();

        // perception mode
        bool initPerceptionVars();
        bool initTracker();
        void fsmPerception();
        void refreshOPC();
        void refreshOPCIDs();
        void refreshOPCNames();
        void refreshTrackNames();
        void refreshBlobs();
        void refreshTracker();
        void updateTrackIDsNoDupes();
        void refreshPerception();
        bool refreshPerceptionAndValidate();
        bool doPopulateDB();
        bool getTrackerBottleIndexFromID(const int &id, int &tbi);
        bool getAffBottleIndexFromTrackROI(const int &u, const int &v, int &abi);
        int label2id(const string &label);
        bool getLabel(const int &u, const int &v, string &label);
        bool getLabelMajorityVote(const int &u, const int &v, string &winnerLabel, const int &rounds=5);
        bool mono2stereo(const string &objName, double &x, double &y, double &z);
        bool getTooltipOffset(const string &objName, Bottle &offset);
        bool isOnTopOf(const string &objName, Bottle &objBelow);
        bool isReachableWith(const string &objName, Bottle &objReachable);
        bool isPullableWith(const string &objName, Bottle &objPullable);
        bool isHandFree(const string &handName);
        string inWhichHand(const string &objName);
        bool setFilterOrder(const int &n);

        // IDL functions
        bool pauseTrack(const string &objName);
        bool resumeTrack(const string &objName);
        Bottle getColorHistogram(const int32_t &u, const int32_t &v);

        // playback mode
        bool initPlaybackVars();
        void setPlaybackFile(const string &_file);
        void fsmPlayback();
};

#endif

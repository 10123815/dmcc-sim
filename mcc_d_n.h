#ifndef _MCC_D_N_H_
#define _MCC_D_N_H_

// op header
#include <opnet.h>

// cpp header
#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <fstream>
#include <algorithm>
#include <random>
#include <cmath>
#include <memory>
#include <functional>

namespace mcc
{

	static std::random_device rd;
	static std::mt19937 gen(rd());

	// interface关键字声明
	#define interface class __declspec(novtable)

	enum NodeType
	{
		kSuperNode	= 4,
		kFixedNode  = 2,
		kMobileNode = 1
	};

	const unsigned short g_kAppTypeCount = 2;

	enum AppType
	{
		kApp1 = 0,
		kApp2 = 1
	};

	const float g_kDeg2Rad = 3.1415926f / 180.0f;
	const float g_kRad2Deg = 180.0f / 3.1415926f;

	const float g_kMaxUsage = 0.5f;

	const float g_kCpuUsageWeight = 0.333f;
	const float g_kMemUsageWeight = 0.333f;
	const float g_kBwUsageWeight  = 0.333f;

	const float g_kBandWidth = 200.0f * 1024 * 1024;

	// 边界
	const float g_kXMin = -00.0f;
	const float g_kXMax = 100.0f;
	const float g_kYMin = -100.0f;
	const float g_kYMax = 100.0f;

	// 栅格大小
	const float g_kSquareSize = 20.0f;

	// ici code define
	const int g_kTimeCome		 = 0;
	const int g_kBroadCastTick   = 1;
	const int g_kDiscoverOther	 = 2;

	const int g_kMove			 = 100;

	const int g_kLocalNodeData	 = 200;
	
	const int g_kComLocalExe     = 1000;
	const int g_kComRemoteExe    = 1001;
	const int g_kComLocalExeEnd  = 1002;
	const int g_kComRemoteExeEnd = 1003;
	const int g_kOffloadInfo	 = 1004;
	const int g_kBroadCast		 = 1100;

	const int g_kCapacity		 = 2000;
	const int g_kCapacityReq 	 = 2001;

	const int g_kMergeRequest	 = 3000;
	const int g_kMergeResponse	 = 3001;
	const int g_kMerge 		 	 = 3002;

	const int g_kDeployment		 = 4000;

	const int g_kCheckState		 = 5000;
	const int g_kCloudletCheck	 = 5001;	// cloudlet检查component运行状态
	const int g_kCloudletRemove  = 5002;	// node告诉cloudlet，可以移除我了
	const int g_kClientRemoved	 = 5003;	// cloudlet告诉node，你可以走了

	const int g_kRestart		 = 6000;

	const int g_kChangeMobility  = 7000;

	const int g_kTriggerSelf	 = 10000;

	// 广播包监听端口
	const int g_kDiscoverPkgPort = 9999;

	// 距离
	const float g_kMaxConnectDis = 1000.0f;
	// 最小链接SNR
	const float g_kMinConnectSNR = -100.0f;

	const float g_kHandoffTime	 = 0.01f;

	enum MobilityModel
	{
		kMobility_RWP	= 10000,
		kMobility_RPG	= 10001,
	};

	const float g_kInitTemperature 		= 100.0f;		// SA的初始温度 
	const float g_kTemperatureDecrease 	= 0.01f;		// 温度减量	      
}


#endif
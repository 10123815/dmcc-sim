#include "mcc_node.h"

using namespace mcc;

// friend
bool Compare (const NodeData &data1, const NodeData &data2)
{
	return data1.Capacity() < data2.Capacity();
}

// public
float NodeData::Capacity ( ) const
{
	float bit_Gbit = 1024.0f * 1024.0f * 1024.0f;
	return cores_ * speed_ * g_kCpuUsageWeight + 
			mem_ * g_kMemUsageWeight / bit_Gbit +		// Gbits
			bandwidth_ * g_kBwUsageWeight / bit_Gbit;	// Gbits
}

// private
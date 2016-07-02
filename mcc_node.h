#ifndef _MCC_NODE_H_
#define _MCC_NODE_H_ 

#include "mcc_d_n.h"
#include "mcc_sim_mng.h"

namespace mcc
{

	class  NodeData
	{
	public:
		NodeData ( ) = default;

		void set_data (NodeType node_type)
		{
			int type = static_cast<int>(node_type);

			// 1核~4核
			// std::uniform_int_distribution<> u(1, 4);
			cores_ = type;

			speed_ = type;

			mem_ = 1024.0f * 1024.0f * type;

			bandwidth_ = 1024.0f * 1024.0f * 200.0f;

			rep_ = 0;
		}

		unsigned short cores ( ) const { return cores_; }
		float speed ( ) const { return speed_; }
		float mem ( ) const { return mem_; }
		float bandwidth ( ) const { return bandwidth_; }

		float rep ( ) const { return rep_; }
		void add_rep (float val) { rep_ += val; }
		void minus_rep ( float val) { rep_ -= val; }

		float usage ( ) const { return usage_; }
		void set_usage (float val) { usage_ = val; }

		float Capacity ( ) const ;
		friend bool Compare (const NodeData &data1, const NodeData &data2);

	private:
		// read only
		unsigned short cores_;	// 个
		float speed_;			// load/ms
		float mem_;				// bits
		float bandwidth_;		// bits

		float rep_;
		float usage_;
	};

	class Node
	{

	public:
		// 设、取值
		unsigned short node_id ( ) const { return node_id_; }
		unsigned short cloudlet_id ( ) const { return cloudlet_id_; }
		void set_cloudlet_id (unsigned short v) { cloudlet_id_ = v; }	// 宿主转移时

		// 计算使用率，每当节点上增加（减少）组件时都要重新计算
		float Usage ( );

	protected:
		void set_node_id (unsigned short v) { node_id_ = v; };	// 由id分配函数调用
		NodeType node_type_ = kFixedNode;						// 结点类型

	private:
		unsigned short node_id_;				// 系统分配的ID
		unsigned short cloudlet_id_;		 	// 所在CLoudlet的宿主ID

	};
}

#endif
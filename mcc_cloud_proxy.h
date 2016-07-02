#ifndef _MMC_CLOUD_PROXY_H_
#define _MMC_CLOUD_PROXY_H_ 

#include "mcc_d_n.h"
#include "mcc_app.h"
#include "mcc_timer.h"

namespace mcc
{

	// 一个app的组件分配
	class Deployment
	{
	public:
		Deployment (Objid init_node_id, int app_index, int length) : 
		init_node_id_ (init_node_id),
		app_index_	  (app_index)
		{
			for (int i = 0; i < length; ++i)
			{
				// 初始化全部offload到本地结点上
				offload_nodes_id_.push_back(init_node_id);
			}
		}

		int app_index ( ) const { return app_index_; }
		std::vector<Objid>& offload_nodes ( ) { return offload_nodes_id_; }
		std::vector<Objid> coffload_nodes ( ) const { return offload_nodes_id_; }
		int size ( ) const { return offload_nodes_id_.size(); }

	private:
		Objid init_node_id_;
		int app_index_;
		std::vector<Objid> offload_nodes_id_;
	};

	class CloudletProxy
	{

	public:

		// 取、设值
		Objid cloudlet_id ( ) const { return cloudlet_id_; }
		void set_cloudlet_id (Objid id) { cloudlet_id_ = id; }
		void set_cloudlet_proxy_id (Objid id) { cloudlet_proxy_id_ = id; }

		// 发现机制是否开启
		bool is_discover_active ( ) const { return discover_avtive_; }

		// cloudlet能力，只在添、删节点时才会改变
		float capacity ( ) const { return capacity_; }

		int size ( ) const { return nodes_.size(); }

		// cloudlet使用率, 实时
		float usage ( ) const
		{
			if (nodes_.size() == 0)
				return 0;

			float usage = 0;
			for (auto pair : nodes_)
			{
				usage += pair.second->usage();
			}
			usage = usage / nodes_.size();
			usage = max(0, usage);
			usage = min(1, usage);
			return usage;
		}

		// cloudlet中加入一个节点
		void AddNodeData (Objid id, std::shared_ptr<NodeData> p_node_data)
		{
			nodes_.insert({id, p_node_data});
			capacity_ += p_node_data->Capacity(); 
		}

		void RemoveNodeData (Objid id)
		{
			if (id == cloudlet_id_)
				return;

			if (nodes_.find(id) != nodes_.end())
			{
				capacity_ -= nodes_[id]->Capacity();
				nodes_.erase(id);
			}

			if (total_com_deployment_.find(id) != total_com_deployment_.end())
				total_com_deployment_.erase(id);
		}

		// 注册组件，只发生在初始化时，同时将本地的分配初始化
		// app编号对应其全部组件
		void RegisterComponent (std::map<int, std::vector<Component>> coms_)
		{
			for (auto ele : coms_)
			{
				coms_regedit_.insert(ele);
				Deployment deployment(cloudlet_id_, ele.first, ele.second.size());
				// 初始只有本地一个节点的com分配
				total_com_deployment_[cloudlet_id_].push_back(deployment);
			}
		}

		// 激活
		void set_active (bool act) { active_ = act; }
		bool active ( ) const { return active_; }

		// 开始广播
		void StartBroadcastDiscoverPkg (unsigned short multi = 10);

		// 广播发现包
		void BroadcastDiscoverPkg ( );

		// 停止广播
		void StopBroadcastDiscoverPkg ( );

		// 开始监听发现包
		void StartListenDiscoverPkg ( );

		// 当收到发现包，即发现别人时
		void OnDiscoverOthers (Ici* p_disc_ici);

		// 停止监听发现包
		void StopListenDiscoverPkg ( );

		// 当得到对方的能力值
		void OnGetCapacityFromOther (Ici* p_ici);

		// 当收到合并请求
		void OnGetMergeRequest (Ici* p_ici);
		void OnGetMergeResponse (Ici* p_ici);

		// 其他CLoudlet合并入
		void Merge(Ici *p_ici);

		// 决策，不改变cloudlet本身
		void Plan ( );

		// 分配
		void Deploy ( ) const ;

		void CheckCloudletState ( );

		void RemoveNode (Ici* p_ici);

	private:

		// 给定一个解，评价usage
		float _CpuUsage (const std::map<Objid, std::vector<Deployment>>& deployments) const;

		// 随机变换一个解，得到另一个解，并得到usage增量
		std::map<Objid, std::vector<Deployment>> _RandomNewSolution (const std::map<Objid, std::vector<Deployment>>& deployments, float* p_delta_usage) const;
		std::map<Objid, std::vector<Deployment>> _RandomNewSolutionBW (const std::map<Objid, std::vector<Deployment>>& deployments, float* p_delta_usage) const;

		// 返回弃子的id
		Objid _DiscardedNode ( ) const;

		// 找到cloudlet中rep最小的20%的结点
		std::map<Objid, NodeData> _FindKMinRepNode (float perc = 0.2) const;

		std::map<Objid, std::shared_ptr<NodeData>> nodes_;
		std::set<Objid> removed_nodes_;
		std::pair<Objid, NodeData> last_removed_node_;
		Objid cloudlet_proxy_id_;		// 进程id
		Objid cloudlet_id_;				// 指向的cLoudlet所在的node的id

		std::map<int, std::vector<Component>> coms_regedit_;	// 对应某个app的组件注册表

		std::map<Objid, std::vector<Deployment>> total_com_deployment_;		// 当前全部的组件的分配，node-apps

		bool active_ = true;			// proxy是否激活，当处于其他CLoudlet下时，关闭
		bool discover_avtive_ = true;	// 是否发现其他节点
		float capacity_;				// cloudlet总能力

		std::shared_ptr<Timer> p_broadcast_timer_;	// 保存添加的定时器
		std::shared_ptr<Timer> p_checkstt_timer_;
  
  	};
}

#endif		
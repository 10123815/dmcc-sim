#ifndef _MMC_CLOUD_PROXY_H_
#define _MMC_CLOUD_PROXY_H_ 

#include "mcc_d_n.h"
#include "mcc_app.h"
#include "mcc_timer.h"

namespace mcc
{

	// һ��app���������
	class Deployment
	{
	public:
		Deployment (Objid init_node_id, int app_index, int length) : 
		init_node_id_ (init_node_id),
		app_index_	  (app_index)
		{
			for (int i = 0; i < length; ++i)
			{
				// ��ʼ��ȫ��offload�����ؽ����
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

		// ȡ����ֵ
		Objid cloudlet_id ( ) const { return cloudlet_id_; }
		void set_cloudlet_id (Objid id) { cloudlet_id_ = id; }
		void set_cloudlet_proxy_id (Objid id) { cloudlet_proxy_id_ = id; }

		// ���ֻ����Ƿ���
		bool is_discover_active ( ) const { return discover_avtive_; }

		// cloudlet������ֻ����ɾ�ڵ�ʱ�Ż�ı�
		float capacity ( ) const { return capacity_; }

		int size ( ) const { return nodes_.size(); }

		// cloudletʹ����, ʵʱ
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

		// cloudlet�м���һ���ڵ�
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

		// ע�������ֻ�����ڳ�ʼ��ʱ��ͬʱ�����صķ����ʼ��
		// app��Ŷ�Ӧ��ȫ�����
		void RegisterComponent (std::map<int, std::vector<Component>> coms_)
		{
			for (auto ele : coms_)
			{
				coms_regedit_.insert(ele);
				Deployment deployment(cloudlet_id_, ele.first, ele.second.size());
				// ��ʼֻ�б���һ���ڵ��com����
				total_com_deployment_[cloudlet_id_].push_back(deployment);
			}
		}

		// ����
		void set_active (bool act) { active_ = act; }
		bool active ( ) const { return active_; }

		// ��ʼ�㲥
		void StartBroadcastDiscoverPkg (unsigned short multi = 10);

		// �㲥���ְ�
		void BroadcastDiscoverPkg ( );

		// ֹͣ�㲥
		void StopBroadcastDiscoverPkg ( );

		// ��ʼ�������ְ�
		void StartListenDiscoverPkg ( );

		// ���յ����ְ��������ֱ���ʱ
		void OnDiscoverOthers (Ici* p_disc_ici);

		// ֹͣ�������ְ�
		void StopListenDiscoverPkg ( );

		// ���õ��Է�������ֵ
		void OnGetCapacityFromOther (Ici* p_ici);

		// ���յ��ϲ�����
		void OnGetMergeRequest (Ici* p_ici);
		void OnGetMergeResponse (Ici* p_ici);

		// ����CLoudlet�ϲ���
		void Merge(Ici *p_ici);

		// ���ߣ����ı�cloudlet����
		void Plan ( );

		// ����
		void Deploy ( ) const ;

		void CheckCloudletState ( );

		void RemoveNode (Ici* p_ici);

	private:

		// ����һ���⣬����usage
		float _CpuUsage (const std::map<Objid, std::vector<Deployment>>& deployments) const;

		// ����任һ���⣬�õ���һ���⣬���õ�usage����
		std::map<Objid, std::vector<Deployment>> _RandomNewSolution (const std::map<Objid, std::vector<Deployment>>& deployments, float* p_delta_usage) const;
		std::map<Objid, std::vector<Deployment>> _RandomNewSolutionBW (const std::map<Objid, std::vector<Deployment>>& deployments, float* p_delta_usage) const;

		// �������ӵ�id
		Objid _DiscardedNode ( ) const;

		// �ҵ�cloudlet��rep��С��20%�Ľ��
		std::map<Objid, NodeData> _FindKMinRepNode (float perc = 0.2) const;

		std::map<Objid, std::shared_ptr<NodeData>> nodes_;
		std::set<Objid> removed_nodes_;
		std::pair<Objid, NodeData> last_removed_node_;
		Objid cloudlet_proxy_id_;		// ����id
		Objid cloudlet_id_;				// ָ���cLoudlet���ڵ�node��id

		std::map<int, std::vector<Component>> coms_regedit_;	// ��Ӧĳ��app�����ע���

		std::map<Objid, std::vector<Deployment>> total_com_deployment_;		// ��ǰȫ��������ķ��䣬node-apps

		bool active_ = true;			// proxy�Ƿ񼤻����������CLoudlet��ʱ���ر�
		bool discover_avtive_ = true;	// �Ƿ��������ڵ�
		float capacity_;				// cloudlet������

		std::shared_ptr<Timer> p_broadcast_timer_;	// ������ӵĶ�ʱ��
		std::shared_ptr<Timer> p_checkstt_timer_;
  
  	};
}

#endif		
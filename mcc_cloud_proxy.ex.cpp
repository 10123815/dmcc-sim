#include "mcc_cloud_proxy.h"

using namespace mcc;

// public
Objid CloudletProxy::_DiscardedNode ( ) const
{
	std::map<Objid, NodeData> discardes = _FindKMinRepNode();
	float max_usage = -1;
	Objid discard_node_id = -1;
	for (auto discard_pair : discardes)
	{
		auto total_deployment = total_com_deployment_;
		if (total_deployment.find(discard_pair.first) != total_deployment.end())
		{
			total_deployment.erase(discard_pair.first);
			float usage = _CpuUsage(total_deployment);
			if (usage > max_usage)
			{
				max_usage = usage;
				discard_node_id = discard_pair.first;
			}
		}
	}
	return discard_node_id;
}

// private

// 找到cloudlet中rep最小的20%的结点
std::map<Objid, NodeData> CloudletProxy::_FindKMinRepNode (float perc) const
{
	// 先把数据copy到vector里，关联容器不适用泛型算法
	std::vector<std::pair<Objid, float>> reps;
	for (auto ele : nodes_)
	{
		reps.push_back(std::make_pair(ele.first, ele.second->rep()));
	}

	// 前n个, 从1开始
	int n = (int)(reps.size() * 0.2f);
	n = n == 0 ? 1 : n;
	auto nth = reps.begin() + n;
	nth_element(reps.begin(), nth, reps.end(), 
		[](std::pair<Objid, float> &p1, std::pair<Objid, float> &p2)
		{
			return p1.second < p2.second;
		});

	//结果
	std::map<Objid, NodeData> result;
	for (int i = 0; i < n; ++i)
	{
		auto id = reps[i].first;
		auto pair = *nodes_.find(id);
		auto p_node_data = pair.second;
		result.insert({id, NodeData(*p_node_data)});
	}
	return result;
}

void CloudletProxy::StartBroadcastDiscoverPkg (unsigned short multi)
{
	discover_avtive_ = true;
	// 每十个delta发一次
	p_broadcast_timer_ = Time::GetInstance()->AddTimer(g_kBroadCastTick, cloudlet_proxy_id_, multi);
}

void CloudletProxy::BroadcastDiscoverPkg ( )
{
	if (!discover_avtive_)
		return;

	Ici* p_ici_dicover_pkg = op_ici_create("mcc2_discover_pkg");
	op_ici_attr_set(p_ici_dicover_pkg, "sender id", &cloudlet_id_);
	op_ici_attr_set(p_ici_dicover_pkg, "size", 32);
	// 发向监听端口
	op_ici_attr_set(p_ici_dicover_pkg, "src node id", cloudlet_id_);
	IciSender::SendRemote(p_ici_dicover_pkg, 0, g_kBroadCast, "mng", 1);
}

void CloudletProxy::StopBroadcastDiscoverPkg ( )
{
	discover_avtive_ = false;
	if (p_broadcast_timer_.use_count() > 0)
	{
		Time::GetInstance()->RemoveTimer(p_broadcast_timer_);
		p_broadcast_timer_.reset();
	}
}

void CloudletProxy::StartListenDiscoverPkg ( )
{
	SimulationManager::GetInstance()->AddListener(g_kDiscoverPkgPort, cloudlet_id_, "cloudlet proxy");
}

void CloudletProxy::OnDiscoverOthers (Ici* p_disc_ici)
{
	if (discover_avtive_)
	{	
		Objid* p_sender_id = 0;
		op_ici_attr_get(p_disc_ici, "sender id", &p_sender_id);
		if (*p_sender_id == cloudlet_id_)
			return;

		// 刚发现对方，不停止广播

		// 发自己的
		Ici* p_capacity_ici = op_ici_create("mcc2_capacity");
		op_ici_attr_set(p_capacity_ici, "usage", usage());
		op_ici_attr_set(p_capacity_ici, "capacity", capacity());
		op_ici_attr_set(p_capacity_ici, "sender id", &cloudlet_id_);
		op_ici_attr_set(p_capacity_ici, "size", 160);
		op_ici_attr_set(p_capacity_ici, "src node id", cloudlet_id_);
		IciSender::SendRemote(p_capacity_ici, 0, g_kCapacity, "cloudlet proxy", *p_sender_id);

	}

}

void CloudletProxy::StopListenDiscoverPkg ( )
{
	discover_avtive_ = false;
	SimulationManager::GetInstance()->RemoveListener(g_kDiscoverPkgPort, cloudlet_id_, "cloudlet proxy");
}

void CloudletProxy::OnGetCapacityFromOther (Ici* p_ici)
{
	double usage = 0;
	double cap = 0;
	Objid* p_sender_id = 0;
	op_ici_attr_get(p_ici, "usage", &usage);
	op_ici_attr_get(p_ici, "capacity", &cap);
	op_ici_attr_get(p_ici, "sender id", &p_sender_id);
	usage = max(0, usage);
	usage = min(1, usage);

	// 自己的
	usage = max(0, this->usage());
	usage = min(1, this->usage());
	double my_cap = capacity();

	if (*p_sender_id != cloudlet_id_ && cap > my_cap)
	{
		// 就是你了！
		// 合并请求
		Ici* p_merge_req_ici = op_ici_create("mcc2_merge_req");
		op_ici_attr_set(p_merge_req_ici, "req cloudlet id", &cloudlet_id_);
		op_ici_attr_set(p_merge_req_ici, "usage", this->usage());
		op_ici_attr_set(p_merge_req_ici, "size", 96);
		op_ici_attr_set(p_merge_req_ici, "src node id", cloudlet_id_);
		IciSender::SendRemote(p_merge_req_ici, 0, g_kMergeRequest, "cloudlet proxy", *p_sender_id);

		// 关闭广播
		StopListenDiscoverPkg();
		StopBroadcastDiscoverPkg();
	}
}

void CloudletProxy::OnGetMergeRequest (Ici* p_ici)
{
	Objid* p_req_cloudlet_id;
	double req_usage = 0;
	op_ici_attr_get(p_ici, "req cloudlet id", &p_req_cloudlet_id);
	op_ici_attr_get(p_ici, "usage", &req_usage);
	// 对合并请求的响应
	Ici* p_merge_res_ici = op_ici_create("mcc2_merge_res");
	op_ici_attr_set(p_merge_res_ici, "res cloudlet id", &cloudlet_id_);
	if (discover_avtive_ && 
		active_ &&
		//(req_usage + this->usage()) / 2 < g_kMaxUsage &&
		*p_req_cloudlet_id != cloudlet_id_)
	{
		// 同意
		op_ici_attr_set(p_merge_res_ici, "ok", 1);
		// 关闭广播
		// StopBroadcastDiscoverPkg();
		// StopListenDiscoverPkg();
	}
	else
	{
		// 我正在请求别人
		op_ici_attr_set(p_merge_res_ici, "ok", 0);
	}

	op_ici_attr_set(p_merge_res_ici, "size", 40);
	op_ici_attr_set(p_merge_res_ici, "src node id", cloudlet_id_);
	IciSender::SendRemote(p_merge_res_ici, 0, g_kMergeResponse, "cloudlet proxy", *p_req_cloudlet_id);

}

void CloudletProxy::OnGetMergeResponse (Ici* p_ici)
{
	Objid* p_res_cloudlet_id;
	int ok = 0;
	op_ici_attr_get(p_ici, "res cloudlet id", &p_res_cloudlet_id);
	op_ici_attr_get(p_ici, "ok", &ok);

	if (ok == 1 && *p_res_cloudlet_id != cloudlet_id_)
	{
		// 对方同意了
		// 合并到...
		Ici* p_merge_to_ici = op_ici_create("mcc2_merge_to");
		op_ici_attr_set(p_merge_to_ici, "child id", &cloudlet_id_);
		op_ici_attr_set(p_merge_to_ici, "nodes", &nodes_);
		op_ici_attr_set(p_merge_to_ici, "deployment", &total_com_deployment_);
		int size = 32 * 9 * total_com_deployment_.size() + 32;
		size += (32 + sizeof(NodeData)) * nodes_.size();
		op_ici_attr_set(p_merge_to_ici, "size", size);
		op_ici_attr_set(p_merge_to_ici, "src node id", cloudlet_id_);
		IciSender::SendRemote(p_merge_to_ici, 0, g_kMerge, "cloudlet proxy", *p_res_cloudlet_id);	

		// 切换移动性模型
		Ici* p_mobility_ici = op_ici_create("mcc2_mobility");
		op_ici_attr_set(p_mobility_ici, "leader id", *p_res_cloudlet_id);
		op_ici_attr_set(p_mobility_ici, "model", kMobility_RPG);
		op_ici_attr_set(p_mobility_ici, "size", 40);
		IciSender::SendLocalProcess(p_mobility_ici, 0, g_kChangeMobility, "mobility");

		// 关闭自己的cloudlet
		set_active(false);

	}
	else
	{
		// 少年不哭，站起来撸
		StartListenDiscoverPkg();
		StartBroadcastDiscoverPkg();
	}

}

void CloudletProxy::Merge (Ici* p_ici)
{
	Objid* p_child_id = 0;
	op_ici_attr_get(p_ici, "child id", &p_child_id);
	if (*p_child_id == cloudlet_id_)
		return;

	// 从一个变成多个
	if (nodes_.size() == 1)
	{
		p_checkstt_timer_ = Time::GetInstance()->AddTimer(g_kCheckState, cloudlet_proxy_id_, 10);
	}
	
	std::map<Objid, std::shared_ptr<NodeData>>* p_nodes = 0;
	std::map<Objid, std::vector<Deployment>>* p_deployment = 0;
	op_ici_attr_get(p_ici, "nodes", &p_nodes);
	op_ici_attr_get(p_ici, "deployment", &p_deployment);

	// 添加至节点列表
	for (auto ele : *p_nodes)
	{
		AddNodeData(ele.first, ele.second);
	}
	// 添加至组件分配列表
	for (auto ele : *p_deployment)
	{
		total_com_deployment_[ele.first] = ele.second;
	}

	Plan();

	// 指向自己
	*p_child_id = cloudlet_id_;

	// 合并后，重开广播
	if (!discover_avtive_)
	{
		StartBroadcastDiscoverPkg();
		StartListenDiscoverPkg();
	}
}

void CloudletProxy::Plan ( )
{
	total_com_deployment_ = Utility::SimulatedAnnealed(total_com_deployment_, this, &CloudletProxy::_RandomNewSolution);
	Deploy();
}

// send deploying strategy to each node
void CloudletProxy::Deploy ( ) const
{
	// 每个节点的
	for (auto deployment_pair : total_com_deployment_)
	{
		Objid child_id = deployment_pair.first;
		Ici* p_deployment_ici = op_ici_create("mcc2_deployment");
		int index = 0;
		int size = 0;
		for (auto deployment : deployment_pair.second)
		{
			for (auto id : deployment.coffload_nodes())
			{
				char attr_name[5];
				sprintf(attr_name, "%d", index++);
				size += 32;
				op_ici_attr_set(p_deployment_ici, attr_name, id);
			}
		}
		size += 32;
		op_ici_attr_set(p_deployment_ici, "com size", index);
		op_ici_attr_set(p_deployment_ici, "size", size);
		IciSender::SendRemote(p_deployment_ici, 0, g_kDeployment, "apps", child_id);	
	}
}

float CloudletProxy::_CpuUsage (const std::map<Objid, std::vector<Deployment>>& deployments) const
{
	float total_usage = 0;
	for (const auto& ele : deployments)
	{
		// 结点请求产生的usage
		float node_cpu_usage = 0;
		for (const auto& deployment : ele.second)
		{
			// 每一个com对应一个offload结点
			auto offloads = deployment.coffload_nodes();
			auto p_coms = coms_regedit_.find(deployment.app_index());
			if (p_coms == coms_regedit_.cend())
			{
				std::cout<<"no this component\n";
				return -1;
			}

			auto coms = p_coms->second;
			if (coms.size() != offloads.size())
			{
				std::cout<<"wrong size\n";
				return -1;
			}

			// app产生的cpu usage
			float app_cpu_usage = 0;
			for (int i = 0; i < offloads.size(); ++i)
			{
				auto iter_node_data = nodes_.find(offloads[i]);
				if (iter_node_data != nodes_.cend())
				{
					NodeData node_data = *(iter_node_data->second);
					float cpu_cap = node_data.cores() * node_data.speed();
					float cpu_usage = coms[i].CpuLoad() / cpu_cap * coms[i].freq();
					app_cpu_usage += cpu_usage;
				}
			}
			node_cpu_usage += app_cpu_usage;
		}
		total_usage += node_cpu_usage;
	}
	return total_usage / nodes_.size();
}


std::map<Objid, std::vector<Deployment>> CloudletProxy::_RandomNewSolution (const std::map<Objid, std::vector<Deployment>>& sln, float* p_delta_usage) const
{
	std::map<Objid, std::vector<Deployment>> new_sln = sln;
	if (new_sln.size() == 1)
	{
		Objid node_id = new_sln.begin()->first;
		for (auto& dep : new_sln.begin()->second)
		{
			for (int i = 0; i < 3; ++i)
			{
				dep.offload_nodes()[i] = node_id;
			}
		}
		return new_sln;
	}

	// 全部node
	std::vector<Objid> ids;
	for (const auto& ele : new_sln)
	{
		ids.push_back(ele.first);
	}

	// 改变其中一个
	std::uniform_int_distribution<> u_node(0, ids.size() - 1);
	int change_node_id = u_node(gen);

	if (change_node_id < ids.size())
	{
		// 改变某个node
		auto iter_sln = new_sln.find(ids[change_node_id]);
		std::vector<Deployment>& apps = iter_sln->second;
		std::uniform_int_distribution<> u_app(0, 1);
		int app_index = u_app(gen);

		// 某个组件是否改变offload
		auto coms = coms_regedit_.find(app_index)->second;
		std::uniform_real_distribution<float> u_com(0, 2);
		int i = u_com(gen);

		// 新的offload结点
		Objid cur_node_id = apps[app_index].offload_nodes()[i];
		std::uniform_int_distribution<> u_offload(0, ids.size() - 2);
		int new_offload_node_i = u_offload(gen);
		Objid new_node_id = ids[new_offload_node_i];
		if (new_node_id == cur_node_id)
			new_node_id = ids[ids.size() - 1];
		apps[app_index].offload_nodes()[i] = new_node_id;

		// usage差
		// 减去旧的(找最大的)
		NodeData node_data;
		auto cur_node_iter = nodes_.find(cur_node_id);
		if (cur_node_iter != nodes_.cend())
		{
			node_data = *(cur_node_iter->second);
		}	
		else
		{
			NodeData node_data = last_removed_node_.second;
		}

		float cpu_cap = node_data.cores() * node_data.speed();
		float cpu_usage = coms[i].CpuLoad() / cpu_cap * coms[i].freq();
		*p_delta_usage += cpu_usage;

		// 加上新的(找最大的)
		auto new_node_iter = nodes_.find(new_node_id);
		node_data = *(new_node_iter->second);
		cpu_cap = node_data.cores() * node_data.speed();
		cpu_usage = coms[i].CpuLoad() / cpu_cap * coms[i].freq();
		*p_delta_usage -= cpu_usage;

	}
	return new_sln;
}

std::map<Objid, std::vector<Deployment>> CloudletProxy::_RandomNewSolutionBW (const std::map<Objid, std::vector<Deployment>>& sln, float* p_delta_usage) const
{

	std::map<Objid, std::vector<Deployment>> new_sln = sln;
	if (new_sln.size() == 1)
	{
		Objid node_id = new_sln.begin()->first;
		for (auto& dep : new_sln.begin()->second)
		{
			for (int i = 0; i < 3; ++i)
			{
				dep.offload_nodes()[i] = node_id;
			}
		}
		return new_sln;
	}

	// 全部node
	std::vector<Objid> ids;
	for (const auto& ele : new_sln)
	{
		ids.push_back(ele.first);
	}

	// 改变其中一个
	std::uniform_int_distribution<> u_node(0, ids.size() - 1);
	int change_node_id = u_node(gen);

	if (change_node_id < ids.size())
	{
		// 改变某个node
		auto iter_sln = new_sln.find(ids[change_node_id]);
		std::vector<Deployment>& apps = iter_sln->second;
		std::uniform_int_distribution<> u_app(0, 1);
		int app_index = u_app(gen);

		// 某个组件是否改变offload
		auto coms = coms_regedit_.find(app_index)->second;
		std::uniform_real_distribution<float> u_com(0, 2);
		int i = u_com(gen);

		// 新的offload结点
		Objid cur_node_id = apps[app_index].offload_nodes()[i];
		std::uniform_int_distribution<> u_offload(0, ids.size() - 2);
		int new_offload_node_i = u_offload(gen);
		Objid new_node_id = ids[new_offload_node_i];
		if (new_node_id == cur_node_id)
			new_node_id = ids[ids.size() - 1];
		apps[app_index].offload_nodes()[i] = new_node_id;

		// usage差
		// 减去旧的(找最大的)
		NodeData node_data;
		if (cur_node_id != last_removed_node_.first)
		{
			auto cur_node_iter = nodes_.find(cur_node_id);
			node_data = *(cur_node_iter->second);
		}	
		else
		{
			NodeData node_data = last_removed_node_.second;
		}
		float cpu_cap = node_data.cores() * node_data.speed();
		float cpu_usage = coms[i].CpuLoad() / cpu_cap * coms[i].freq();
		*p_delta_usage += cpu_usage;

		// 加上新的(找最大的)
		auto new_node_iter = nodes_.find(new_node_id);
		node_data = *(new_node_iter->second);
		cpu_cap = node_data.cores() * node_data.speed();
		cpu_usage = coms[i].CpuLoad() / cpu_cap * coms[i].freq();
		*p_delta_usage -= cpu_usage;

		float net_usage = coms[i].NetLoad() / g_kBandWidth;
		if (change_node_id != cur_node_id)
		{
			// 旧的卸载在远程，减去旧的
			*p_delta_usage += net_usage;
		}

		if (change_node_id != new_node_id)
		{
			// 新的卸载在远程，加上新的
			*p_delta_usage -= net_usage;
		}

	}
	return new_sln;
}

void CloudletProxy::CheckCloudletState ( )
{

	// 被动
	bool removing = false;
	for (auto node_pair : nodes_)
	{
		if (node_pair.first == cloudlet_id_)
			continue;

		// 已经在等待队列了
		if (removed_nodes_.find(node_pair.first) != removed_nodes_.cend())
			continue;

		float snr = Utility::SINR(node_pair.first, cloudlet_id_, false);
		if (snr < g_kMinConnectSNR)
		{
			// 检查状态
			removing = true;
			Ici* p_ici = op_ici_create("mcc2_com_running_stt");
			op_ici_attr_set(p_ici, "cloudlet id", cloudlet_id_);
			op_ici_attr_set(p_ici, "size", 32);
			op_ici_attr_set(p_ici, "src node id", cloudlet_id_);
			IciSender::SendRemote(p_ici, 0, g_kCloudletCheck, "apps", node_pair.first);
			removed_nodes_.insert(node_pair.first);
		}
	}
	if (removing)
		return;

	// 主动
	if (usage() > g_kMaxUsage)
	{
		Objid discard_node_id = _DiscardedNode();
		if (discard_node_id == cloudlet_id_ || 
			discard_node_id == -1 ||
			nodes_.find(discard_node_id) == nodes_.end() ||
			removed_nodes_.find(discard_node_id) != removed_nodes_.cend())
			return;

		Ici* p_ici = op_ici_create("mcc2_com_running_stt");
		op_ici_attr_set(p_ici, "cloudlet id", cloudlet_id_);
		op_ici_attr_set(p_ici, "size", 32);
		op_ici_attr_set(p_ici, "src node id", cloudlet_id_);
		IciSender::SendRemote(p_ici, 0, g_kCloudletCheck, "apps", discard_node_id);
	}
}

void CloudletProxy::RemoveNode (Ici* p_ici)
{
	Objid discard_node_id = 0;
	op_ici_attr_get(p_ici, "discard", &discard_node_id);

	if (removed_nodes_.find(discard_node_id) != removed_nodes_.end() &&
		nodes_.find(discard_node_id) != nodes_.cend())
	{
		removed_nodes_.erase(discard_node_id);
		auto removed_iter = nodes_.find(discard_node_id);
		NodeData removed_node_data = *(removed_iter->second);
		last_removed_node_ = std::make_pair(discard_node_id, removed_node_data);
		RemoveNodeData(discard_node_id);

		// 告诉child，“你可以走了”
		Ici* p_restart_ici = op_ici_create("mcc2_restart"); 
		op_ici_attr_set(p_restart_ici, "restart id", discard_node_id);
		op_ici_attr_set(p_restart_ici, "size", 32);
		op_ici_attr_set(p_restart_ici, "src node id", cloudlet_id_);
		IciSender::SendRemote(p_restart_ici, 0, g_kRestart, "cloudlet proxy", discard_node_id);

		// 有多个变成一个
		if (nodes_.size() == 1 && p_checkstt_timer_.use_count() > 0)
		{
			Time::GetInstance()->RemoveTimer(p_checkstt_timer_);
			p_checkstt_timer_.reset();
		}
		
		Plan();
	}

}
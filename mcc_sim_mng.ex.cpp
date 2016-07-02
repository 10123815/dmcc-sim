#include "mcc_sim_mng.h"

using namespace mcc;

Vector2 Vector2::Lerp (const Vector2 &from, const Vector2 &to, float t)
{
    Vector2 v = to - from;
    t = t > 1 ? 1 : t;
    t = t < 0 ? 0 : t;
    return v * t;
}

Vector2 Vector2::NodePosition (const Objid& node_id)
{
	double x = 0;
	double y = 0;
	op_ima_obj_attr_get_dbl(node_id, "x", &x);
	op_ima_obj_attr_get_dbl(node_id, "y", &y);
	return Vector2(x, y);
}

float Vector2::NodeDistance (const Objid& node_id1, const Objid& node_id2)
{
	Vector2 pos1 = NodePosition(node_id1);
	Vector2 pos2 = NodePosition(node_id2);
	return Distance(pos1, pos2);
}

void IciSender::SendLoacl (Ici* p_ici, float delay, int intrpt_code)
{
	if (p_ici != OPC_NIL)
		op_ici_install(p_ici);
	op_intrpt_schedule_self(op_sim_time() + max(0, delay), intrpt_code);
	if (p_ici != OPC_NIL)
		op_ici_install(OPC_NIL);
}

void IciSender::SendLocalProcess (Ici* p_ici, float delay, int intrpt_code, const char* proc_name)
{
	Objid parent_id = op_topo_parent(op_id_self());
	Objid proc_id = op_id_from_name(parent_id, OPC_OBJTYPE_PROC, proc_name);
	if (p_ici != OPC_NIL)
		op_ici_install(p_ici);
	op_intrpt_schedule_remote(op_sim_time() + max(0, delay), intrpt_code, proc_id);
	if (p_ici != OPC_NIL)
		op_ici_install(OPC_NIL);
}

void IciSender::SendRemote (Ici* p_ici, float delay, int intrpt_code, const char* proc_name, Objid node_id)
{

	Objid proc_id = op_id_from_name(node_id, OPC_OBJTYPE_PROC, proc_name);
	if (p_ici != OPC_NIL)
	{
		Objid src_node_id = 0;
		op_ici_attr_get(p_ici, "src node id", &src_node_id);

		if (node_id >= 2 && src_node_id >= 2)
		{
			// sinr
			double sinr = Utility::SINR(src_node_id, node_id, false);
	
			op_ici_attr_set(p_ici, "sinr", sinr);
		}

		op_ici_install(p_ici);
	}
	op_intrpt_schedule_remote(op_sim_time() + max(0, delay), intrpt_code, proc_id);
	if (p_ici != OPC_NIL)
		op_ici_install(OPC_NIL);
}

/*
IciSender end
*/


/*
SimulationManager
*/

SimulationManager* SimulationManager::GetInstance ( )
{
	return Singleton<SimulationManager>::GetInstance();
}

// 添加结点到某个监听端口，该端口收到中断后，再在结点上触发中断
void SimulationManager::AddListener (int port, Objid node_id, std::string proc_name)
{
	auto listener = std::make_pair(node_id, proc_name);
	listeners_[port].insert(listener);
}

void SimulationManager::RemoveListener (int port, Objid node_id, std::string proc_name)
{
	auto listener = std::make_pair(node_id, proc_name);
	listeners_[port].erase(listener);
}

void SimulationManager::Forward (Ici* p_ici)
{
	int port = 0;
	Objid* p_sender_id = 0;
	Objid src_node_id = 0;
	op_ici_attr_get(p_ici, "port", &port);
	op_ici_attr_get(p_ici, "sender id", &p_sender_id);
	op_ici_attr_get(p_ici, "src node id", &src_node_id);
	op_ici_destroy(p_ici);
	for (auto listener : listeners_[port])
	{
		auto to_id = listener.first;
		// 不再发给自己
		if (*p_sender_id != to_id)
		{
			float snr = Utility::SINR(*p_sender_id, to_id, false);
			// 一定距离以内
			if (snr > g_kMinConnectSNR)
			{
				std::uniform_real_distribution<float> u(1, 2);
				auto proc_name = listener.second.c_str();
				Ici* p_disc_ici = op_ici_create("mcc2_discover_pkg");
				op_ici_attr_set(p_disc_ici, "sender id", p_sender_id);
				op_ici_attr_set(p_disc_ici, "port", port);
				op_ici_attr_set(p_disc_ici, "size", 64);
				op_ici_attr_set(p_disc_ici, "src node id", src_node_id);
				IciSender::SendRemote(p_disc_ici, u(gen), g_kDiscoverOther, proc_name, to_id);
			}
		}
	}
}

/*
SimulationManager end
*/


/*
Utility
*/

float Utility::PositiveNormDist (float e, float d)
{
    std::normal_distribution<float> norm_dist(e, d);
    float result = norm_dist(gen);
    if (result < e)
    	result = e * 2.0f - result;
    return result;
}

float Utility::PositiveNormFromExp (float e, float d)
{
    float ee = (std::exponential_distribution<>(e))(gen);
    float dd = (std::exponential_distribution<>(d))(gen);
    return PositiveNormDist(ee, dd);
}

// 返回两个节点间的随机时延
float Utility::Delay(Objid from, Objid to)
{
	Vector2 pos_src = Vector2::NodePosition(from);
	Vector2 pos_des = Vector2::NodePosition(to);
	float dis = Vector2::Distance(pos_src, pos_des);
	std::uniform_real_distribution<float> u(1, 2);
	return u(gen) + dis / g_kMaxConnectDis;
}

float Utility::SINR (Objid tran_node_id, Objid recv_node_id, bool inter)
{

	float dis = Vector2::NodeDistance(tran_node_id, recv_node_id);
	// 发射功率
	double P = 10 * log10(0.151) - 46 - 30 * log(dis);
	P = pow(10, P / 10);
	// 噪声
	double N = 2e-10;
	// 干扰
	double I = 0;
	const std::vector<Objid>& nodes = SimulationManager::GetInstance()->NodeRef();
	if (inter)
	{
		for (auto id : nodes)
		{
			if (id != tran_node_id && id != recv_node_id)
			{
				dis = Vector2::NodeDistance(id, recv_node_id);
				double i = 10 * log10(0.151) - 46 - 30 * log(dis);
				i = pow(10, i / 10);
				I += i;
			}
		}
	}
	double sinr = P / (N + I);
	return 10 * log10(sinr);
}
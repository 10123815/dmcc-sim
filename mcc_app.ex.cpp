#include "mcc_app.h"

using namespace mcc;

// 该方法一次执行的耗时
float Method::Execute (float speed)
{
	speed = speed <= 1.0f ? 1.0f : speed;
	float t = cpu_load_ / speed;
	return t;
}

void Method::Load (float *p_cpu, float *p_mem, float *p_bw)
{
	*p_cpu += max(0, cpu_load_ * calls_);
	*p_mem += max(0, mem_load_);
	*p_bw  += max(0, (arg_size_ + res_size_) * calls_); 
}

void Method::_InitParam ( )
{
	// cpu负载
	cpu_load_ = Utility::PositiveNormFromExp(0.01f, 0.167f);
	// cpu_load_ = 100.0f;

	// 内存消耗
	mem_load_ = 1024.0f;

	// 参数、返回值
	arg_size_ = Utility::PositiveNormFromExp(0.00000125f, 0.0000083f);
	res_size_ = Utility::PositiveNormFromExp(0.0000125f, 0.000083f);

	// 调用次数
	std::geometric_distribution<> calls_dist(0.25f);
	calls_ = calls_dist(gen);
}

// method个数分布
std::geometric_distribution<> Component::com_size_dist_ = std::geometric_distribution<>(0.25);

float Component::Run (float speed)
{
	float exe_time = 0.1f;
	for (std::vector<Method>::iterator i = methods_.begin(); i != methods_.end(); ++i)
	{
		float method_exe_time = i->Execute(speed);
		exe_time += method_exe_time;
		i->Load(&cpu_usage_, &mem_usage_, &net_usage_);
	}
	return exe_time;
}


/*
App
*/

void App::Start ( )
{
	// 在一个随机时间后用户开始使用
	std::exponential_distribution<> e_dist(10);
	float delay = max(0.1f, e_dist(gen));
	// 第一个组件在本地执行
	Ici* p_call = op_ici_create("mcc2_call");
	op_ici_attr_set(p_call, "req node id", &node_id_);
	op_ici_attr_set(p_call, "com", 0);
	op_ici_attr_set(p_call, "app", static_cast<int>(app_type_));
	IciSender::SendLoacl(p_call, delay, g_kComLocalExe);
}

void App::OnComponentStart (const int com_index, const Objid& req_node_id, std::shared_ptr<NodeData> p_node_data)
{

	// 确定执行时间和负载
	float cpu_cap = p_node_data->cores() * p_node_data->speed();
	float exe_time = coms_[com_index].Run(cpu_cap); 

	float finish_time = exe_time + Time::GetInstance()->cur_time() / 1000;

	float cpu_usage = g_kCpuUsageWeight * coms_[com_index].cpu_usage() / cpu_cap * coms_[com_index].freq();
	float mem_usage = g_kMemUsageWeight * coms_[com_index].mem_usage() / p_node_data->mem();
	// float net_usage = g_kBwUsageWeight  * coms_[com_index].net_usage() / p_node_data->bandwidth() * coms_[com_index].freq();
	coms_[com_index].Clear();
	float cur_usage = cpu_usage + mem_usage;

	Ici* p_return = op_ici_create("mcc2_return");
	// 执行该组件的node
	op_ici_attr_set(p_return, "exe node id", &node_id_);
	op_ici_attr_set(p_return, "com", com_index);
	op_ici_attr_set(p_return, "app", static_cast<int>(app_type_));
	if (req_node_id == node_id_)
	{
		// 请求来自本地
		// 从com_start到com_end，该component的usage始终存在
		// 直到com_end，再将这次的usage减去
		p_node_data->set_usage(p_node_data->usage() + cur_usage);
		op_ici_attr_set(p_return, "remote", 0);
		op_ici_attr_set(p_return, "usage", cur_usage);
		IciSender::SendLoacl(p_return, exe_time, g_kComLocalExeEnd);
	}
	else
	{
		// 请求来自远程
		// cur_usage += net_usage;
		p_node_data->set_usage(p_node_data->usage() + cur_usage);
		op_ici_attr_set(p_return, "remote", 1);
		op_ici_attr_set(p_return, "usage", cur_usage);
		// 发给本地，以计算rep，usage
		IciSender::SendLoacl(p_return, exe_time, g_kComLocalExeEnd);

		// 发给调用者，以计算rep，新的
		Ici* p_return_remote = op_ici_create("mcc2_return");
		op_ici_attr_set(p_return_remote, "exe node id", &node_id_);
		op_ici_attr_set(p_return_remote, "com", com_index);
		op_ici_attr_set(p_return_remote, "app", static_cast<int>(app_type_));
		op_ici_attr_set(p_return_remote, "remote", 1);
		op_ici_attr_set(p_return_remote, "usage", cur_usage);
		op_ici_attr_set(p_return_remote, "src node id", node_id_);
		IciSender::SendRemote(p_return_remote, exe_time, g_kComRemoteExeEnd, "apps", req_node_id);
		std::cout<<"finish time : "<<finish_time<<std::endl;

		// 告知调用者offload细节
		Ici* p_offload_info_ici = op_ici_create("mcc2_offload_info");
		op_ici_attr_set(p_offload_info_ici, "size", 32);
		op_ici_attr_set(p_offload_info_ici, "finish time", (double)finish_time);
		op_ici_attr_set(p_offload_info_ici, "app", static_cast<int>(app_type_));
		op_ici_attr_set(p_offload_info_ici, "com", com_index);
		op_ici_attr_set(p_offload_info_ici, "src node id", node_id_);
		IciSender::SendRemote(p_offload_info_ici, 0, g_kOffloadInfo, "apps", req_node_id);
	}
}

void App::OnComponentEnd (int com_index)
{
	// 获取下一组件编号
	std::map<int, float> probilities;
	int i = 0;
	float sum = 0;
	for (auto is_next : app_gra_[com_index])
	{
		if (is_next == 1)
		{
			float freq = coms_[i].freq();
			sum += freq;
			probilities[i] = sum;
		}	
		i++;
	}
	if (sum == 0)
	{
		// 没有下一个组件
		Start();
		return;
	}
	std::uniform_real_distribution<float> u(0, sum);
	float p = u(gen);
	int next_com_index = -1;
	for (auto prob : probilities)
	{
		if (p < prob.second)
		{
			next_com_index = prob.first;
			break;
		}
	}
	if (next_com_index == -1)
	{
		// 没有下一个组件
		Start();
		return;
	}

	// offload的结点
	Objid offload_id = offloads_[next_com_index];

	// 执行下一个组件
	Ici* p_call = op_ici_create("mcc2_call");
	// 由app所在的node发出请求
	op_ici_attr_set(p_call, "req node id", &node_id_);
	op_ici_attr_set(p_call, "com", next_com_index);
	op_ici_attr_set(p_call, "app", static_cast<int>(app_type_));
	float delay = 0;
	if (offload_id == node_id_)
	{
		IciSender::SendLoacl(p_call, delay, g_kComLocalExe);
	}
	else
	{
		// 这里应该有一个方法计算rpc的延迟
		delay += 0.01f;
		op_ici_attr_set(p_call, "src node id", node_id_);
		IciSender::SendRemote(p_call, delay, g_kComRemoteExe, "apps", offload_id);
	}
}

bool AppManager::OnGetOffloadStrategy (Ici* p_ici)
{
	std::vector<Objid> ids;
	int size = 0;
	op_ici_attr_get(p_ici, "com size", &size);
	int app_index = 0;

	// 分配是否改变
	bool changed = false;

	for (int i = 0; i < size; ++i)
	{
		char attr_name[4];
		sprintf(attr_name, "%d", i);
		int index = 0;
		op_ici_attr_get(p_ici, attr_name, &index);
		Objid id = static_cast<Objid>(index);
		ids.push_back(id);
		if ((i + 1) % 3 == 0)
		{
			changed = apps_[app_index]->CopyOffload() != ids;
			apps_[app_index++]->Offload(ids);
			ids.clear();
		}
	}
	return changed;
}

void AppManager::OnGetOffloadInfo (Ici* p_ici)
{
	double finish_time = 0;
	int app_index = 0;
	int com_index = 0;
	op_ici_attr_get(p_ici, "finish time", &finish_time);
	op_ici_attr_get(p_ici, "com", &com_index);
	op_ici_attr_get(p_ici, "app", &app_index);

	finish_time += Time::GetInstance()->cur_time();
	float mft = app(app_index)->max_finish_time();
std::cout<<"finish time : "<<finish_time<<std::endl;
	if (finish_time > mft)
	{
		app(app_index)->set_max_finish_time(finish_time);
		app(app_index)->set_last_offload_com(com_index);
	}

}

float AppManager::OnRemoved (Ici* p_ici)
{

	Objid cloudlet_id_ = 0;
	op_ici_attr_get(p_ici, "cloudlet id", &cloudlet_id_);

	float cur_time = Time::GetInstance()->cur_time();

	// 本地运行和卸载运行的时间差
	float max_delay = 0;
	// 全部应用最晚执行完时间
	float max_finish_time = 0;
	for (auto p_app : apps_)
	{
		float max_app_finish_time = p_app->max_finish_time();
		if (max_app_finish_time != -1)
		{
			int com_index = p_app->last_offload_com();
			if (com_index != -1)
			{
				float cpu_cap = p_node_data_->cores() * p_node_data_->speed();
				float self_exe_time = p_app->CopyComponent()[com_index].Run(cpu_cap);
				self_exe_time += cur_time;
				max_delay = max(self_exe_time - (max_app_finish_time + 25.48f), max_delay);
				max_finish_time = max(max_finish_time, max_app_finish_time);
			}
			
		}
	}


	float delay = 0;
	if (max_finish_time > cur_time + g_kHandoffTime)
	{
		// 立即移除
	}
	else
	{
		// 执行完再移除
		delay += max_finish_time - cur_time;
		max_delay = 0;
	}

	// 告诉cloudlet，我要走了
	Ici* p_client_removed_ici = op_ici_create("mcc2_remove");
	op_ici_attr_set(p_client_removed_ici, "discard", node_id_);
	op_ici_attr_set(p_client_removed_ici, "src node id", node_id_);
	IciSender::SendRemote(p_client_removed_ici, delay, g_kCloudletRemove, "cloudlet proxy", cloudlet_id_);

	return max_delay;
}

/*
App end
*/


AppFactory* AppFactory::GetInstance ( )
{
	return Singleton<AppFactory>::GetInstance();
}

// 初始化所有应用
void AppFactory::_CreateApp ( )
{
	std::vector<std::vector<std::vector<unsigned short>>> v
	{
		{
			{0, 1, 0},
			{1, 0, 1},
			{0, 0, 0},
		}, 
		{
			{0, 1, 0},
			{0, 0, 1},
			{1, 0, 0},
		}
	};
	for (int i = 0; i < v.size(); ++i)
	{
		apps_.push_back(App(v[i], static_cast<AppType>(i)));
	}
}

App AppFactory::GetAppFromType (AppType type)
{
	int index = static_cast<int>(type);
	return apps_[index];
}

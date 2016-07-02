#include "mcc_timer.h" 

using namespace mcc;

Time* Time::GetInstance ( )
{
	return Singleton<Time>::GetInstance();
}

void Time::Go ( )
{
	cur_time_ += delta_time_;
	op_intrpt_schedule_self(op_sim_time() + delta_time_, 0);

	// 触发定时器
	auto iter = timers_.begin();
	// 删除定时器，只在次循环
	while (iter != timers_.end())
	{
		if ((*iter)->times() == 0)
		{
			iter = timers_.erase(iter);
			continue;
		}

		float added_time = cur_time_ - (*iter)->add_time();
		float multi = (*iter)->multi() * delta_time_;
		// 判断浮点数整除
		int a = added_time / multi;
		float b = added_time / multi;
		// 时辰到了....
		if (a == b)
		{
			op_intrpt_schedule_remote(op_sim_time(), (*iter)->intrpt_code(), (*iter)->proc_id());
			if ((*iter)->times() > 0)
				(*iter)->minus_times();

			// 已经跑完次数
			if ((*iter)->times() == 0)
			{
				iter = timers_.erase(iter);
			}
			else
				iter++;
		}
		else
			iter++;
	}
}

std::shared_ptr<Timer> Time::AddTimer (int intrpt_code, Objid node_id, char* proc_name, unsigned short multi, unsigned short times)
{
	Objid proc_id = op_id_from_name(node_id, OPC_OBJTYPE_PROC, proc_name);
	return AddTimer(intrpt_code, proc_id, multi, times);
}

std::shared_ptr<Timer> Time::AddTimer (int intrpt_code, Objid proc_id, unsigned short multi, unsigned short times)
{
	std::shared_ptr<Timer> p_timer = std::make_shared<Timer>(times, multi, cur_time_, proc_id, intrpt_code);
	timers_.push_back(p_timer);
	return p_timer;
}

void Time::RemoveTimer (std::shared_ptr<Timer> p_timer)
{

	if (p_timer != nullptr && p_timer.use_count() > 0)
	// 只需置剩余次数为0，删除自有别人做，dry
		p_timer->set_times(0);

}
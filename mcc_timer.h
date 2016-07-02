#ifndef _MMC_TIMER_H_
#define _MMC_TIMER_H_

#include "mcc_d_n.h"
#include "singleton.h"

namespace mcc
{

	// 定时器
	class Timer
	{
	public:
		Timer (unsigned short times, unsigned short multi, float add_time, Objid proc_id, int intrpt_code):
		times_ 		(times), 
		multi_ 		(multi), 
		add_time_ 	(add_time),
		proc_id_	(proc_id),
		intrpt_code_(intrpt_code)
		{}

		// 取、设值
		unsigned short times ( ) const { return times_; }
		void set_times (unsigned short val) { times_ = (val < 0) ? 0 : val; }
		void minus_times ( ) { times_--; }
		unsigned short multi ( ) const { return multi_; }
		float add_time ( ) const { return add_time_; }
		Objid proc_id ( ) const { return proc_id_; }
		int intrpt_code ( ) const { return intrpt_code_; }

	private:
		unsigned short times_ = -1;	// 触发次数，-1为无限次
		unsigned short multi_ = 1;	// 间隔
		float add_time_;			// 添加时间
		Objid proc_id_;				// 在哪个进程上触发
		int intrpt_code_;			// 中断码
	};

	class Time
	{

	public:


		static Time* GetInstance ( );
		double cur_time ( ) const { return cur_time_; }
		double delta_time ( ) const { return delta_time_; }

		// 跳到下一时刻
		void Go ( );

		// 添加定时器后的进程将按照给定的步长循环触发中断，共触发times次
		// 步长是delta_time的multi倍
		std::shared_ptr<Timer> AddTimer (int intrpt_code, Objid node_id, char* proc_name, unsigned short multi, unsigned short times = -1);
		std::shared_ptr<Timer> AddTimer (int intrpt_code, Objid proc_id, unsigned short multi, unsigned short times = -1);
		
		// 只能remove循环定时器
		void RemoveTimer (std::shared_ptr<Timer> timer);

	private:
		std::vector<std::shared_ptr<Timer>> timers_;
		double delta_time_ = 0.1;
		double cur_time_;
			
	};
}

#endif

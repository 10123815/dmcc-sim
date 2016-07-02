#ifndef _MCC_APP_H_
#define _MCC_APP_H_ 

#include "mcc_d_n.h"
#include "mcc_node.h"
#include "mcc_sim_mng.h"
#include "mcc_timer.h"
#include "singleton.h"

namespace mcc
{

	class Method
	{
	public:
		Method ( )
		{
			running_ = false;
			// 确保他们是正的
			_InitParam();
		}

		Method (float cpu_load, float mem_load, float arg_size, float res_size, float calls):
		cpu_load_ (cpu_load),
		mem_load_ (mem_load),
		arg_size_ (arg_size), 
		res_size_ (res_size),
		calls_    (calls)
		{
			running_ = false;
		}
		
		// 执行该方法, 返回执行时间
		float Execute (float speed);

		float CpuLoad ( ) const { return max(0.1f, cpu_load_ * calls_); }

	 	float NetLoad ( ) const { return max(0.1f, (arg_size_ + res_size_) * calls_); }

		// 获取方法执行的资源使用
		void Load (float *p_cpu, float *p_mem, float *p_bw);
		
	private:
		// 初始化各参数
		void _InitParam ( );

		float cpu_load_;
		float mem_load_;
		float arg_size_;
		float res_size_;
		unsigned short calls_;
		bool running_;

	};

	class Component
	{
	public:
		Component ( )
		{
			int size = com_size_dist_(gen);
			for (int i = 0; i < size; ++i)
			{
				methods_.push_back(Method());
			}

			// 被调用的频率
			std::uniform_real_distribution<> u(0, 1);
			freq_ = u(gen); 
		}

		// 设、取值
		float cpu_usage ( ) const { return cpu_usage_; }
		float mem_usage ( ) const { return mem_usage_; }
		float net_usage ( ) const { return net_usage_; }
		float freq ( ) const { return freq_; }
		void Clear ( ) 
		{
			cpu_usage_ = 0;
			mem_usage_ = 0;
			net_usage_ = 0;
		}

		float CpuLoad ( ) const
		{
			float cpu_load = 0;
			for (auto& method : methods_)
			{
				cpu_load += method.CpuLoad();
			}
			return cpu_load;
		}

		float NetLoad ( ) const
		{
			float net_load = 0;
			for (auto& method : methods_)
			{
				net_load += method.NetLoad();
			}
			return net_load;
		}

		void set_offload_node (Objid id) { offload_node_id_ = id; }

		void set_node_id (Objid id) { node_id_ = id; }

		// 运行这个组件
		float Run (float speed);

		// 运行结束
		void OnFinish (float finish_time);
		
	private:
		static std::geometric_distribution<> com_size_dist_;	// Component大小，method个数

		float cpu_usage_ = 0.0f;			// CPU使用，每个方法执行后累加
		float mem_usage_ = 0.0f;
		float net_usage_ = 0.0f;
		float freq_;						// 该组件的被调用频率
		Objid offload_node_id_;	// 该组件被卸载到这个节点上
		std::vector<Method> methods_;

		Objid node_id_;		// 初始所在的node
	};

	class App
	{
	public:
		App (std::vector<std::vector<unsigned short>> app_gra, AppType type) : 
		app_type_ (type)
		{
			app_gra_ = app_gra;
			size_ = app_gra.size();
			for (int i = 0; i < size_; ++i)
			{		
				coms_.push_back(Component());
			}
		}

		Objid node_id ( ) { return node_id_; }

		void set_node_id (Objid id) 
		{ 
			node_id_ = id; 
			for (int i = 0; i < size_; ++i)
			{
				offloads_.push_back(id);
				coms_[i].set_node_id(id);
				coms_[i].set_offload_node(id);
			}
		}

		float max_finish_time ( ) const { return max_finish_time_; }
		void set_max_finish_time (float value) {max_finish_time_ = value; }

		int last_offload_com ( ) const { return last_offload_com_; }
		void set_last_offload_com (int value) { last_offload_com_ = value; }

		std::vector<Component> CopyComponent ( ) { return coms_; }

		void Offload (std::vector<Objid> offloads)
		{
			if (offloads.size() != coms_.size())
			{
				std::cout<<"fuck\n";
				return;
			}

			for (int i = 0; i < offloads.size(); ++i)
			{
				offloads_[i] = offloads[i];
			}
		}

		std::vector<Objid> CopyOffload ( ) { return offloads_; }
		
		// 开始执行
		void Start ( );

		// 执行某个component
		void OnComponentStart (const int com_index, const Objid& req_node_id, std::shared_ptr<NodeData> p_node_data);

		// 执行结束
		void OnComponentEnd (int com_index);

	private:
		AppType app_type_;	//
		Objid node_id_;		// app安装到的node	

		unsigned short size_;
		std::vector<Component> coms_;							// 该应用所有组件
		std::vector<Objid> offloads_;							// 某个组件卸载到的结点
		std::vector< std::vector<unsigned short> > app_gra_;	// 定义组件之间的相互调用

		float max_finish_time_ = -1;		// 正在执行的com的最长估计执行时间
		int last_offload_com_ = -1;		// 正在卸载的组件
	};

	// 这个类创建独一无二的一个App
	class AppFactory
	{
	public:
		AppFactory ( )
		{
			_CreateApp();
		}
		static AppFactory* GetInstance ( );
        App GetAppFromType (AppType type);

    private:
        void _CreateApp ( );
        std::vector<App> apps_;

	};

	// 一个设备有一个，在op里创建
	class AppManager
	{
	public:
		AppManager ( )
		{
			for (int i = 0; i < g_kAppTypeCount; ++i)
			{
				App app = AppFactory::GetInstance()->GetAppFromType(static_cast<AppType>(i));
				// 全部安装
				apps_.push_back(std::make_shared<App>(app));
			}
		}

		void set_node_id (const Objid& id) 
		{
			node_id_ = id;
			for (int i = 0; i < apps_.size(); ++i)
			{
				apps_[i]->set_node_id(id);
			}
		}

		void set_node_data (std::shared_ptr<NodeData> p_node_data)
		{
			p_node_data_ = p_node_data;
		}

		std::shared_ptr<App> app (int index) { return apps_[index]; }

		// 这个设备上的全部component
		// 只要拷贝
		std::map<int, std::vector<Component>> Components ( )
		{
			std::map<int, std::vector<Component>> coms;
			int i = 0;
			for (auto p_app : apps_)
			{
				coms.insert({i++, p_app->CopyComponent()});
			}
			return coms;
		}

		void StartDevice ( )
		{
			// 运行其中的某些
			for (int i = 0; i < apps_.size(); ++i)
			{
				apps_[i]->Start();
			}
		}
		
		// 获取卸载信息
		void OnGetOffloadInfo (Ici* p_ici);
	
		bool OnGetOffloadStrategy (Ici* p_ici);

		float OnRemoved (Ici* p_ici);
	
	private:
		Objid node_id_;
		std::vector<std::shared_ptr<App>> apps_;
		std::shared_ptr<NodeData> p_node_data_;
	};
}

#endif
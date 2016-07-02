#ifndef _MCC_SIM_MNG_H_
#define _MCC_SIM_MNG_H_ 

#include "mcc_d_n.h"
#include "singleton.h"

namespace mcc
{

    // 二维向量，可表示位置、速度等
    class Vector2
    {
    public:
        Vector2 ( ) = default;
    	Vector2 (float x, float y) : x_(x), y_(y) { }
        Vector2 (const Vector2 &v) : x_(v.x()), y_(v.y()) { }

    	// 向量长度
    	float Magnitude ( )
    	{
    		return sqrt(pow(x_, 2) + pow(y_, 2));
    	}

    	// 返回单位向量
    	Vector2 &Normalize ( )
    	{  
            float magnitude = Magnitude();
            x_ /= magnitude;
            y_ /= magnitude;
            return *this;
    	}

        // 存取
        float x ( ) const { return x_; }
        float y ( ) const { return y_; }
        void set_x (float value) { x_ = value; }
        void set_y (float value) { y_ = value; }

        std::string ToString ( ) const
        {
            char temp[64];
            sprintf(temp, "%f %f", x_, y_);
            std::string str(temp);
            return str;
        }

        Vector2& operator+= (const Vector2 &v)
        {
            this->x_ += v.x();
            this->y_ += v.y();
            return *this;
        }

        Vector2& operator-= (const Vector2 &v)
        {
            this->x_ -= v.x();
            this->y_ -= v.y();
            return *this;
        }

        Vector2 operator+ (const Vector2 &v) const
        {
            return Vector2(this->x() + v.x(), this->y() + v.y());
        }

        Vector2 operator- (const Vector2 &v) const
        {
            return Vector2(x_ - v.x(), y_ - v.y());
        }

        Vector2 operator* (const float& f) const
        {
            return Vector2(this->x() * f, this->y() * f);
        }

        // 两点距离
        static float Distance (const Vector2 &v1, const Vector2 &v2) 
        {
            return sqrt(pow(v1.x() - v2.x(), 2) + pow(v1.y() - v2.y(), 2));
        }

        // 线性插值
        static Vector2 Lerp (const Vector2 &from, const Vector2 &to, float t);

        // 获取结点位置
        static Vector2 NodePosition (const Objid& node_id);

        // 节点间距离
        static float NodeDistance (const Objid& node_id1, const Objid& node_id2);

    private:
    	float x_;
    	float y_;
    };

    class IciSender
    {
    private:
        // static void SizeofIci (Ici* p_ici);
    public:
        // 发ici到本地本进程
        static void SendLoacl (Ici* p_ici, float delay, int intrpt_code);

        // 发ici到本地其他进程
        static void SendLocalProcess (Ici* p_ici, float delay, int intrpt_code, const char* proc_name);

        // 发ici到其他节点
        static void SendRemote (Ici* p_ici, float delay, int intrpt_code, const char* proc_name, Objid node_id);
    };

	class SimulationManager
	{
	public:
		static SimulationManager* GetInstance ( );

        // 添加某个端口的监听者
        void AddListener (int port, Objid node_id, std::string proc_name);

        // 某个结点取消监听
        void RemoveListener (int port, Objid node_id, std::string proc_name);

        // 收到某个中断后转发
        void Forward (Ici* p_ici);

        void AddNode (Objid id)
        {
            node_ids_.push_back(id);
        }

        const std::vector<Objid>& NodeRef ( ) const { return node_ids_; } 

	private:
        // 对某个端口的全部监听者及其处理进程
        std::map<int, std::set<std::pair<Objid, std::string>>> listeners_;

        // 参与仿真的全部结点
        std::vector<Objid> node_ids_;
		
	};

    class Utility
    {
    public:
        // 正正态分布
        static float PositiveNormDist (float e, float d);
            
        // 正正态分布期望、方差由e指数分布确定
        static float PositiveNormFromExp (float e, float d);

        // 返回一个随机的时延
        static float Delay (Objid from, Objid to);

        static float SINR (Objid tran_node_id, Objid recv_node_id, bool inter = true);

        // 模拟退火
        // 不需要评价函数，但需要得到改变解后的增量
        template <typename T, typename C>
        static T SimulatedAnnealed (T ans_sln, const C* p_obj, T (C::*new_sln_func)(const T&, float*) const)
        {
            float temp = g_kInitTemperature;
            std::uniform_real_distribution<> ru(0, 1);
            float delta_val = 0;
            while (temp >= 0)
            {
                T new_sln = (p_obj->*new_sln_func)(ans_sln, &delta_val);
                if (exp(delta_val / temp) >= ru(gen))
                {
                    ans_sln = new_sln;
                }
                temp -= g_kTemperatureDecrease;
                delta_val = 0;
            }
            return ans_sln;
        }
        
    };

    // 无参数，无返回值的委托
    class IDelegateHandler0
    {
    public:
        virtual void operator() ( ) = 0;
    };

    template <typename T>
    class DelegateHandler : public IDelegateHandler0
    {
    public:
        DelegateHandler (T* p_obj, void (T::*p_func)()):
        p_obj_  (p_obj),
        p_func_ (p_func)
        {}

        void operator() ( )
        {
            p_obj_->*p_func_();
        }

    private:
        void (T::*p_func_) ( );
        T* p_obj_;
    };

    class Delegate0
    {
    public:

        void operator+= (IDelegateHandler0* delegateHandler)
        {
            p_delegateHandlers_.push_back(delegateHandler);
        }

        void operator() ( ) 
        {
            for (auto& handler : p_delegateHandlers_)
            {
                (*handler)();
            }
        }

        void Clear ( )
        {
            p_delegateHandlers_.clear();
        }

    private:
        std::vector<IDelegateHandler0*> p_delegateHandlers_;
    };

    // 一个参数，无返回值的委托
    template <typename ARG>
    class IDelegateHandler
    {
    public:
        virtual void operator() (ARG arg) = 0;
    };

    template <typename T, typename ARG>
    class DelegateHandler1 : public IDelegateHandler<ARG>
    {
    public:
        DelegateHandler1 (T* p_obj, void (T::*p_func)(ARG arg)):
        p_obj_  (p_obj),
        p_func_ (p_func)
        {}

        void operator() (ARG arg)
        {
            p_obj->*p_func_(arg);
        }

    private:
        void (T::*p_func_)(ARG arg);
        T* p_obj_;
    };

    template <typename ARG>
    class Delegate1
    {
    public:

        void operator+= (IDelegateHandler<ARG>* delegateHandler)
        {
            p_delegateHandlers_.push_back(delegateHandler);
        }

        void operator() (ARG  arg) 
        {
            for (auto& p_handler : p_delegateHandlers_)
            {
                (*p_handler)(arg);
            }
        }

        void Clear ( )
        {
            p_delegateHandlers_.clear();
        }

    private:
        std::vector<IDelegateHandler<ARG>*> p_delegateHandlers_;
    };

    // 无返回值，两个参数
    template <typename ARG1, typename ARG2>
    class IDelegateHandler2
    {
    public:
        virtual void operator() (ARG1 arg1, ARG2 arg2) = 0;
    };

    template <typename T, typename ARG1, typename ARG2>
    class DelegateHandler2 : public IDelegateHandler2<ARG1, ARG2>
    {
    public:
        DelegateHandler2 (T* p_obj, void (T::*p_func)(ARG1 arg1, ARG2 arg2)):
        p_obj_  (p_obj),
        p_func_ (p_func)
        {}

        void operator() (ARG1 arg1, ARG2 arg2)
        {
            p_obj_->*p_func_(arg1, arg2);
        }

    private:
        void (T::*p_func_)(ARG1 arg1, ARG2 arg2);
        T* p_obj_;
    };

    template <typename ARG1, typename ARG2>
    class Delegate2
    {
    public:
        void operator+= (IDelegateHandler2<ARG1, ARG2>* delegateHandler)
        {
            p_delegateHandlers_->push_back(delegateHandler);
        }

        void operator() (ARG1 arg1, ARG2 arg2)
        {
            for (auto p_handler : p_delegateHandlers_)
            {
                (*p_handler)(arg1, arg2);
            }
        }

        void Clear ( )
        {
            p_delegateHandlers_.clear();
        }

    private:
        std::vector<IDelegateHandler2<ARG1, ARG2>*> p_delegateHandlers_;
    };

}

#endif	   
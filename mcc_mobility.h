#ifndef _MCC_MOBILITY_H_
#define _MCC_MOBILITY_H_ 

#include "mcc_d_n.h"
#include "mcc_timer.h"
#include "mcc_sim_mng.h"

namespace mcc
{

	class Mobility
	{
	public:
		Mobility (Objid node_id) :
		node_id_ (node_id)
		{}

		Vector2 cur_position ( ) const { return position_; }

		void StartMoving (Objid id);

		// 更新位置
		virtual void UpdatePosition ( ) = 0;

		virtual void Init ( ) = 0;

		// 动
		void Move ( );

	protected:
		Vector2 position_;
		Objid node_id_;
		
	};

	class RandomWayPoint : public Mobility
	{
	public:
		RandomWayPoint (Objid node_id, Vector2 cur_pos) : 
		Mobility  (node_id)
		{
			position_ = cur_pos;
			speed_dis_ = std::uniform_real_distribution<float>(10, 15);
			pause_dis_ = std::uniform_real_distribution<float>(1, 5);
			dest_x_dis_ = std::uniform_real_distribution<float>(g_kXMin, g_kXMax);
			dest_y_dis_ = std::uniform_real_distribution<float>(g_kYMin, g_kYMax);
		}

		void UpdatePosition ( );

		void Init ( );

	private:
		// 设置初始速度、方向
		Vector2 destination_;
		float speed_;
		float pause_;

		bool moving_ = true;

		std::uniform_real_distribution<float> speed_dis_;
		std::uniform_real_distribution<float> pause_dis_;
		std::uniform_real_distribution<float> dest_x_dis_;
		std::uniform_real_distribution<float> dest_y_dis_;
	};

	class RandomDircetion : Mobility
	{
	public:
		RandomDircetion (Objid node_id, Vector2 cur_pos) :
		Mobility (node_id)
		{
			position_ = cur_pos;
			speed_dis_ = std::uniform_real_distribution<float>(10, 15);
			pause_dis_ = std::uniform_real_distribution<float>(1, 5);
			dir_dis_ = std::uniform_real_distribution<float>(0, 360);
			duration_dis_ = std::uniform_real_distribution<float>(1, 10);
		}

		void UpdatePosition ( );

		void Init ( );

	private:
		Vector2 direction_;
		float speed_;
		float duration_;
		float pause_;

		bool moving_ = true;
		bool bounce_ = false;

		std::uniform_real_distribution<float> speed_dis_;
		std::uniform_real_distribution<float> duration_dis_;
		std::uniform_real_distribution<float> pause_dis_;
		std::uniform_real_distribution<float> dir_dis_;
	};

	class ReferencePointGroup : public Mobility
	{
	public:
		ReferencePointGroup (Objid node_id, Objid id, Vector2 cur_pos) :
		Mobility   (node_id),
		leader_id_ (id)
		{
			position_ = cur_pos;
			leader_position_ = Vector2::NodePosition(leader_id_);
		}

		void UpdatePosition ( );

		void Init ( );

	private:
		std::uniform_real_distribution<float> rag_dis_;
		std::uniform_real_distribution<float> r_dis_;

		Objid leader_id_;
		Vector2 leader_position_;
	};

	class SocialBased : public Mobility
	{
	public:
		SocialBased (Objid node_id, Vector2 cur_pos) : 
		Mobility (node_id)
		{
			position_ = cur_pos;
		}

		void UpdatePosition ( );

		void Init ( );

	private:

		// 选择栅格中的随机一点
		void _Destination (unsigned i, unsigned j);

		Vector2 destination_;
		float speed_;

		std::uniform_real_distribution<float> speed_dis_;
		
	};

	class SocialManager
	{
	friend Singleton<SocialManager>;

	public:
		static SocialManager* GetInstance ( );

		std::pair<unsigned, unsigned> ChooseSquare (Objid node_id);

	private:
		SocialManager ( );
		std::pair<unsigned, unsigned> _IndexFromObjid (Objid node_id);
		void _FreshM ( );

		std::vector< std::vector<float> > m_;
		std::map<Objid, unsigned> indecies_;	// objid对应的标号
		std::vector<Objid> node_ids_;

		unsigned size_;		// 栅格个数

	};

}

#endif

#include "mcc_mobility.h"

using namespace mcc;

void Mobility::Move ( )
{
	op_ima_obj_attr_set(node_id_, "x", position_.x());
	op_ima_obj_attr_set(node_id_, "y", position_.y());
}

void Mobility::StartMoving (Objid id)
{
	node_id_ = op_topo_parent(id);
	Time::GetInstance()->AddTimer(g_kMove, id, 1);
}

void RandomWayPoint::Init ( )
{
	// 目标地点
	float x = dest_x_dis_(gen);
	float y = dest_y_dis_(gen);
	destination_ = Vector2(x, y);

	// 初始速度
	speed_ = speed_dis_(gen);
}

void RandomWayPoint::UpdatePosition ( )
{
	float delta = Time::GetInstance()->delta_time();
	if (moving_)
	{
		Vector2 direction = (destination_ - position_).Normalize();
		position_ += direction * speed_ * delta;
		float dis = Vector2::Distance(position_, destination_);
		if (dis < 0.1)
		{
			moving_ = false;
			pause_ = pause_dis_(gen);
		}
	}
	else
	{
		pause_ -= delta;
		if (pause_ <= 0)
		{
			moving_ = true;
			Init();
		}
	}
}

void RandomDircetion::Init ( )
{

	// 方向
	float angle = dir_dis_(gen);
	float x = cos(angle * g_kDeg2Rad);
	float y = sin(angle * g_kDeg2Rad);
	direction_ = Vector2(x, y);
	direction_.Normalize();

	// 初始速度
	speed_ = speed_dis_(gen);

	// 移动时间
	duration_ = duration_dis_(gen);
}

void RandomDircetion::UpdatePosition ( )
{
	float delta = Time::GetInstance()->delta_time();
	if (moving_)
	{
		position_ += direction_ * speed_ * delta;
		duration_ -= delta;
		// 累了
		if (duration_ <= 0)
		{
			moving_ = false;
			pause_ = pause_dis_(gen);
		}

		// 撞墙了？
		if (bounce_)
		{
			if (position_.x() > g_kXMax || position_.x() < g_kXMin)		
			{
				position_.set_x(-position_.x());
			}
			if (position_.y() > g_kYMax || position_.y() < g_kYMin)
			{
				position_.set_y(-position_.y());
			}
		}
	}
	else
	{
		pause_ -= delta;
		if (pause_ <= 0)
		{
			moving_ = true;
			Init();
		}
	}
}

void ReferencePointGroup::Init ( )
{
	// 随机的角度和距离
	rag_dis_ = std::uniform_real_distribution<float>(0, 2 * 3.1415926f);
	r_dis_ = std::uniform_real_distribution<float>(30, 50);
}

void ReferencePointGroup::UpdatePosition ( )
{
	float rag = rag_dis_(gen);
	Vector2 dir(cos(rag), sin(rag));
	float rad = r_dis_(gen);
	float delta = Time::GetInstance()->delta_time();
	Vector2 cur_leader_position = Vector2::NodePosition(leader_id_);
	// Vi = Vl + M * R
	position_ += cur_leader_position - leader_position_ +  dir * rad * delta;
	leader_position_ = cur_leader_position;
}

void SocialBased::Init ( )
{
	// 目标栅格
	auto index = SocialManager::GetInstance()->ChooseSquare(node_id_);
	// 栅格中的一点;
	_Destination(index.first, index.second);

	speed_dis_ = std::uniform_real_distribution<float>(5, 15);
	speed_ = speed_dis_(gen);
}

void SocialBased::UpdatePosition ( )
{
	float dis = Vector2::Distance(position_, destination_);
	if (dis > 1)
	{
		// 没到
		Vector2 dir = (destination_ - position_).Normalize();
		position_ += dir * speed_ * Time::GetInstance()->delta_time();
	}
	else
	{
		// 到了重新选目的地
		// 目标栅格
		auto index = SocialManager::GetInstance()->ChooseSquare(node_id_);
		// 栅格中的一点;
		_Destination(index.first, index.second);
	}
}

void SocialBased::_Destination (unsigned i, unsigned j)
{
	float x = i * g_kSquareSize + g_kXMin;
	x = min(x, g_kXMax - g_kSquareSize);
	float y = j * g_kSquareSize + g_kYMin;
	y = min(y, g_kYMax - g_kSquareSize);

	std::uniform_real_distribution<float> x_dis(x, x + g_kSquareSize);
	std::uniform_real_distribution<float> y_dis(y, y + g_kSquareSize);
	destination_ = Vector2(x_dis(gen), y_dis(gen));
}

SocialManager* SocialManager::GetInstance ( )
{
	return Singleton<SocialManager>::GetInstance();
}

SocialManager::SocialManager ( )
{

	size_ = (g_kXMax - g_kXMin) / g_kSquareSize * ((g_kYMax - g_kYMin) / g_kSquareSize);

	int l = 0;
	// 获取全部node，注意node命名<node_i>
	while (true)
	{
		char name[128];
		sprintf(name, "node_%d", l);
		Objid node_id = op_id_from_name(0, OPC_OBJTYPE_NODE_FIX, name);
		if (node_id == OPC_OBJID_INVALID)
			break;
		else
		{
			node_ids_.push_back(node_id);
			indecies_[node_id] = l++;
		}
	}

	// M矩阵
	_FreshM();
}

std::pair<unsigned, unsigned> SocialManager::ChooseSquare (Objid id)
{
	std::vector<float> attrs(size_);
	std::vector<unsigned> count(size_);

	// 横向栅格个数
	unsigned width = (g_kXMax - g_kXMin) / g_kSquareSize;
	for (auto node_id : node_ids_)
	{
		auto index = _IndexFromObjid(node_id);
		float attr = m_[indecies_[node_id]][indecies_[id]];
		attrs[index.first + width * index.second] += attr;
		count[index.first + width * index.second] += 1;
	}

	// 最具吸引力的栅格
	float max_attr = 0;
	unsigned x = 0;
	for (int i = 0; i < size_; ++i)
	{
		float attr = 0;
		if (count[i] != 0)
		{
			attr = attrs[i] / (float)count[i];
		}
		if (attr > max_attr)
		{
			max_attr = attr;
			x = i;
		}
	}

	// 定期刷新M
	if ((int)(Time::GetInstance()->cur_time() / Time::GetInstance()->delta_time()) % 2 == 0)
		_FreshM();

	return std::make_pair(x / width, x % width);

}

std::pair<unsigned, unsigned> SocialManager::_IndexFromObjid (Objid id)
{
	Vector2 pos = Vector2::NodePosition(id);
	unsigned i = (unsigned)((pos.x() - g_kXMin) / g_kSquareSize);
	unsigned j = (unsigned)((pos.y() - g_kYMin) / g_kSquareSize);
	return std::make_pair(i, j);
}

void SocialManager::_FreshM ( )
{
	int l = node_ids_.size();
	std::uniform_real_distribution<float> m_dis(0, 1);
	m_ = std::vector< std::vector<float> >(l, std::vector<float>(l));
	for (int i = 0; i < l; ++i)
	{
		for (int j = i; j < l; ++j)
		{
			if (i == j)
			{
				m_[i][j] = 0;
			}
			else
			{
				m_[i][j] = m_dis(gen);
			}
		}
	}

	for (int i = 1; i < l; ++i)
	{
		for (int j = 0; j < i; ++j)
		{
			m_[i][j] = m_[j][i];
		}
	}
}
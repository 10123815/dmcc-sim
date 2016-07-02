#ifndef _SINGLETON_H_
#define _SINGLETON_H_ 

#include <memory>

template <typename T>
class Singleton
{

private:
	Singleton(){}
	~Singleton(){}
	
    static std::unique_ptr<T> instance_;

public:
	inline static T* GetInstance();

};

template <typename T>
std::unique_ptr<T> Singleton<T>::instance_;

template <typename T>
T* Singleton<T>::GetInstance()
{
	if (instance_.get() == 0)
	{
		instance_.reset(new T());
	}
	return instance_.get();
}

#endif
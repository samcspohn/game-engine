#include "Transform.h"

class component
{
	// friend game_object;

public:
	virtual void onStart();
	virtual void onDestroy();

	static bool _registerEngineComponent();
	// virtual void onCollision(game_object *go, glm::vec3 point, glm::vec3 normal);
	virtual void update();
	virtual void lateUpdate();
	virtual void init();
	virtual void deinit();

	// virtual void onEdit() = 0;
	// virtual void _copy(game_object *go) = 0;
	transform2 transform;
	int getThreadID();
	size_t getHash();

	SER_HELPER()
	{
		ar;
		// ar &transform;
	}
};
REGISTER_BASE(component)
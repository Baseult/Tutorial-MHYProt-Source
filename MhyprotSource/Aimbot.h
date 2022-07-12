#pragma once
#include "Overlay.h"
#include "Vector3.h"

class CEntity;

struct TargetInfo
{
	TargetInfo()
	{
		this->entity = 0;
		this->screenPosition = Vector3(0, 0, 0);
		this->screenPositionNoPredict = Vector3(0, 0, 0);
		this->velocity = Vector3(0, 0, 0);
		this->isInScreen = false;
		this->Position = Vector3(0, 0, 0);
	}

	Vector3 screenPosition;
	Vector3 screenPositionNoPredict;
	Vector3 Position;
	Vector3 velocity;
	uint64_t entity;
	bool isInScreen;
};

class Aimbot
{
	static void on_update();
public:
	static Vector3 predict_impact_pos(const Vector3& source, const Vector3& destination, const Vector3& target_velocity, Vector3& impact_coords, double& hittimetokill, double& hitvelocity, double& hitdistance);
	static void initialize();

	static TargetInfo current_target;
	static bool is_lock;

	static TargetInfo get_best_target();
	static TargetInfo get_target_info(uint64_t entity);
};

#pragma once
#include "../../ModuleBase/Module.h"
class KillauraFOX : public Module {
private:
private:
public:
	Vec2<float> rotAngle;
	int delayFE = 0;
	int delayFEO = 0;
	int packetsFE = 0;
	int range = 0;
	bool mobs = false;
	bool multi = true;
	int slotMode = 0;
	KillauraFOX();
	void onDisable() override;
	void onEnable() override;
	void onNormalTick(LocalPlayer* localPlayer) override;
	void onUpdateRotation(LocalPlayer* localPlayer) override;
	void onSendPacket(Packet* packet) override;
};
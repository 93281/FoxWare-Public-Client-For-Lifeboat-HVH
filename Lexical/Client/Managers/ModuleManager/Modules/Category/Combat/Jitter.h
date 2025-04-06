#pragma once
#include "../../ModuleBase/Module.h"

class Jitter : public Module {
private:
	int mode = 0;
public:
	Jitter();
	std::string getModeText() override;
	void onSendPacket(Packet* packet) override;
};
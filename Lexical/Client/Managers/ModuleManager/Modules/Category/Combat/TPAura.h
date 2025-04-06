#pragma once
#include "../../ModuleBase/Module.h"

class TPAura : public Module {
private:
    int range = 50;
    int yOffset = 0;
    Actor* currentTarget = nullptr;
    Vec3<float> targetPos;
    float lerpSpeed = 0.4;
    int multiplier = 1;
public:
    TPAura();
    static bool sortByDist(Actor* a1, Actor* a2);
    virtual void onEnable() override;
    void onDisable() override;
    virtual void onNormalTick(LocalPlayer* actor) override;
    virtual void onUpdateRotation(LocalPlayer* localPlayer) override;
    void onSendPacket(Packet* packet) override;
};
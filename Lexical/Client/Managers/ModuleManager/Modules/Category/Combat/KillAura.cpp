#include "Killaura.h"
#include "../../../../../Client.h"
#include <algorithm>
#include <chrono>

float hookminrange;
int targeting = 0;
std::chrono::time_point<std::chrono::steady_clock> lastAttackTime;
int attackCounter = 0;
float accumulatedTime = 0.f;
bool instantrotate;
Killaura::Killaura() : Module("Killaura-CAT", "attacks entities around you", Category::COMBAT) {
    registerSetting(new BoolSetting("Mobs", "attack mobs", &mobs, false));
    registerSetting(new SliderSetting<float>("Range", "range in which targets will be hit", &range, 3.5f, 1.f, 50.f));
    registerSetting(new SliderSetting<float>("Hook Min Range", "range in which targets will be hit", &hookminrange, 3.5f, 1.f, 15.f));
    registerSetting(new BoolSetting("useHook", "rotate", &useHookspeed, true));
    registerSetting(new SliderSetting<float>("HookSpeed", "edits rotate and fly speed based off factor", &hookspeedH, 3.5f, 1.f, 50.f));
    registerSetting(new SliderSetting<int>("CPS", "clicks per second", &delayFE, 15, 1, 50));
    registerSetting(new BoolSetting("Rotations", "rotate", &rotate, true));
    registerSetting(new BoolSetting("Instant Rotate", "rotate", &instantrotate, true));
    registerSetting(new EnumSetting("Target", "Target filter", { "Distance", "Health" }, &targeting, 0));
}

void Killaura::onDisable() {
    attackCounter = 0;
    accumulatedTime = 0.f;
    arewehooking = false;
}

void Killaura::onEnable() {
    attackCounter = 0;
    accumulatedTime = 0.f;
    arewehooking = false;
    lastAttackTime = std::chrono::steady_clock::now();
}

Vec2<float> targetRot;

void Killaura::onNormalTick(LocalPlayer* localPlayer) {
    if (!localPlayer) return;
    auto now = std::chrono::steady_clock::now();
    float deltaTime = std::chrono::duration<float>(now - lastAttackTime).count();
    lastAttackTime = now;
    accumulatedTime += deltaTime;
    float expectedAttacks = delayFE * accumulatedTime;
    int attacksToDo = static_cast<int>(expectedAttacks);
    if (attacksToDo > 0) {
        accumulatedTime -= attacksToDo / static_cast<float>(delayFE);

        std::vector<Actor*> validTargets;
        for (Actor* ent : localPlayer->getlevel()->getRuntimeActorList()) {
            if (!TargetUtil::isTargetValid(ent, mobs)) continue;
            if (localPlayer->getPos().dist(ent->getPos()) > range) continue;
            validTargets.push_back(ent);
        }

        if (targeting == 0) {
            std::sort(validTargets.begin(), validTargets.end(), [localPlayer](Actor* a, Actor* b) {
                return localPlayer->getPos().dist(a->getPos()) < localPlayer->getPos().dist(b->getPos());
                });
        }
        else if (targeting == 1) {
            std::sort(validTargets.begin(), validTargets.end(), [](Actor* a, Actor* b) {
                return a->getHealth() > b->getHealth();
                });
        }
        if (!validTargets.empty()) {
            Actor* target = validTargets.front();
            for (int i = 0; i < attacksToDo; i++) {
                localPlayer->swing();
                localPlayer->getgameMode()->attack(target);
                attackCounter++;
            }

            targetRot = localPlayer->getEyePos().CalcAngle(target->getEyePos());
        }
    }
}

void Killaura::onUpdateRotation(LocalPlayer* localPlayer) {
    if (!rotate || !localPlayer) return;
    BlockSource* region = Game::clientInstance->getRegion();
    Level* level = localPlayer->level;
    bool foundTarget = false;
    std::vector<Actor*> validTargets;

    for (Actor* ent : localPlayer->getlevel()->getRuntimeActorList()) {
        if (!TargetUtil::isTargetValid(ent, mobs)) continue;
        float distance = localPlayer->getPos().dist(ent->getPos());
        if (distance > range) continue;
        validTargets.push_back(ent);
    }

    if (targeting == 0) {
        std::sort(validTargets.begin(), validTargets.end(), [localPlayer](Actor* a, Actor* b) {
            return localPlayer->getPos().dist(a->getPos()) < localPlayer->getPos().dist(b->getPos());
            });
    }
    else if (targeting == 1) {
        std::sort(validTargets.begin(), validTargets.end(), [](Actor* a, Actor* b) {
            return a->getHealth() > b->getHealth();
            });
    }
    if (!validTargets.empty()) {
        Actor* target = validTargets.front();
        float distance = localPlayer->getPos().dist(target->getPos());
        float targetYaw = targetRot.y;
        if (instantrotate) {
            targetRot = localPlayer->getEyePos().CalcAngle(validTargets[0]->getEyePos());
        }
        if (auto* headRot = localPlayer->getActorHeadRotationComponent()) {
            headRot->headYaw = targetYaw;
        }
        if (auto* bodyRot = localPlayer->getMobBodyRotationComponent()) {
            bodyRot->bodyYaw = targetYaw;
        }

        localPlayer->rotation->presentRot = targetRot;

        if (distance < hookminrange && useHookspeed)
            arewehooking = true;
        else
            arewehooking = false;

        foundTarget = true;
    }

    if (!foundTarget) {
        arewehooking = false;
    }
}

void Killaura::onSendPacket(Packet* packet) {
}
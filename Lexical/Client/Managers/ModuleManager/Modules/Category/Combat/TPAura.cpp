#include "TPAura.h"
static bool teleporting = false;
bool attackEnabled = false;
int xPos = 0.0f;
std::vector<Actor*> targetListJ;
bool includeMobs = false;
int speedModif = 0;
int swap = 1;
bool pargetattack = false;
bool hookH = false;
TPAura::TPAura() : Module("Magnet", "smooth teleport to enemies.", Category::COMBAT) {
    registerSetting(new SliderSetting<int>("hit range", "maximum distance to hit targets", &range, 1, 1, 35));
    registerSetting(new SliderSetting<float>("speed", "smoothing factor for teleportation", &lerpSpeed, 0.f, 0.f, 20.f));
    registerSetting(new BoolSetting("enable attack", "toggle attacking enemies", &attackEnabled, false));
    registerSetting(new BoolSetting("packet attack", "toggle pacgetting enemies", &pargetattack, false));
    registerSetting(new BoolSetting("mobs", "toggle attacking mobs", &includeMobs, false));
    registerSetting(new BoolSetting("hook", "amazing on ign", &hookH, false));
    registerSetting(new SliderSetting<int>("attack multiplier", "adjust attack speed", &multiplier, 1, 1, 10));
    registerSetting(new SliderSetting<int>("vertical offset", "adjust y position", &yOffset, 0, -10, 10));
    registerSetting(new SliderSetting<int>("horizontal offset", "adjust x and z position", &xPos, 0, -30, 30));
    registerSetting(new SliderSetting<int>("circle speed", "adjust circle speed", &speedModif, 1, 0, 25));
    registerSetting(new EnumSetting("weapon", "auto switch to best weapon", { "none", "switch", "spoof" }, &swap, 0));
}
void TPAura::onEnable() {
}
void TPAura::onDisable() {
    targetListJ.clear();
}
int getBestWeaponSlot2(Actor* target) {
    LocalPlayer* localPlayer = Game::getLocalPlayer();
    Container* inventory = localPlayer->playerInventory->container;
    float damage = 0.f;
    int slot = localPlayer->playerInventory->selectedSlot;
    for (int i = 0; i < 9; i++) {
        localPlayer->playerInventory->selectedSlot = i;
        float currentDamage = localPlayer->calculateAttackDamage(target);
        if (currentDamage > damage) {
            damage = currentDamage;
            slot = i;
        }
    }

    return slot;
}
bool TPAura::sortByDist(Actor* a1, Actor* a2) {
    Vec3<float> lpPos = Game::getLocalPlayer()->getPos();
    return ((a1->getPos().dist(lpPos)) < (a2->getPos().dist(lpPos)));
}
#include <iostream> 
Vec2 <float> nuvolarot1;
void TPAura::onNormalTick(LocalPlayer* localPlayer) {
    static float tickCount = 0;
    tickCount += speedModif;
    int oldSlot = localPlayer->playerInventory->selectedSlot;
    Level* level = localPlayer->level;
    BlockSource* region = Game::clientInstance->getRegion();
    GameMode* gm = localPlayer->gameMode;
    targetListJ.clear();
    for (auto& entity : level->getRuntimeActorList()) {
        if (TargetUtil::isTargetValid(entity, includeMobs)) {
            float rangeCheck = range;
            if (region->getSeenPercent(localPlayer->getEyePos(), entity->aabbShape->aabb) == 0.0f)
                rangeCheck = range;

            if (WorldUtil::distanceToEntity(localPlayer->getPos(), entity) <= rangeCheck)
                targetListJ.push_back(entity);
        }
    }
    if (targetListJ.empty()) {
        return;
    }
    if (swap == 2 || swap == 1) {
        localPlayer->playerInventory->selectedSlot = getBestWeaponSlot2(targetListJ[0]);
    }
    std::sort(targetListJ.begin(), targetListJ.end(), TPAura::sortByDist);
    Vec3<float> targetPos = targetListJ[0]->getPos();
    targetPos.y += yOffset;
    targetPos.x += xPos * std::cos(tickCount);
    targetPos.z += xPos * std::sin(tickCount);
    if (!teleporting) {
        Vec3<float> currentPos = localPlayer->getPos();
        Vec3<float> direction = {
            targetPos.x - currentPos.x,
            targetPos.y - currentPos.y,
            targetPos.z - currentPos.z
        };
        float distance = sqrtf(
            direction.x * direction.x +
            direction.y * direction.y +
            direction.z * direction.z
        );
        float Hookspeed = 1.f;
        if (hookH) {
            if (WorldUtil::distanceToEntity(localPlayer->getPos(), targetListJ[0]) <= 0.5) {
                Hookspeed = 4.f;
            }
            else {
                Hookspeed = 1.f;
            }
           // std::cout << "Hook speed: " << Hookspeed << std::endl;
        }
        if (distance > 0.1f) { 
            if (distance > 0) {
                direction.x /= distance;
                direction.y /= distance;
                direction.z /= distance;
            }
            float moveDistance = std::min(distance, lerpSpeed * Hookspeed);
            Vec3<float> newPos = {
                currentPos.x + direction.x * moveDistance,
                currentPos.y + direction.y * moveDistance,
                currentPos.z + direction.z * moveDistance
            };
            localPlayer->setPos(newPos);
        }
        teleporting = false;
    }
    if (attackEnabled) {
        localPlayer->swing();
        localPlayer->getgameMode()->attack(targetListJ[0]);
    }
    if (swap == 2) {
        localPlayer->playerInventory->selectedSlot = oldSlot;
    }
}
void TPAura::onUpdateRotation(LocalPlayer* localPlayer) { /* --- Playertick? --- */
    if (attackEnabled) {
        if (targetListJ.empty()) {
            return;

        }
        for (int j = 0; j < multiplier; ++j) {
            Vec3<float> aimPos = targetListJ[0]->getEyePos();
            aimPos.y = targetListJ[0]->aabbShape->aabb.getCenter().y;
            nuvolarot1 = localPlayer->getEyePos().CalcAngle(aimPos);
            nuvolarot1.x = round(nuvolarot1.x / 45.f) * 45.f;
            nuvolarot1.y = round(nuvolarot1.y / 5.f) * 5.f;
            ActorRotationComponent* rotation = localPlayer->rotation;
            ActorHeadRotationComponent* headRot = localPlayer->getActorHeadRotationComponent();
            MobBodyRotationComponent* bodyRot = localPlayer->getMobBodyRotationComponent();
            rotation->presentRot.x = nuvolarot1.x; //normal
            rotation->presentRot.y = nuvolarot1.y;
            headRot->headYaw = nuvolarot1.y;
            bodyRot->bodyYaw = nuvolarot1.y;
        }
    }
}
void TPAura::onSendPacket(Packet* packet) {
    if (auto localPlayer = Game::getLocalPlayer(); !localPlayer) return;
    const auto packetId = packet->getId();
    if (packetId == PacketID::PlayerAuthInput) {
        PlayerAuthInputPacket* paip = static_cast<PlayerAuthInputPacket*>(packet);
        InteractPacket* interact = static_cast<InteractPacket*>(packet);
        if (!pargetattack) return;
        if (targetListJ.empty()) return;
        auto* target = targetListJ[0];
        const InteractAction action = InteractAction::LEFT_CLICK;
        interact->mAction = action;
        interact->mTargetId = target->getActorTypeComponent()->id;
        interact->mPos = Game::getLocalPlayer()->getPos();
    }
}
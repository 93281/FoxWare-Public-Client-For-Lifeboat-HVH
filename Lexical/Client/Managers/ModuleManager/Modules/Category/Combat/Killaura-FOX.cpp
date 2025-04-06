#include "Killaura-FOX.h"
#include "../../../../../Client.h"
#include "../../../../../../Utils/Minecraft/Intenvoru.h"
#include <iostream>  
/* BASICALLY PACKETAURA */
KillauraFOX::KillauraFOX() : Module("KillauraFOX", "attacks entities around you using packets", Category::COMBAT) {
    registerSetting(new EnumSetting("switch", "automatically switches to best weapon", { "none", "packet" }, &slotMode, 0));
    registerSetting(new SliderSetting<int>("delay", "the interval", &delayFE, delayFE, 0, 20));
    registerSetting(new SliderSetting<int>("range", "the interval", &range, range, 0, 20));
    registerSetting(new BoolSetting("multi", "multi attack", &multi, false));
    registerSetting(new BoolSetting("mobs", "attack mobs", &mobs, false));
}
void KillauraFOX::onDisable() {
    delayFE = 0;
}

void KillauraFOX::onEnable() {
    delayFE = 0;
}
int getBestWeaponSlotK(Actor* target) {
    LocalPlayer* localPlayer = Game::getLocalPlayer();
    if (!localPlayer) return -1;

    Container* inventory = localPlayer->playerInventory->container;
    float maxDamage = 0.f;
    int bestSlot = localPlayer->playerInventory->selectedSlot;

    for (int i = 0; i < 9; i++) {
        float currentDamage = localPlayer->calculateAttackDamage(target);
        if (currentDamage > maxDamage) {
            maxDamage = currentDamage;
            bestSlot = i;
        }
    }
    return bestSlot;
}


void KillauraFOX::onNormalTick(LocalPlayer* localPlayer) {
    LocalPlayer* player = Game::getLocalPlayer();
    if (!player) return;
    auto& inventory = *player->playerInventory;
    for (Actor* ent : player->getlevel()->getRuntimeActorList()) {
        if (!TargetUtil::isTargetValid(ent, mobs)) continue;
        if (player->getPos().dist(ent->getPos()) > range) continue;
        const int oldSlot = inventory.selectedSlot;
        if (slotMode == 1) inventory.selectedSlot = getBestWeaponSlotK(ent);
        if (slotMode == 1) inventory.selectedSlot = oldSlot;
        break;
    }
}

void KillauraFOX::onUpdateRotation(LocalPlayer* localPlayer) {
}

void KillauraFOX::onSendPacket(Packet* packet) {
    LocalPlayer* player = Game::getLocalPlayer();
    if (!player) return;
    auto& inventory = *player->playerInventory;
    if (delayFEO < delayFE) {
        delayFEO++;
        return;
    }
    delayFEO = 0;
    if (packet->getId() == PacketID::Interact || packet->getId() == PacketID::PlayerAuthInput) {
        int hitCount = 0;
        for (Actor* ent : player->getlevel()->getRuntimeActorList()) {
            if (!TargetUtil::isTargetValid(ent, mobs)) continue;
            if (player->getPos().dist(ent->getPos()) > range) continue;
            if (packet->getId() == PacketID::Interact) {
                InteractPacket* interact = static_cast<InteractPacket*>(packet);
                interact->mAction = InteractAction::LEFT_CLICK;
                interact->mTargetId = ent->getActorTypeComponent()->id;
                interact->mPos = ent->getPos();
            }
            if (packet->getId() == PacketID::PlayerAuthInput) {
                PlayerAuthInputPacket* paip = static_cast<PlayerAuthInputPacket*>(packet);
                Vec2<float> rotAngle = player->getEyePos().CalcAngle(ent->getEyePos());
                paip->rotation.y = rotAngle.y;
                paip->rotation.x = rotAngle.x;
                paip->mInputData |= InputData::PerformItemInteraction;
                const int oldSlot = inventory.selectedSlot;
                if (slotMode == 1) inventory.selectedSlot = getBestWeaponSlotK(ent);
                player->getgameMode()->attack(ent);
                if (slotMode == 1) inventory.selectedSlot = oldSlot;
            }
            hitCount++;
            if (!multi || hitCount >= packetsFE) break;
        }
    }
}

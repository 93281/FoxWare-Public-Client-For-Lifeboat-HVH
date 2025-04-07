#include "AutoCrystal.h"
#include "../../../../../../Utils/Minecraft/Intenvoru.h"
bool smartbreak = false;
bool packetbreak1 = false;
bool mobsJS;
AutoCrystal::AutoCrystal() : Module("CrystalAura", "automatically breaks and places crystal", Category::COMBAT) {
	registerSetting(new BoolSetting("place", "place end crystals at target", &place, true));
	registerSetting(new BoolSetting("break", "explode end crystals at target", &explode, true));
	registerSetting(new BoolSetting("multitask", "multitasks like eating and crystalling", &multiTask, true));
	registerSetting(new BoolSetting("safety", "prioritizes safety over damage", &safety, true));
	registerSetting(new BoolSetting("java", "for java servers", &java, false));
	registerSetting(new SliderSetting<int>("placedist", "range for placing crystals", &placeRange, 5, 1, 12));
	registerSetting(new SliderSetting<int>("breakdist", "range for breaking crystals", &breakRange, 5, 1, 12));
	registerSetting(new SliderSetting<int>("targetdist", "range for targeting entities", &targetRange, 10, 1, 20));
	registerSetting(new SliderSetting<float>("proximity", "proximity for crystal placement", &proximity, 6.f, 1.f, 12.f));
	registerSetting(new SliderSetting<float>("enemydmg", "minimum damage to enemy", &enemyDamage, 8.f, 0.f, 36.f));
	registerSetting(new SliderSetting<float>("selfdmg", "maximum damage to self", &localDamage, 4.f, 0.f, 50.f));
	registerSetting(new SliderSetting<int>("wasteamount", "number of crystals to place", &wasteAmount, 3, 1, 10));
	registerSetting(new BoolSetting("rotate", "rotate to placement locations", &rotate, true));
	registerSetting(new SliderSetting<int>("placespeed", "speed of placing crystals", &placeSpeed, 10, 0, 20));
	registerSetting(new SliderSetting<int>("breakspeed", "speed of breaking crystals", &breakSpeed, 10, 0, 20));
	registerSetting(new SliderSetting<int>("boostspeed", "speed of id prediction", &predictSpeed, 10, 0, 20));
	registerSetting(new BoolSetting("boost", "predict crystal runtime id for faster actions", &predict, false));
	registerSetting(new SliderSetting<int>("packets", "number of packets for prediction", &predictPacket, 5, 1, 10));
	registerSetting(new SliderSetting<float>("boostdamage", "minimum damage for boosting", &boostDmg, 10.f, 0.f, 20.f));
	registerSetting(new BoolSetting("swap", "swap to end crystal", &swap, true));
	registerSetting(new BoolSetting("switchback", "switch back to previous slot", &switchBack, true));
	registerSetting(new EnumSetting("render", "rendering mode for placements", { "off", "box", "flat" }, &renderType, 0));
	registerSetting(new ColorSetting("color", "render color", &renderColor, { 255, 0, 0 }));
	registerSetting(new BoolSetting("renderdamage", "display damage dealt during render", &dmgText, true));
	registerSetting(new BoolSetting("selftest", "enable testing on yourself", &selfTest, false));
	registerSetting(new BoolSetting("mobs", "enable testing on yourself", &mobsJS, false));
	registerSetting(new BoolSetting("smart break", "break checks", &smartbreak, false));
	registerSetting(new BoolSetting("packet break", "break checks", &packetbreak1, false));
}

bool AutoCrystal::sortCrystal(CrystalData c1, CrystalData c2) {
	return c1.targetDamage > c2.targetDamage;
}
float calculateDamage(const BlockPos& crystalPos, Actor* target) { //this is basic but works
	Vec3<float> crystalPosFloat(static_cast<float>(crystalPos.x) + 0.5f,
		static_cast<float>(crystalPos.y) + 1.0f,
		static_cast<float>(crystalPos.z) + 0.5f);
	Vec3<float> targetPos = target->getHumanPos();
	float distance = crystalPosFloat.dist(targetPos);
	distance = std::max(0.0f, std::min(distance, 12.0f));
	float damage = (12.0f * (1.0f - (distance / 6.0f)));
	return std::max(0.0f, damage);
}

bool AutoCrystal::isPlaceValid(const BlockPos& blockPos, Actor* actor) {
	BlockLegacy* block = Game::clientInstance->getRegion()->getBlock(blockPos)->blockLegacy;
	if (!(block->blockId == 7 || block->blockId == 49))
		return false;
	Vec3<float> blockPosFloat(blockPos.x, blockPos.y, blockPos.z);
	if (Game::getLocalPlayer()->getEyePos().dist(actor->getEyePos()) > placeRange)
		return false;
	Vec3<float> lower(blockPos.x, blockPos.y, blockPos.z);
	Vec3<float> upper(blockPos.x + 1, blockPos.y + 2, blockPos.z + 1);
	const AABB blockAABB(lower, upper);

	for (Actor* entity : entityList) {
		if (entity->getActorTypeComponent()->id == 71) continue; // Skip crystals
		AABB entityAABB = entity->aabbShape->aabb;
		if (entity->getActorTypeComponent()->id == 319 || entity == Game::getLocalPlayer()) {
			Vec3<float> pos = entity->stateVector->pos;
			entityAABB.lower = pos.sub(Vec3<float>(0.3f, 0.0f, 0.3f));
			entityAABB.upper = pos.add(Vec3<float>(0.3f, 1.8f, 0.3f));
			if (!java) entityAABB = entityAABB.expand(0.1f);
		}

		if (entityAABB.intersects(blockAABB)) return false;
	}

	return true;
}
void AutoCrystal::generatePlacement(Actor* actor) {
	placeList.clear();

	const Vec3 targetPos = actor->getHumanPos();
	const BlockPos center((int)targetPos.x, (int)targetPos.y, (int)targetPos.z);
	int radius = (int)proximity;
	if (actor->getHealth() < 10) radius = std::min(radius, 3);

	for (int x = -radius; x <= radius; x++) {
		for (int y = -2; y <= 2; y++) {
			for (int z = -radius; z <= radius; z++) {
				BlockPos blockPos = BlockPos(center.x + x, center.y + y, center.z + z);
				Vec3<float> blockPosFloat(blockPos.x + 0.5f, blockPos.y + 1.0f, blockPos.z + 0.5f);
				float distance = blockPosFloat.dist(targetPos);
				if (distance > placeRange || distance < 1.0f) continue;

				if (isPlaceValid(blockPos, actor)) {
					CrystalPlace placement(actor, blockPos);
					placement.targetDamage = calculateDamage(blockPos, actor);
					placement.localDamage = calculateDamage(blockPos, Game::getLocalPlayer());

					if (placement.targetDamage >= enemyDamage &&
						(!safety || placement.localDamage <= localDamage)) {
						placeList.emplace_back(placement);
					}
				}
			}
		}
	}
	std::sort(placeList.begin(), placeList.end(), [&](const CrystalPlace& a, const CrystalPlace& b) {
		if (a.targetDamage != b.targetDamage) return a.targetDamage > b.targetDamage;
		float distA = Vec3<float>(a.blockPos.x + 0.5f, a.blockPos.y + 1.0f, a.blockPos.z + 0.5f).dist(targetPos);
		float distB = Vec3<float>(b.blockPos.x + 0.5f, b.blockPos.y + 1.0f, b.blockPos.z + 0.5f).dist(targetPos);
		if (distA != distB) return distA < distB;
		return true;
		});
}
void AutoCrystal::getCrystals(Actor* actor) {
	for (Actor* entity : entityList) {
		if (entity == nullptr) continue;
		if (entity->getActorTypeComponent()->id != 71) continue;
		if (entity->getEyePos().dist(Game::getLocalPlayer()->getEyePos()) > breakRange) continue;
		const CrystalBreak breakment(actor, entity); // breakment?? :swej:
		if (breakment.targetDamage >= enemyDamage) {
			if (safety && breakment.localDamage <= localDamage) {
				highestId = entity->getRuntimeIDComponent()->runtimeId.id;
				breakList.emplace_back(breakment);
			}
			else if (!safety) {
				highestId = entity->getRuntimeIDComponent()->runtimeId.id;
				breakList.emplace_back(breakment);
			}
		}
	}
	if (!breakList.empty()) std::sort(breakList.begin(), breakList.end(), sortCrystal);
}

void AutoCrystal::placeCrystal(GameMode* gm) {
	if (Game::getLocalPlayer() == nullptr) return;
	if (placeList.empty()) return;
	if (InventoryUtils::getSelectedItemId() != 758) return;
	int placed = 0;
	if (iPlaceDelay >= 20 - placeSpeed) {
		for (const CrystalPlace& place : placeList) {
			gm->buildBlock(place.blockPos, 0, false);
			if (++placed >= wasteAmount) break;
			Game::getLocalPlayer()->swing();
		}
		iPlaceDelay = 0;
	}
	else iPlaceDelay++;
}

// In the breakCrystal function:
void AutoCrystal::breakCrystal(GameMode* gm) {
	if (Game::getLocalPlayer() == nullptr) return;
	if (!breakList.empty()) {
		if (breakList[0].crystal != nullptr) {
			if (smartbreak) {
				int breakInterval = std::max(1, 5 - (breakSpeed / 20));
				if (iBreakDelay >= breakInterval) {
					highestId = breakList[0].crystal->getRuntimeIDComponent()->runtimeId.id;
					gm->attack(breakList[0].crystal);
					Game::getLocalPlayer()->swing();
					if (breakSpeed > 50) {
						gm->attack(breakList[0].crystal);
						Game::getLocalPlayer()->swing();
					}
					iBreakDelay = 0;
				}
				else {
					iBreakDelay++;
				}
			}
			if (!smartbreak) {
				for (Actor* crystal : Game::getLocalPlayer()->getlevel()->getRuntimeActorList()) {
					if (crystal == nullptr) continue;
					if (crystal->getActorTypeComponent()->id != 71) continue;
					if (crystal->getEyePos().dist(Game::getLocalPlayer()->getEyePos()) > breakRange) continue;
					gm->attack(crystal);
					Game::getLocalPlayer()->swing();
					break;
				}
			}
		}
		if (!placeList.empty() && predict && breakList[0].targetDamage >= boostDmg) {
			int scaledPackets = predictPacket * (1 + (predictSpeed / 50));
			if (iBoostDelay >= std::max(1, 3 - (predictSpeed / 33))) {
				shouldChange = true;
				for (int i = 0; i < scaledPackets; i++) {
					gm->attack(placeList[0].actor);
					Game::getLocalPlayer()->swing();
					highestId++;
				}
				highestId -= scaledPackets;
				shouldChange = false;
				iBoostDelay = 0;
			}
			else {
				iBoostDelay++;
			}
		}
	}
}
void AutoCrystal::onNormalTick(LocalPlayer* localPlayer) {
	targetList.clear();
	entityList.clear();
	placeList.clear();
	breakList.clear();
	if (!localPlayer) return;
	if (swap) InventoryUtils::SwitchTo(InventoryUtils::getItem(720));
	if (!multiTask && Game::getLocalPlayer()->getItemUseDuration() > 0)
		return;
	Vec3 localPos = Game::getLocalPlayer()->getEyePos();
	for (Actor* actor : Game::getLocalPlayer()->getlevel()->getRuntimeActorList()) {
		if (!actor) continue;
		entityList.push_back(actor);
		if (!TargetUtil::isTargetValid(actor, mobsJS)) continue;
		if (actor->getEyePos().dist(localPos) > targetRange) continue;

		targetList.push_back(actor);
	}
	if (selfTest) targetList.push_back(Game::getLocalPlayer());
	if (targetList.empty()) return;
	std::sort(targetList.begin(), targetList.end(), TargetUtil::sortByDist);
	if (place) generatePlacement(targetList[0]);
	if (explode) getCrystals(targetList[0]);
	const int oldSlot = InventoryUtils::getSelectedSlot();
	bool didSwitch = false;
	if (explode) {
		breakCrystal(Game::getLocalPlayer()->gameMode);
	}
	if (place) {
		placeCrystal(Game::getLocalPlayer()->gameMode);
	}
	if (switchBack) InventoryUtils::SwitchTo(oldSlot);
}
void AutoCrystal::onSendPacket(Packet* packet) {
	if (Game::getLocalPlayer() == nullptr) return;
	if (rotate && !placeList.empty()) {
		const Vec2<float>& angle = Game::getLocalPlayer()->getEyePos().CalcAngle(placeList[0].blockPos.toFloat());
		if (packet->getId() == PacketID::PlayerAuthInput) {
			PlayerAuthInputPacket* authPkt = (PlayerAuthInputPacket*)packet;
			MovePlayerPacket* mpp = (MovePlayerPacket*)packet;
			authPkt->rotation = angle;
			authPkt->headYaw = angle.y;
			if (packetbreak1 && explode) breakCrystal(Game::getLocalPlayer()->getgameMode());
		}
		if (!predict || !shouldChange) return;
		if (packet->getId() == PacketID::InventoryTransaction) {
			InventoryTransactionPacket* invPkt = (InventoryTransactionPacket*)packet;
			ComplexInventoryTransaction* invComplex = invPkt->transaction.get();
			if (invComplex->type == ComplexInventoryTransaction::Type::ItemUseOnEntityTransaction) *(int*)((uintptr_t)(invComplex)+0x68) = highestId;
		}
	}
}

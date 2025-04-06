#include "Criticals.h"

Criticals::Criticals() : Module("Criticals", "Each hit is a critical hit", Category::COMBAT) {
	registerSetting(new EnumSetting("Mode", "NULL", { "Nukkit" }, &mode, 0));
}

std::string Criticals::getModeText() {
	switch (mode) {
	case 0: {
		return "Nukkit";
		break;
	}
	}
	return "NULL";
}
void Criticals::onSendPacket(Packet* packet) {
    struct GlideState {
        float offset = 0.f;
        int stage = 0;
        const float steps[13] = { -0.15f, -0.3f, -0.45f, -0.6f, -0.75f,
                                -0.9f, -1.05f, -1.2f, -1.35f, -1.5f,
                                -1.05f, -0.6f, -0.15f };
    };
    static GlideState glide;
    if (packet->getId() != PacketID::PlayerAuthInput) return;
    LocalPlayer* player = Game::getLocalPlayer();
    if (!player) {
        glide.offset = 0.f;
        glide.stage = 0;
        return;
    }
    PlayerAuthInputPacket* authPacket = (PlayerAuthInputPacket*)packet;
    authPacket->position.y += glide.steps[glide.stage];
    if (++glide.stage >= 13) {
        glide.stage = 0;
    }
}
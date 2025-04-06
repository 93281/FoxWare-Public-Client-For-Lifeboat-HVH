#include "Jitter.h"

Jitter::Jitter() : Module("Jitter", "Each hit is a critical hit", Category::COMBAT) {
	registerSetting(new EnumSetting("Mode", "NULL", { "Nukkit-Safe", "Nukkit-Blatant" }, &mode, 0));
}

std::string Jitter::getModeText() {
	switch (mode) {
	case 0: {
		return "Nukkit";
		break;
	}
	}
	return "NULL";
}
void Jitter::onSendPacket(Packet* packet) {
    if (mode == 0) {
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
        authPacket->position.x += glide.steps[glide.stage];
        authPacket->position.z += glide.steps[glide.stage];
        if (++glide.stage >= 13) {
            glide.stage = 0;
        }
    }
    if (mode == 1) {
        struct GlideState {
            float remainingOffset = 0.f;
            int stage = 0;
            const float targetSteps[5] = { -0.35f, -0.7f, -1.05f, -0.7f, -0.35f };
        };
        static GlideState glide;
        if (packet->getId() != PacketID::PlayerAuthInput) return;
        LocalPlayer* player = Game::getLocalPlayer();
        if (!player) {
            glide.remainingOffset = 0.f;
            glide.stage = 0;
            return;
        }
        PlayerAuthInputPacket* authPacket = (PlayerAuthInputPacket*)packet;
        if (glide.remainingOffset != 0.f) {
            float step = (glide.remainingOffset > 0) ? 0.35f : -0.35f;
            if (fabs(glide.remainingOffset) < 0.35f) {
                step = glide.remainingOffset;
            }
            authPacket->position.x += step;
            authPacket->position.z += step;
            glide.remainingOffset -= step;
        }
        else if (glide.stage < 5) {
            glide.remainingOffset = glide.targetSteps[glide.stage++];
        }
        else {
            glide.stage = 0;
        }
    }
}
#include "Criticals.h"
bool customStepsEnabled = false;
int stepCount = 13;
float customSteps[30]; 
std::vector<float> defaultSteps;
std::vector<Setting*> stepSettings;
Criticals::Criticals() : Module("Criticals", "Each hit is a critical hit", Category::COMBAT) {
    registerSetting(new EnumSetting("Mode", "Critical hit mode", { "Nukkit", "Custom" }, &mode, 0));
    defaultSteps = { -0.15f, -0.3f, -0.45f, -0.6f, -0.75f, -0.9f, -1.05f, -1.2f, -1.35f, -1.5f, -1.05f, -0.6f, -0.15f };
    registerSetting(new BoolSetting("Custom Steps", "Enable custom step values", &customStepsEnabled, false));
    registerSetting(new SliderSetting<int>("Step Count", "Number of steps in sequence", &stepCount, 13, 1, 30));
    for (int i = 0; i < 30; i++) {
        stepSettings.push_back(new SliderSetting<float>(
            ("Step " + std::to_string(i + 1)).c_str(),
            "Y offset for this step",
            &customSteps[i],
            (i < defaultSteps.size()) ? defaultSteps[i] : 0.f,
            -2.0f,
            2.0f
        ));
        registerSetting(stepSettings[i]);
    }
}

std::string Criticals::getModeText() {
    switch (mode) {
    case 0: return "Nukkit";
    case 1: return "Custom";
    default: return "NULL";
    }
}

void Criticals::onSendPacket(Packet* packet) {
    struct GlideState {
        float offset = 0.f;
        int stage = 0;
        std::vector<float> steps;
    };
    static GlideState glide;
    if (packet->getId() != PacketID::PlayerAuthInput) return;
    LocalPlayer* player = Game::getLocalPlayer();
    if (!player) {
        glide.offset = 0.f;
        glide.stage = 0;
        return;
    }
    if (customStepsEnabled) {
        glide.steps.clear();
        for (int i = 0; i < stepCount; i++) {
            glide.steps.push_back(customSteps[i]);
        }
    }
    else if (mode == 0) {
        glide.steps = defaultSteps;
    }
    if (glide.steps.empty()) return;
    PlayerAuthInputPacket* authPacket = (PlayerAuthInputPacket*)packet;
    authPacket->position.y += glide.steps[glide.stage];
    if (++glide.stage >= glide.steps.size()) {
        glide.stage = 0;
    }
}
#include "ClickGUI.h"
#include "../../../../../../Utils/TimerUtil.h"
#include "../../../../../../Libs/json.hpp"
#include "../../../ModuleManager.h"
bool blueWINDOWH;
bool dothecoolparticles;
float rectsizemodule;
ClickGUI::ClickGUI() : Module("ClickGUI", "Display all modules", Category::CLIENT, VK_INSERT) {
	registerSetting(new ColorSetting("Color", "NULL", &mainColor, mainColor));
    registerSetting(new SliderSetting<float>("Blur", "Background blur intensity", &blurStrength, 4.f, 0.f, 20.f));
	registerSetting(new BoolSetting("Description", "Show Description", &showDescription, true));
    registerSetting(new BoolSetting("BlurCGUIWindow", "Show CGUI Blur", &blueWINDOWH, true));
    registerSetting(new BoolSetting("Particles", "Show Particles", &dothecoolparticles, true));
    registerSetting(new SliderSetting<float>("Mod Rect Size", "Module enabled/disable rectangle size", &rectsizemodule, 0.f, 0.95f, 1.0f));
}

ClickGUI::~ClickGUI() {
	for (auto& window : windowList) {
		delete window;
	}
	windowList.clear();
}
struct clickGuiParticles {
	Vec2<float> startPos;
	Vec2<float> endPos;
	float speed;
	float size;
	float duration = 1.f;
	clickGuiParticles(const Vec2<float>& startpos, const Vec2<float>& endpos, const float& size2, const float& speed2) {
		this->startPos = startpos;
		this->endPos = endpos;
		this->size = size2;
		this->speed = speed2;
	};
};
std::vector<std::shared_ptr<clickGuiParticles>> circleList;
ClickGUI::ClickWindow::ClickWindow(std::string windowName, Vec2<float> startPos, Category c) {
	this->name = windowName;
	this->category = c;
	this->pos = startPos;
	this->extended = true;

	for (auto& mod : ModuleManager::moduleList) {
		if (mod->getCategory() == c) {
			this->moduleList.push_back(mod);
		}
	}

	std::sort(this->moduleList.begin(), this->moduleList.end(), [](Module* lhs, Module* rhs) {
		return lhs->getModuleName() < rhs->getModuleName();
		});
}

void ClickGUI::onDisable() {
	Game::clientInstance->grabMouse();

	isLeftClickDown = false;
	isRightClickDown = false;
	isHoldingLeftClick = false;
	isHoldingRightClick = false;

	draggingWindowPtr = nullptr;

	capturingKbSettingPtr = nullptr;
	draggingSliderSettingPtr = nullptr;

	openAnim = 0.0f;
}

void ClickGUI::onEnable() {
	Game::clientInstance->releasebMouse();
	openAnim = 0.0f;
}

bool ClickGUI::isVisible() {
	return false;
}

void ClickGUI::onKeyUpdate(int key, bool isDown) {
	if (!isEnabled()) {
		if (key == getKeybind() && isDown) {
			setEnabled(true);
		}
	}
	else {
		if (isDown) {
			if (key < 192) {
				if (capturingKbSettingPtr != nullptr) {
					if (key != VK_ESCAPE)
						*capturingKbSettingPtr->value = key;
					capturingKbSettingPtr = nullptr;
					return;
				}
			}
			if (key == getKeybind() || key == VK_ESCAPE) {
				setEnabled(false);
			}
		}
	}
}

void ClickGUI::onMouseUpdate(Vec2<float> mousePosA, char mouseButton, char isDown) {

	//MouseButtons
	//0 = mouse move
	//1 = left click
	//2 = right click
	//3 = middle click
	//4 = scroll   (isDown: 120 (SCROLL UP) and -120 (SCROLL DOWN))

	switch (mouseButton) {
	case 0:
		mousePos = mousePosA;
		break;
	case 1:
		isLeftClickDown = isDown;
		isHoldingLeftClick = isDown;
		break;
	case 2:
		isRightClickDown = isDown;
		isHoldingRightClick = isDown;
		break;
	case 4:
		float moveVec = (isDown < 0) ? -15.f : 15.f;
		for (auto& window : windowList) {
			if (window == draggingWindowPtr)
				continue;

			window->pos.y += moveVec;
		}
		break;
	}

	if (draggingWindowPtr != nullptr) {
		if (!isHoldingLeftClick)
			draggingWindowPtr = nullptr;
	}

	if (capturingKbSettingPtr != nullptr) {
		if (isRightClickDown) {
			*capturingKbSettingPtr->value = 0;
			capturingKbSettingPtr = nullptr;
			isRightClickDown = false;
		}
	}

	if (draggingSliderSettingPtr != nullptr) {
		if (!isHoldingLeftClick)
			draggingSliderSettingPtr = nullptr;
	}
}

void ClickGUI::InitClickGUI() {
	setEnabled(false);

	Vec2<float> startPos = Vec2<float>(25.f, 35.f);
	windowList.push_back(new ClickWindow("Combat", startPos, Category::COMBAT));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Movement", startPos, Category::MOVEMENT));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Render", startPos, Category::RENDER));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Player", startPos, Category::PLAYER));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("World", startPos, Category::WORLD));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Misc", startPos, Category::MISC));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Client", startPos, Category::CLIENT));

	initialized = true;
}
inline float fremate() {
	return 60.f;
}
inline float RandomFloat(float a, float b) {
	float random = ((float)rand()) / (float)RAND_MAX;
	float diff = b - a;
	float r = random * diff;
	return a + r;
}
inline float lerpSync(const float& a, const float& b, const float& duration) {
	const float result = (float)(a + (b - a) * duration * (60.f / fremate()));
	if (a < b && result > b) return b;
	else if (a > b && result < b) return b;
	return result;
}

void ClickGUI::Render() {
    if (!initialized) return;
    if (Game::canUseMoveKeys()) Game::clientInstance->releasebMouse();
    static Vec2<float> oldMousePos = mousePos;
    mouseDelta = mousePos.sub(oldMousePos);
    oldMousePos = mousePos;
    Vec2<float> screenSize = Game::clientInstance->guiData->windowSizeReal;
    float deltaTime = D2D::deltaTime;
    float textSize = 0.9f;
    float textPaddingX = 8.f;
    float textPaddingY = 4.f;
    float textHeight = D2D::getTextHeight("", textSize);
    float rounding = 4.f;
    float borderWidth = 1.f;
    std::string descriptionText = "NULL";
    openAnim = std::min(openAnim + deltaTime * 2.f, 1.f);
    static float opacity = 0.f;
    opacity = lerpSync(opacity, this->isEnabled() ? 1.f : 0.f, 0.15f);
    if (blurStrength > 0.1f)
        D2D::addBlur(Vec4<float>(0.f, 0.f, screenSize.x, screenSize.y), blurStrength * openAnim);
    D2D::fillRectangle(Vec4<float>(0.f, 0.f, screenSize.x, screenSize.y), UIColor(0, 0, 0, (int)(110 * openAnim)));
    if (dothecoolparticles) {
        int steps = 100;
        float stepHeight = screenSize.y / steps;
        for (int i = 0; i < steps; i++) {
            float t = (float)i / (steps - 1);
            UIColor currentColor = ColorUtil::lerp(
                UIColor(0, 0, 0, 100),
                UIColor(mainColor.r, mainColor.g, mainColor.b, 100),
                t
            );
            float top = i * stepHeight;
            float bottom = top + stepHeight;
            D2D::fillRectangle(Vec4<float>(
                0.f,
                top,
                screenSize.x,
                bottom
            ), currentColor);
        }
        float start = RandomFloat(0.f, screenSize.x);
        if (circleList.size() < 125) {
            int count = 1 * (int)(ceil(60.f / fremate()));
            for (int i = 0; i < count; i++)
                circleList.emplace_back(new clickGuiParticles(
                    Vec2<float>(start, screenSize.y + 10.f),
                    Vec2<float>(start, screenSize.y / RandomFloat(1.5f, 2.5f)),
                    RandomFloat(3.f, 10.f),
                    RandomFloat(1.f, 7.f)
                ));
        }
        for (int i = 0; i < circleList.size(); i++) {
            std::shared_ptr<clickGuiParticles> p = circleList[i];
            int alpha = (int)(150 * opacity * p->duration);
            alpha = std::clamp(alpha, 0, 255);
            UIColor colorWithAlpha(mainColor.r, mainColor.g, mainColor.b, alpha);
            D2D::fillCircle(p->startPos, colorWithAlpha, p->size);
            p->startPos.y -= p->speed * (20.f / fremate());
            if (p->startPos.y <= p->endPos.y)
                p->duration = lerpSync(p->duration, 0.f, 0.1f);
            if (p->duration <= 0.1f) {
                circleList.erase(circleList.begin() + i);
                i--;
            }
        }
    }
    for (auto& window : windowList) {
        if (window == draggingWindowPtr) window->pos = window->pos.add(mouseDelta);
        static CustomFont* customFontMod = ModuleManager::getModule<CustomFont>();
        float fontPercent = (float)customFontMod->fontSize / 25.f;
        float headerWidth = (int)(220.f * fontPercent);
        Vec4<float> headerRect = Vec4<float>(
            window->pos.x,
            window->pos.y,
            window->pos.x + headerWidth + (textPaddingX * 2.f),
            window->pos.y + textHeight + (textPaddingY * 2.f)
        );
        if (headerRect.contains(mousePos)) {
            if (isLeftClickDown) {
                draggingWindowPtr = window;
                isLeftClickDown = false;
            }
            else if (isRightClickDown) {
                window->extended = !window->extended;
                isRightClickDown = false;
            }
        }
        D2D::fillRoundedRectangle(headerRect, UIColor(30, 30, 40), rounding);
        D2D::drawRoundedRectangle(headerRect, UIColor(80, 80, 90), rounding, borderWidth);
        D2D::drawText(
            Vec2<float>(headerRect.x + textPaddingX, headerRect.y + textPaddingY),
            window->name,
            UIColor(240, 240, 245),
            textSize
        );
        if (window->extended) {
            float moduleSpace = 4.f;
            float yHeight = moduleSpace;
            for (auto& mod : window->moduleList) {
                yHeight += textHeight + (textPaddingY * 2.f);
                if (mod->extended) {
                    for (auto& setting : mod->getSettingList()) {
                        yHeight += textHeight + (textPaddingY * 2.f);
                        if (setting->type == SettingType::COLOR_S && static_cast<ColorSetting*>(setting)->extended) {
                            for (auto& slider : static_cast<ColorSetting*>(setting)->colorSliders) {
                                yHeight += textHeight + (textPaddingY * 2.f);
                            }
                        }
                    }
                }
                yHeight += moduleSpace;
            }
            float wbgPaddingX = 4.f;
            Vec4<float> contentRect = Vec4<float>(
                headerRect.x + wbgPaddingX,
                headerRect.w,
                headerRect.z - wbgPaddingX,
                headerRect.w + yHeight
            );
            if (blueWINDOWH) {
                D2D::addBlur(contentRect, 59.f);
            }
            if (rectsizemodule < 0.9f) {
                rectsizemodule = 0.9f;
            }
            D2D::fillRoundedRectangle(contentRect, UIColor(25, 25, 35, 100), rounding);
            D2D::drawRoundedRectangle(contentRect, UIColor(60, 60, 70), rounding, borderWidth);
            float yOffset = headerRect.w + moduleSpace;
            for (auto& mod : window->moduleList) {
                float modPaddingX = wbgPaddingX + 6.f;
                Vec4<float> modRect = Vec4<float>(
                    headerRect.x + modPaddingX + ((headerRect.z - headerRect.x - 2.f * modPaddingX) * (1.f - rectsizemodule) / 2.f),
                    yOffset + ((textHeight + textPaddingY * 2.f) * (1.f - rectsizemodule) / 2.f),
                    headerRect.z - modPaddingX - ((headerRect.z - headerRect.x - 2.f * modPaddingX) * (1.f - rectsizemodule) / 2.f),
                    yOffset + (textHeight + textPaddingY * 2.f) - ((textHeight + textPaddingY * 2.f) * (1.f - rectsizemodule) / 2.f)
                );
                if (modRect.contains(mousePos)) {
                    descriptionText = mod->getDescription();
                    if (isLeftClickDown) {
                        mod->toggle();
                        isLeftClickDown = false;
                    }
                    else if (isRightClickDown) {
                        mod->extended = !mod->extended;
                        isRightClickDown = false;
                    }
                }
                UIColor modColor = mod->isEnabled() ?
                    UIColor(mainColor.r, mainColor.g, mainColor.b, 180) :
                    UIColor(40, 40, 50, 120);
                D2D::fillRoundedRectangle(modRect, modColor, rounding);
                D2D::drawText(
                    Vec2<float>(modRect.x + textPaddingX, modRect.y + textPaddingY),
                    mod->getModuleName(),
                    mod->isEnabled() ? UIColor(255, 255, 255) : UIColor(200, 200, 200),
                    textSize
                );
                yOffset += textHeight + (textPaddingY * 2.f);
                if (mod->extended) {
                    float settingPaddingX = 12.f;
                    for (auto& setting : mod->getSettingList()) {
                        Vec4<float> settingRect = Vec4<float>(
                            modRect.x + settingPaddingX,
                            yOffset,
                            modRect.z - 6.f,
                            yOffset + textHeight + (textPaddingY * 2.f)
                        );
                        if (settingRect.contains(mousePos)) descriptionText = setting->description;
                        if (setting->type == SettingType::BOOL_S) {
                            BoolSetting* boolSetting = static_cast<BoolSetting*>(setting);
                            if (settingRect.contains(mousePos) && isLeftClickDown) {
                                (*boolSetting->value) = !(*boolSetting->value);
                                isLeftClickDown = false;
                            }
                            float toggleSize = textHeight;
                            Vec4<float> toggleRect = Vec4<float>(
                                settingRect.z - textPaddingX - toggleSize,
                                settingRect.y + (settingRect.w - settingRect.y - toggleSize) / 2.f,
                                settingRect.z - textPaddingX,
                                settingRect.y + (settingRect.w - settingRect.y + toggleSize) / 2.f
                            );
                            D2D::fillRoundedRectangle(toggleRect, UIColor(50, 50, 60), toggleSize / 2.f);
                            if (*boolSetting->value) {
                                D2D::fillRoundedRectangle(toggleRect, UIColor(mainColor.r, mainColor.g, mainColor.b, 200), toggleSize / 2.f);
                            }
                            D2D::drawText(
                                Vec2<float>(settingRect.x + textPaddingX, settingRect.y + textPaddingY),
                                setting->name,
                                UIColor(220, 220, 230),
                                textSize
                            );
                        }
                        else if (setting->type == SettingType::KEYBIND_S) {
                            KeybindSetting* keybindSetting = static_cast<KeybindSetting*>(setting);
                            if (settingRect.contains(mousePos) && isLeftClickDown) {
                                capturingKbSettingPtr = (capturingKbSettingPtr == keybindSetting) ? nullptr : keybindSetting;
                                isLeftClickDown = false;
                            }
                            std::string keyName = capturingKbSettingPtr == keybindSetting ? "..." :
                                ((*keybindSetting->value) != 0 ? KeyNames[(*keybindSetting->value)] : "None");
                            D2D::drawText(
                                Vec2<float>(settingRect.x + textPaddingX, settingRect.y + textPaddingY),
                                setting->name + ":",
                                UIColor(220, 220, 230),
                                textSize
                            );
                            D2D::drawText(
                                Vec2<float>(settingRect.z - textPaddingX - D2D::getTextWidth(keyName, textSize),
                                    settingRect.y + textPaddingY),
                                keyName,
                                UIColor(220, 220, 230),
                                textSize
                            );
                        }
                        else if (setting->type == SettingType::ENUM_S) {
                            EnumSetting* enumSetting = static_cast<EnumSetting*>(setting);
                            if (settingRect.contains(mousePos)) {
                                if (isLeftClickDown) {
                                    (*enumSetting->value) = ((*enumSetting->value) + 1) % enumSetting->enumList.size();
                                    isLeftClickDown = false;
                                }
                                else if (isRightClickDown) {
                                    (*enumSetting->value) = ((*enumSetting->value) - 1 + enumSetting->enumList.size()) % enumSetting->enumList.size();
                                    isRightClickDown = false;
                                }
                            }
                            std::string modeName = enumSetting->enumList[(*enumSetting->value)];
                            D2D::drawText(
                                Vec2<float>(settingRect.x + textPaddingX, settingRect.y + textPaddingY),
                                setting->name + ":",
                                UIColor(220, 220, 230),
                                textSize
                            );
                            D2D::drawText(
                                Vec2<float>(settingRect.z - textPaddingX - D2D::getTextWidth(modeName, textSize),
                                    settingRect.y + textPaddingY),
                                modeName,
                                UIColor(220, 220, 230),
                                textSize
                            );
                        }
                        else if (setting->type == SettingType::COLOR_S) {
                            ColorSetting* colorSetting = static_cast<ColorSetting*>(setting);
                            if (settingRect.contains(mousePos) && isRightClickDown) {
                                colorSetting->extended = !colorSetting->extended;
                                isRightClickDown = false;
                            }
                            float colorBoxSize = textHeight;
                            Vec4<float> colorBoxRect = Vec4<float>(
                                settingRect.z - textPaddingX - colorBoxSize,
                                settingRect.y + (settingRect.w - settingRect.y - colorBoxSize) / 2.f,
                                settingRect.z - textPaddingX,
                                settingRect.y + (settingRect.w - settingRect.y + colorBoxSize) / 2.f
                            );
                            D2D::drawText(
                                Vec2<float>(settingRect.x + textPaddingX, settingRect.y + textPaddingY),
                                setting->name + ":",
                                UIColor(220, 220, 230),
                                textSize
                            );
                            D2D::fillRoundedRectangle(colorBoxRect, (*colorSetting->colorPtr), rounding);
                            D2D::drawRoundedRectangle(colorBoxRect, UIColor(90, 90, 100), rounding, borderWidth);
                            if (colorSetting->extended) {
                                yOffset += textHeight + (textPaddingY * 2.f);
                                for (auto& slider : colorSetting->colorSliders) {
                                    Vec4<float> sliderRect = Vec4<float>(
                                        settingRect.x,
                                        yOffset,
                                        settingRect.z,
                                        yOffset + textHeight + (textPaddingY * 2.f)
                                    );
                                    if (sliderRect.contains(mousePos) && isLeftClickDown) {
                                        draggingSliderSettingPtr = slider;
                                        isLeftClickDown = false;
                                    }
                                    uint8_t& value = (*slider->valuePtr);
                                    float minValue = (float)slider->minValue;
                                    float maxValue = (float)slider->maxValue;
                                    if (draggingSliderSettingPtr == slider) {
                                        float percent = std::clamp((mousePos.x - sliderRect.x) / (sliderRect.z - sliderRect.x), 0.f, 1.f);
                                        value = (int)(minValue + (maxValue - minValue) * percent);
                                    }
                                    float valuePercent = std::clamp((value - minValue) / (maxValue - minValue), 0.f, 1.f);
                                    Vec4<float> filledRect = Vec4<float>(
                                        sliderRect.x,
                                        sliderRect.y,
                                        sliderRect.x + (sliderRect.z - sliderRect.x) * valuePercent,
                                        sliderRect.w
                                    );
                                    D2D::fillRectangle(filledRect, UIColor(mainColor.r, mainColor.g, mainColor.b, 150));
                                    D2D::drawRectangle(sliderRect, UIColor(70, 70, 80), borderWidth);
                                    char valueText[10];
                                    sprintf_s(valueText, 10, "%i", (int)value);
                                    D2D::drawText(
                                        Vec2<float>(sliderRect.x + textPaddingX, sliderRect.y + textPaddingY),
                                        slider->name + ":",
                                        UIColor(220, 220, 230),
                                        textSize
                                    );
                                    D2D::drawText(
                                        Vec2<float>(sliderRect.z - textPaddingX - D2D::getTextWidth(valueText, textSize),
                                            sliderRect.y + textPaddingY),
                                        valueText,
                                        UIColor(220, 220, 230),
                                        textSize
                                    );
                                    yOffset += textHeight + (textPaddingY * 2.f);
                                }
                            }
                        }
                        else if (setting->type == SettingType::SLIDER_S) {
                            SliderSettingBase* sliderSetting = static_cast<SliderSettingBase*>(setting);
                            if (settingRect.contains(mousePos) && isLeftClickDown) {
                                draggingSliderSettingPtr = sliderSetting;
                                isLeftClickDown = false;
                            }
                            if (sliderSetting->valueType == ValueType::INT_T) {
                                SliderSetting<int>* intSlider = static_cast<SliderSetting<int>*>(sliderSetting);
                                int& value = (*intSlider->valuePtr);
                                float minValue = (float)intSlider->minValue;
                                float maxValue = (float)intSlider->maxValue;

                                if (draggingSliderSettingPtr == sliderSetting) {
                                    float percent = std::clamp((mousePos.x - settingRect.x) / (settingRect.z - settingRect.x), 0.f, 1.f);
                                    value = (int)(minValue + (maxValue - minValue) * percent);
                                }
                                float valuePercent = std::clamp((value - minValue) / (maxValue - minValue), 0.f, 1.f);
                                Vec4<float> filledRect = Vec4<float>(
                                    settingRect.x,
                                    settingRect.y,
                                    settingRect.x + (settingRect.z - settingRect.x) * valuePercent,
                                    settingRect.w
                                );
                                D2D::fillRectangle(filledRect, UIColor(mainColor.r, mainColor.g, mainColor.b, 150));
                                D2D::drawRectangle(settingRect, UIColor(70, 70, 80), borderWidth);
                                char valueText[10];
                                sprintf_s(valueText, 10, "%i", value);
                                D2D::drawText(
                                    Vec2<float>(settingRect.x + textPaddingX, settingRect.y + textPaddingY),
                                    setting->name + ":",
                                    UIColor(220, 220, 230),
                                    textSize
                                );

                                D2D::drawText(
                                    Vec2<float>(settingRect.z - textPaddingX - D2D::getTextWidth(valueText, textSize),
                                        settingRect.y + textPaddingY),
                                    valueText,
                                    UIColor(220, 220, 230),
                                    textSize
                                );
                            }
                            else if (sliderSetting->valueType == ValueType::FLOAT_T) {
                                SliderSetting<float>* floatSlider = static_cast<SliderSetting<float>*>(sliderSetting);
                                float& value = (*floatSlider->valuePtr);
                                float minValue = floatSlider->minValue;
                                float maxValue = floatSlider->maxValue;
                                if (draggingSliderSettingPtr == sliderSetting) {
                                    float percent = std::clamp((mousePos.x - settingRect.x) / (settingRect.z - settingRect.x), 0.f, 1.f);
                                    value = minValue + (maxValue - minValue) * percent;
                                }
                                float valuePercent = std::clamp((value - minValue) / (maxValue - minValue), 0.f, 1.f);
                                Vec4<float> filledRect = Vec4<float>(
                                    settingRect.x,
                                    settingRect.y,
                                    settingRect.x + (settingRect.z - settingRect.x) * valuePercent,
                                    settingRect.w
                                );
                                D2D::fillRoundedRectangle(filledRect, UIColor(mainColor.r, mainColor.g, mainColor.b, 180), 3.f);
                                D2D::drawRoundedRectangle(settingRect, UIColor(70, 70, 80), 3.f, borderWidth);
                                char valueText[12];
                                sprintf_s(valueText, 12, "%.2f", value);
                                D2D::drawText(
                                    Vec2<float>(settingRect.x + textPaddingX, settingRect.y + textPaddingY),
                                    setting->name + ":",
                                    UIColor(220, 220, 230),
                                    textSize
                                );

                                D2D::drawText(
                                    Vec2<float>(settingRect.z - textPaddingX - D2D::getTextWidth(valueText, textSize),
                                        settingRect.y + textPaddingY),
                                    valueText,
                                    UIColor(220, 220, 230),
                                    textSize
                                );
                            }
                            }
                        yOffset += textHeight + (textPaddingY * 2.f);
                    }
                }
                yOffset += moduleSpace;
            }
        }
    }
    if (showDescription && descriptionText != "NULL" && !draggingWindowPtr && !draggingSliderSettingPtr) {
        Vec2<float> mousePadding(20.f, 20.f);
        float tooltipPadding = 8.f;
        float tooltipWidth = D2D::getTextWidth(descriptionText, 0.8f) + tooltipPadding * 2.f;
        Vec4<float> tooltipRect = Vec4<float>(
            mousePos.x + mousePadding.x,
            mousePos.y + mousePadding.y,
            mousePos.x + mousePadding.x + tooltipWidth,
            mousePos.y + mousePadding.y + D2D::getTextHeight(descriptionText, 0.8f) + tooltipPadding
        );
        D2D::fillRoundedRectangle(tooltipRect, UIColor(30, 30, 40, 220), rounding);
        D2D::drawRoundedRectangle(tooltipRect, UIColor(80, 80, 90), rounding, borderWidth);
        D2D::drawText(
            Vec2<float>(tooltipRect.x + tooltipPadding, tooltipRect.y + tooltipPadding / 2),
            descriptionText,
            UIColor(240, 240, 245),
            0.8f
        );
    }
    isLeftClickDown = false;
    isRightClickDown = false;
}
void ClickGUI::updateSelectedAnimRect(Vec4<float>& rect, float& anim) {
	bool shouldUp = rect.contains(mousePos);

	if (draggingWindowPtr != nullptr)
		shouldUp = false;

	if (draggingSliderSettingPtr != nullptr) {
		if (&draggingSliderSettingPtr->selectedAnim != &anim)
			shouldUp = false;
		else
			shouldUp = true;
	}

	//anim += D2D::deltaTime * (shouldUp ? 5.f : -2.f);
	if (shouldUp)
		anim = 1.f;
	else
		anim -= D2D::deltaTime * 2.f;

	if (anim > 1.f)
		anim = 1.f;
	if (anim < 0.f)
		anim = 0.f;
}

using json = nlohmann::json;

void ClickGUI::onLoadConfig(void* confVoid) {
	json* conf = reinterpret_cast<json*>(confVoid);
	std::string modName = this->getModuleName();

	if (conf->contains(modName)) {
		json obj = conf->at(modName);
		if (obj.is_null())
			return;

		if (obj.contains("enabled")) {
			this->setEnabled(obj.at("enabled").get<bool>());
		}

		for (auto& setting : getSettingList()) {
			std::string settingName = setting->name;

			if (obj.contains(settingName)) {
				json confValue = obj.at(settingName);
				if (confValue.is_null())
					continue;

				switch (setting->type) {
				case SettingType::BOOL_S: {
					BoolSetting* boolSetting = static_cast<BoolSetting*>(setting);
					(*boolSetting->value) = confValue.get<bool>();
					break;
				}
				case SettingType::KEYBIND_S: {
					KeybindSetting* keybindSetting = static_cast<KeybindSetting*>(setting);
					(*keybindSetting->value) = confValue.get<int>();
					break;
				}
				case SettingType::ENUM_S: {
					EnumSetting* enumSetting = static_cast<EnumSetting*>(setting);
					(*enumSetting->value) = confValue.get<int>();
					break;
				}
				case SettingType::COLOR_S: {
					ColorSetting* colorSetting = static_cast<ColorSetting*>(setting);
					(*colorSetting->colorPtr) = ColorUtil::HexStringToColor(confValue.get<std::string>());
					break;
				}
				case SettingType::SLIDER_S: {
					SliderSettingBase* sliderSettingBase = static_cast<SliderSettingBase*>(setting);
					if (sliderSettingBase->valueType == ValueType::INT_T) {
						SliderSetting<int>* intSlider = static_cast<SliderSetting<int>*>(sliderSettingBase);
						(*intSlider->valuePtr) = confValue.get<int>();
					}
					else if (sliderSettingBase->valueType == ValueType::FLOAT_T) {
						SliderSetting<float>* floatSlider = static_cast<SliderSetting<float>*>(sliderSettingBase);
						(*floatSlider->valuePtr) = confValue.get<float>();
					}
					break;
				}
				}
			}
		}

		for (auto& window : windowList) {
			std::string windowName = window->name;

			if (obj.contains(windowName)) {
				json confValue = obj.at(windowName);
				if (confValue.is_null())
					continue;

				if (confValue.contains("isExtended")) {
					window->extended = confValue["isExtended"].get<bool>();
				}

				if (confValue.contains("pos")) {
					window->pos.x = confValue["pos"]["x"].get<float>();
					window->pos.y = confValue["pos"]["y"].get<float>();
				}
			}
		}
	}
}

void ClickGUI::onSaveConfig(void* confVoid) {
	json* conf = reinterpret_cast<json*>(confVoid);
	std::string modName = this->getModuleName();
	json obj = (*conf)[modName];

	obj["enabled"] = this->isEnabled();

	for (auto& setting : getSettingList()) {
		std::string settingName = setting->name;

		switch (setting->type) {
		case SettingType::BOOL_S: {
			BoolSetting* boolSetting = static_cast<BoolSetting*>(setting);
			obj[settingName] = (*boolSetting->value);
			break;
		}
		case SettingType::KEYBIND_S: {
			KeybindSetting* keybindSetting = static_cast<KeybindSetting*>(setting);
			obj[settingName] = (*keybindSetting->value);
			break;
		}
		case SettingType::ENUM_S: {
			EnumSetting* enumSetting = static_cast<EnumSetting*>(setting);
			obj[settingName] = (*enumSetting->value);
			break;
		}
		case SettingType::COLOR_S: {
			ColorSetting* colorSetting = static_cast<ColorSetting*>(setting);
			obj[settingName] = ColorUtil::ColorToHexString((*colorSetting->colorPtr));
			break;
		}
		case SettingType::SLIDER_S: {
			SliderSettingBase* sliderSettingBase = static_cast<SliderSettingBase*>(setting);
			if (sliderSettingBase->valueType == ValueType::INT_T) {
				SliderSetting<int>* intSlider = static_cast<SliderSetting<int>*>(sliderSettingBase);
				obj[settingName] = (*intSlider->valuePtr);
			}
			else if (sliderSettingBase->valueType == ValueType::FLOAT_T) {
				SliderSetting<float>* floatSlider = static_cast<SliderSetting<float>*>(sliderSettingBase);
				obj[settingName] = (*floatSlider->valuePtr);
			}
			break;
		}
		}
	}

	for (auto& window : windowList) {
		obj[window->name]["isExtended"] = window->extended;
		obj[window->name]["pos"]["x"] = window->pos.x;
		obj[window->name]["pos"]["y"] = window->pos.y;
	}

	(*conf)[modName] = obj;
}
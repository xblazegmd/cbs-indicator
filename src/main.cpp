#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

#include <cstdint>
#include <string>

using namespace geode::prelude;

// For EndLevelLayer
static bool g_clickBetweenSteps = false;
static bool g_clickOnSteps = false;

static Mod* g_mod = Mod::get();

void setPositionBasedOnSetting(CCNode* node, const std::string& setting) {
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    std::string alignment = g_mod->getSettingValue<std::string>(setting);

    if (alignment == "Top-Left") {
        node->setPosition({ 0, winSize.height });
        node->setAnchorPoint({ 0, 1 });
    } else if (alignment == "Top-Right") {
        node->setPosition({ winSize.width, winSize.height });
        node->setAnchorPoint({ 1, 1 });
    } else if (alignment == "Bottom-Left") {
        node->setPosition({ 0, 0 });
        node->setAnchorPoint({ 0, 0 });
    } else if (alignment == "Bottom-Right") {
        node->setPosition({ winSize.width, 0 });
        node->setAnchorPoint({ 1, 0 });
    }
}

class $modify(CBSPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        if (!g_mod->getSettingValue<bool>("gp-enabled")) return true;

        // Indicator
        std::string indText;

        // Since CBS and CoS can be enabled at the same time
        if (m_clickOnSteps && !m_clickBetweenSteps) indText = "CoS";
        else if (m_clickBetweenSteps) indText = "CBS";

        int64_t gpOpacity = g_mod->getSettingValue<int64_t>("gp-opacity");
        CCNode* indicator;
        if (g_mod->getSettingValue<bool>("gp-image")) {
            indicator = CCSprite::create("cbs.png"_spr);
            static_cast<CCSprite*>(indicator)->setOpacity(gpOpacity);
        } else {
            indicator = CCLabelBMFont::create(indText.c_str(), "bigFont.fnt");
            static_cast<CCLabelBMFont*>(indicator)->setOpacity(gpOpacity);
        }
        indicator->setVisible(m_clickBetweenSteps || m_clickOnSteps);
        indicator->setScale(.2f);

        setPositionBasedOnSetting(indicator, "gp-position");

        indicator->setID("indicator"_spr);

        /* Add this directly to UILayer here since hooking UILayer
        doesn't work for getting "m_clickBetweenSteps" and "m_clickOnSteps" */
        m_uiLayer->addChild(indicator);
        return true;
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        g_clickBetweenSteps = m_clickBetweenSteps;
        g_clickOnSteps = m_clickOnSteps && !m_clickBetweenSteps; // Handle the "CBS and CoS at the same time" case

        // Fade out
        auto indicator = m_uiLayer->getChildByID("indicator"_spr);
        if (!indicator) return;
        indicator->runAction(CCFadeTo::create(.5f, 0));
    }

    void fullReset() {
        PlayLayer::fullReset();
        // Restore opacity after fade out
        auto indicator = typeinfo_cast<CCLabelBMFont*>(m_uiLayer->getChildByID("indicator"_spr));
        if (!indicator) return;
        indicator->setOpacity(g_mod->getSettingValue<int64_t>("gp-opacity"));
    }
};

class $modify(CBSEndLevelLayer, EndLevelLayer) {
    void customSetup() {
        EndLevelLayer::customSetup();
        if (!g_clickBetweenSteps && !g_clickOnSteps) return;

        // Watermark
        std::string watermarkText;
        if (g_clickOnSteps) watermarkText = "CoS";
        else if (g_clickBetweenSteps) watermarkText = "CBS";

        auto watermark = CCLabelBMFont::create(watermarkText.c_str(), "bigFont.fnt");
        watermark->setVisible((g_clickBetweenSteps || g_clickOnSteps) && g_mod->getSettingValue<bool>("wm-enabled"));
        watermark->setScale(.2f);
        watermark->setOpacity(10);

        setPositionBasedOnSetting(watermark, "wm-position");

        watermark->setID("watermark"_spr);
        this->addChild(watermark);

        // Custom completion text
        if (!g_mod->getSettingValue<bool>("end-text-enabled")) return;
        std::string completionStr;
        if (g_clickOnSteps) completionStr = g_mod->getSettingValue<std::string>("end-text-cos");
        else if (g_clickBetweenSteps) completionStr = g_mod->getSettingValue<std::string>("end-text-cbs");

        auto completeMsg = m_mainLayer->getChildByID("complete-message");
        if (!completeMsg) return;

        if (auto completeMsgArea = typeinfo_cast<TextArea*>(completeMsg)) {
			completeMsgArea->setString(completionStr);
			completeMsgArea->setScale(.7f);
		} else if (auto completeMsgLabel = typeinfo_cast<CCLabelBMFont*>(completeMsg)) {
			completeMsgLabel->setString(completionStr.c_str());
			completeMsgLabel->setScale(.7f);
		}
    }
};
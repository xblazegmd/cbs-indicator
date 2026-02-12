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

        auto indicator = CCLabelBMFont::create(indText.c_str(), "bigFont.fnt");
        indicator->setVisible(m_clickBetweenSteps || m_clickOnSteps);
        indicator->setScale(.2f);
        indicator->setOpacity(g_mod->getSettingValue<int64_t>("gp-opacity"));

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

        // Temporary until NodeIDs works again
        for (auto child : CCArrayExt<CCNode*>(m_mainLayer->getChildren()))
            if (auto sprite = typeinfo_cast<CCSprite*>(child))
                for (auto coinTxt : {
                    "secretCoinUI_001.png",
                    "secretCoinUI2_001.png",
                    "secretCoin_b_01_001.png",
                    "secretCoin_2_b_01_001.png"
                }) if (isSpriteFrameName(sprite, coinTxt)) return;

        float size = g_clickBetweenSteps ? .7f : .9f;
        if (auto textArea = m_mainLayer->getChildByType<TextArea>(-1)) {
            textArea->setString(completionStr);
            textArea->setScale(size);
        } else if (auto label = m_mainLayer->getChildByType<CCLabelBMFont>(-1)) {
            label->setString(completionStr.c_str());
            label->setScale(size);
        }
    }
};
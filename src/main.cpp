#include "Geode/cocos/label_nodes/CCLabelBMFont.h"
#include "Geode/utils/cocos.hpp"
#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

using namespace geode::prelude;

// For EndLevelLayer
static bool g_clickBetweenSteps = false;
static bool g_clickOnSteps = false;

class $modify(CBSPlayLayer, PlayLayer) {
    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;

        // Indicator
        std::string indText;
        if (m_clickOnSteps && !m_clickBetweenSteps) indText = "CoS";
        else if (m_clickBetweenSteps) indText = "CBS";

        auto indicator = CCLabelBMFont::create(indText.c_str(), "bigFont.fnt");
        indicator->setVisible(m_clickBetweenSteps || m_clickOnSteps);
        indicator->setScale(.2f);
        indicator->setOpacity(10);

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        indicator->setPosition({ 0, winSize.height });
        indicator->setAnchorPoint({ 0, 1 });

        indicator->setID("indicator"_spr);

        /* Add this directly to UILayer here since hooking UILayer
        doesn't work for getting "m_clickBetweenSteps" and "m_clickOnSteps" */
        m_uiLayer->addChild(indicator);
        return true;
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        g_clickBetweenSteps = m_clickBetweenSteps;
        g_clickOnSteps = m_clickOnSteps && !m_clickBetweenSteps;

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
        indicator->setOpacity(10);
    }
};

class $modify(CBSEndLevelLayer, EndLevelLayer) {
    void customSetup() {
        EndLevelLayer::customSetup();
        if (!g_clickBetweenSteps && !g_clickOnSteps) return;

        // Watermark on the top-right
        std::string watermarkText;
        if (g_clickOnSteps) watermarkText = "CoS";
        else if (g_clickBetweenSteps) watermarkText = "CBS";

        auto watermark = CCLabelBMFont::create(watermarkText.c_str(), "bigFont.fnt");
        watermark->setVisible(g_clickBetweenSteps || g_clickOnSteps);
        watermark->setScale(.2f);
        watermark->setOpacity(10);

        auto winSize = CCDirector::sharedDirector()->getWinSize();
        watermark->setPosition({ winSize.width, winSize.height });
        watermark->setAnchorPoint({ 1, 1 });

        watermark->setID("watermark"_spr);
        this->addChild(watermark);

        // Custom completion text
        std::string completionStr;
        if (g_clickOnSteps) completionStr = "CoS";
        else if (g_clickBetweenSteps) completionStr = "CBS Detected Loser!";

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
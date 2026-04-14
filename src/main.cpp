#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/EndLevelLayer.hpp>

#include <arc/prelude.hpp>
#include <cstdint>
#include <optional>
#include <string>

#include <xblazegmd.geode-api/include/XblazeAPI.hpp>

using namespace geode::prelude;

// For EndLevelLayer
static std::optional<std::string> g_active = std::nullopt;

void setPositionBasedOnSetting(CCNode* node, const std::string& setting) {
    auto winSize = CCDirector::sharedDirector()->getWinSize();
    std::string alignment = Mod::get()->getSettingValue<std::string>(setting);

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
    struct Fields {
        async::TaskHolder<void> m_task;
        CCLabelBMFont* m_indicator;
    };

    bool init(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        if (!PlayLayer::init(level, useReplay, dontCreateObjects)) return false;
        if (!Mod::get()->getSettingValue<bool>("gp-enabled")) return true;

        // Indicator
        std::string indText;
        auto active = getActive();
        if (active.has_value()) {
            indText = active.value();
        }

        m_fields->m_indicator = CCLabelBMFont::create(indText.c_str(), "bigFont.fnt");
        m_fields->m_indicator->setOpacity(Mod::get()->getSettingValue<int64_t>("gp-opacity"));
        m_fields->m_indicator->setVisible(active.has_value()); // If std::nullopt then CBS/CoS are disabled
        m_fields->m_indicator->setScale(.2f);

        setPositionBasedOnSetting(m_fields->m_indicator, "gp-position");
        m_fields->m_indicator->setID("indicator"_spr);

        m_fields->m_task.spawn(
            "checkForCBSToggle"_spr,
            [this] -> arc::Future<> {
                while (true) {
                    auto active = this->getActive();
                    m_fields->m_indicator->setVisible(active.has_value());
                    if (active.has_value()) {
                        m_fields->m_indicator->setCString(active.value().c_str());
                    }

                    co_await xblazeapi::sleepMillis(10);
                }
            },
            [] {}
        );

        /* Add this directly to UILayer here since hooking UILayer
        doesn't work for getting "m_clickBetweenSteps" and "m_clickOnSteps" */
        m_uiLayer->addChild(m_fields->m_indicator);
        return true;
    }

    void levelComplete() {
        PlayLayer::levelComplete();
        g_active = getActive();
        if (m_fields->m_indicator) {
            m_fields->m_indicator->runAction(CCFadeTo::create(.5f, 0)); // Fade out
        }
    }

    void fullReset() {
        PlayLayer::fullReset();
        if (m_fields->m_indicator) {
            m_fields->m_indicator->setOpacity(Mod::get()->getSettingValue<int64_t>("gp-opacity")); // Restore opacity after fade out
        }
    }

    std::optional<std::string> getActive() {
        auto cbf = Loader::get()->getLoadedMod("syzzi.click_between_frames");
        if (cbf && !cbf->getSettingValue<bool>("soft-toggle")) {
            return "CBF";
        }
        if (m_clickBetweenSteps) {
            return "CBS";
        } else if (m_clickOnSteps) {
            return "CoS";
        }
        return std::nullopt;
    }
};

class $modify(CBSEndLevelLayer, EndLevelLayer) {
    void customSetup() {
        EndLevelLayer::customSetup();
        if (!g_active.has_value()) return;

        // Watermark (ignore with CBF since it has a built-in one)
        if (Mod::get()->getSettingValue<bool>("wm-enabled") && g_active.value() != "CBF") {
            // Since at the beginning we exit if g_active is std::nullopt, calling g_active.value() here is safe
            auto watermark = CCLabelBMFont::create(g_active.value().c_str(), "bigFont.fnt");
            watermark->setScale(.2f);
            watermark->setOpacity(Mod::get()->getSettingValue<int64_t>("wm-opacity"));

            setPositionBasedOnSetting(watermark, "wm-position");

            watermark->setID("watermark"_spr);
            this->addChild(watermark);
        }

        // Custom completion text
        if (Mod::get()->getSettingValue<bool>("end-text-enabled")) {
            std::string text;
            if (g_active.value() == "CBF") {
                text = Mod::get()->getSettingValue<std::string>("end-text-cbf");
            } else if (g_active.value() == "CBS") {
                text = Mod::get()->getSettingValue<std::string>("end-text-cbs");
            } else if (g_active.value() == "CoS") {
                text = Mod::get()->getSettingValue<std::string>("end-text-cos");
            }

            auto completeMsg = m_mainLayer->getChildByID("complete-message");
            if (!completeMsg) return;

            if (auto completeMsgArea = typeinfo_cast<TextArea*>(completeMsg)) {
			    completeMsgArea->setString(text);
			    completeMsgArea->setScale(.7f);
		    } else if (auto completeMsgLabel = typeinfo_cast<CCLabelBMFont*>(completeMsg)) {
			    completeMsgLabel->setCString(text.c_str());
			    completeMsgLabel->setScale(.7f);
		    }
        }
    }
};
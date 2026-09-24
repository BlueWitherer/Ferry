#include <Util.h>

#include <Geode/Geode.hpp>

#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

class $modify(MyMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        log::debug("Hello from my MenuLayer::init hook! This layer has {} child nodes.", getChildrenCount());

        if (auto menu = getChildByID("bottom-menu")) {
            auto myButton = CCMenuItemSpriteExtra::create(
                CCSprite::createWithSpriteFrameName("GJ_likeBtn_001.png"),
                this,
                menu_selector(MyMenuLayer::onMyButton));
            myButton->setID("gay-btn"_spr);

            menu->addChild(myButton);
            menu->updateLayout();
        };

        addEventListener(
            ProgressEvent(),
            [](float progress) {
                log::trace("download {}%", progress);
            });

        return true;
    };

    void onMyButton(CCObject*) {
        async::spawn(
            SaveManager::get()->downloadGameVars(),
            [](Result<StringMap<bool>> res) {
                if (res.isErr()) return log::error("Failed to download variables: {}", res.unwrapErr());
                auto vars = std::move(res).unwrap();

                auto gm = GameManager::sharedState();
                for (auto const& [key, val] : vars) {
                    if (key.size() != 4) {
                        log::error("Key {} is not valid", key);
                        continue;
                    };

                    log::trace("Setting game variable {} to {}...", key, val);
                    gm->setGameVariable(key.c_str(), val);
                };
            });
    };
};
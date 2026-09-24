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
                CircleButtonSprite::createWithSprite(
                    "icon.png"_spr,
                    0.95f,
                    CircleBaseColor::Green,
                    CircleBaseSize::MediumAlt),
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
        SyncPopup::create()->show();
    };
};
#include <Util.h>

#include <Geode/Geode.hpp>

#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

class $modify(FerryMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        if (auto menu = getChildByID("bottom-menu")) {
            auto btn = CCMenuItemExt::createSpriteExtra(
                CircleButtonSprite::createWithSprite(
                    "icon.png"_spr,
                    0.95f,
                    CircleBaseColor::Green,
                    CircleBaseSize::MediumAlt),
                [](auto) {
                    if (GJAccountManager::sharedState()->m_accountID <= 0) {
                        AccountLayer::create()->showLayer(GameManager::sharedState()->getGameVariable(GameVar::FastMenu));
                        Notification::create("You must be logged in to use Ferry!", NotificationIcon::Warning)->show();

                        return;
                    };

                    SyncPopup::create()->show();
                });
            btn->setID("ferry-btn"_spr);

            menu->addChild(btn);
            menu->updateLayout();
        };

        return true;
    };
};
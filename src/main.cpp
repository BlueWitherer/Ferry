#include <Util.h>

#include <Geode/Geode.hpp>

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/AccountLayer.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

$on_game(Loaded) {
    log::debug("Using web API url: {}", url::apiBase);
};

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
                    if (!argon::signedIn()) {
                        AccountLayer::create()->showLayer(GameManager::sharedState()->getGameVariable(GameVar::FastMenu));
                        Notification::create("(Ferry) You must be logged in to use Ferry!", NotificationIcon::Warning)->show();

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

class $modify(FerryAccountLayer, AccountLayer) {
    void FLAlert_Clicked(FLAlertLayer* layer, bool btn2) {
        switch (layer->getTag()) {
            default: break;

            case 1: {  // save
                if (Mod::get()->getSettingValue<bool>("also-upload")) {
                    Notification::create("(Ferry) Syncing settings data to cloud...", NotificationIcon::Loading)->show();

                    async::spawn(
                        save::uploadGameVars(),
                        [](WebRes res) {
                            if (res.isOk()) {
                                Notification::create("(Ferry) Synced settings data", NotificationIcon::Success)->show();
                            } else {
                                log::error("Failed to sync settings data: {}", res.getError());
                                Notification::create("(Ferry) Failed to sync settings data", NotificationIcon::Error)->show();
                            };
                        });
                };
            } break;

            case 2: {  // load
                if (Mod::get()->getSettingValue<bool>("also-download")) {
                    Notification::create("(Ferry) Loading settings data from cloud...", NotificationIcon::Loading)->show();

                    async::spawn(
                        save::downloadGameVars(),
                        [](Result<StringMap<bool>> res) {
                            if (res.isErr()) {
                                log::error("Failed to load settings data: {}", res.unwrapErr());
                                Notification::create("(Ferry) Failed to load settings data", NotificationIcon::Error)->show();

                                return;
                            };

                            save::applyGameVars(res.unwrap());

                            Notification::create("(Ferry) Loaded settings data", NotificationIcon::Success)->show();
                        });
                };
            } break;
        };

        AccountLayer::FLAlert_Clicked(layer, btn2);
    };
};
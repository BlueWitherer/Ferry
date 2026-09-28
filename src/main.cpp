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
                if (Mod::get()->getSettingValue<bool>("also-upload")) async::spawn(upload());
            } break;

            case 2: {  // load
                if (Mod::get()->getSettingValue<bool>("also-download")) async::spawn(download());
            } break;
        };

        AccountLayer::FLAlert_Clicked(layer, btn2);
    };

    arc::Future<> upload() {
        if (Mod::get()->getSettingValue<bool>("auto-gamevars")) {
            async::waitForMainThread([]() {
                Notification::create("(Ferry) Syncing settings to cloud...", NotificationIcon::Loading)->show();
            });

            auto const gvRes = co_await save::uploadGameVars();

            if (gvRes.isOk()) {
                Notification::create("(Ferry) Synced settings data", NotificationIcon::Success)->show();
            } else {
                log::error("Failed to sync settings data: {}", gvRes.getError());
                Notification::create("(Ferry) Failed to sync settings", NotificationIcon::Error)->show();
            };
        };

        if (Mod::get()->getSettingValue<bool>("auto-geode")) {
            async::waitForMainThread([]() {
                Notification::create("(Ferry) Syncing Geode settings to cloud...", NotificationIcon::Loading)->show();
            });

            auto const gvRes = co_await save::geode::uploadSettings();

            if (gvRes.isOk()) {
                Notification::create("(Ferry) Synced Geode settings data", NotificationIcon::Success)->show();
            } else {
                log::error("Failed to sync Geode settings data: {}", gvRes.getError());

                async::waitForMainThread([]() {
                    Notification::create("(Ferry) Failed to sync Geode settings", NotificationIcon::Error)->show();
                });
            };
        };

        co_return;
    };

    arc::Future<> download() {
        if (Mod::get()->getSettingValue<bool>("auto-gamevars")) {
            async::waitForMainThread([]() {
                Notification::create("(Ferry) Loading settings from cloud...", NotificationIcon::Loading)->show();
            });

            auto const res = co_await save::downloadGameVars();

            if (res.isErr()) {
                log::error("Failed to load settings data: {}", res.unwrapErr());
                async::waitForMainThread([]() {
                    Notification::create("(Ferry) Failed to load settings", NotificationIcon::Error)->show();
                });

                co_return;
            };

            save::applyGameVars(res.unwrap());

            async::waitForMainThread([]() {
                Notification::create("(Ferry) Loaded settings", NotificationIcon::Success)->show();
            });
        };

        if (Mod::get()->getSettingValue<bool>("auto-geode")) {
            async::waitForMainThread([]() {
                Notification::create("(Ferry) Loading Geode settings from cloud...", NotificationIcon::Loading)->show();
            });

            auto res = co_await save::geode::downloadSettings();

            if (res.isErr()) {
                log::error("Failed to load Geode settings data: {}", res.unwrapErr());
                async::waitForMainThread([]() {
                    Notification::create("(Ferry) Failed to load Geode settings", NotificationIcon::Error)->show();
                });

                co_return;
            };

            save::geode::applySettings(CW_GEODE_ID, save::geode::filterSettings(CW_GEODE_ID, std::move(res).unwrap()));

            async::waitForMainThread([]() {
                Notification::create("(Ferry) Loaded Geode settings", NotificationIcon::Success)->show();
            });
        };

        co_return;
    };
};

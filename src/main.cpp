#include <Util.h>

#include <ranges>

#include <Geode/Geode.hpp>

#include <Geode/modify/MenuLayer.hpp>
#include <Geode/modify/AccountLayer.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

namespace cw::ferry {
    namespace main {
        static asp::SmallVec<std::shared_ptr<Hook>, 1> g_hooks;

        static void toggleHooks(bool on) {
            for (auto& hook : g_hooks) (void)hook->toggle(on);
        };

        static void setupHooks(auto& self, std::string_view settingID) {
            StringMap<std::shared_ptr<Hook>> const& hooks = self.m_hooks;
            auto enable = Mod::get()->getSettingValue<bool>(settingID);

            for (auto& hook : hooks | std::views::values) {
                hook->setAutoEnable(enable);
                (void)hook->toggle(enable);

                g_hooks.push_back(hook);
            };
        };
    };
};

$on_game(Loaded) {
    log::debug("Using web API url: {}", url::apiBase);

    ButtonSettingPressedEventV3(Mod::get(), "btn")
        .listen([](std::string_view buttonKey) {
            if (!CCScene::get()->getChildByID("sync-menu"_spr)) SyncPopup::create()->show();
        })
        .leak();

    listenForSettingChanges<bool>(
        "menu-btn",
        [](bool value) {
            main::toggleHooks(value);
        });
};

class $modify(FerryMenuLayer, MenuLayer) {
    static void onModify(auto& self) {
        main::setupHooks(self, "menu-btn");
    };

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
                if (Mod::get()->getSettingValue<bool>("also-upload")) async::spawn(upload(), []() {});
            } break;

            case 2: {  // load
                if (Mod::get()->getSettingValue<bool>("also-download")) async::spawn(download(), []() {});
            } break;
        };

        AccountLayer::FLAlert_Clicked(layer, btn2);
    };

    arc::Future<> upload() {
        if (Mod::get()->getSettingValue<bool>("auto-gamevars")) {
            co_await async::waitForMainThread([]() {
                Notification::create("(Ferry) Syncing settings to cloud...", NotificationIcon::Loading)->show();
            });

            auto const gvRes = co_await save::uploadGameVars();

            if (gvRes.isOk()) {
                co_await async::waitForMainThread([]() { Notification::create("(Ferry) Synced settings data", NotificationIcon::Success)->show(); });
            } else {
                log::error("Failed to sync settings data: {}", gvRes.getError());
                co_await async::waitForMainThread([]() { Notification::create("(Ferry) Failed to sync settings", NotificationIcon::Error)->show(); });
            };
        };

        if (Mod::get()->getSettingValue<bool>("auto-geode")) {
            co_await async::waitForMainThread([]() { Notification::create("(Ferry) Syncing Geode settings to cloud...", NotificationIcon::Loading)->show(); });

            auto const gvRes = co_await save::geode::uploadSettings();

            if (gvRes.isOk()) {
                co_await async::waitForMainThread([]() { Notification::create("(Ferry) Synced Geode settings data", NotificationIcon::Success)->show(); });
            } else {
                log::error("Failed to sync Geode settings data: {}", gvRes.getError());
                co_await async::waitForMainThread([]() { Notification::create("(Ferry) Failed to sync Geode settings", NotificationIcon::Error)->show(); });
            };
        };

        co_return;
    };

    arc::Future<> download() {
        if (Mod::get()->getSettingValue<bool>("auto-gamevars")) {
            co_await async::waitForMainThread([]() { Notification::create("(Ferry) Loading settings from cloud...", NotificationIcon::Loading)->show(); });

            auto const res = co_await save::downloadGameVars();

            if (res.isErr()) {
                log::error("Failed to load settings data: {}", res.unwrapErr());
                co_await async::waitForMainThread([]() { Notification::create("(Ferry) Failed to load settings", NotificationIcon::Error)->show(); });

                co_return;
            };

            save::applyGameVars(res.unwrap());

            co_await async::waitForMainThread([]() { Notification::create("(Ferry) Loaded settings", NotificationIcon::Success)->show(); });
        };

        if (Mod::get()->getSettingValue<bool>("auto-geode")) {
            co_await async::waitForMainThread([]() { Notification::create("(Ferry) Loading Geode settings from cloud...", NotificationIcon::Loading)->show(); });

            auto res = co_await save::geode::downloadSettings();

            if (res.isErr()) {
                log::error("Failed to load Geode settings data: {}", res.unwrapErr());
                co_await async::waitForMainThread([]() { Notification::create("(Ferry) Failed to load Geode settings", NotificationIcon::Error)->show(); });

                co_return;
            };

            save::geode::applySettings(CW_GEODE_ID, save::geode::filterSettings(CW_GEODE_ID, std::move(res).unwrap()));

            co_await async::waitForMainThread([]() { Notification::create("(Ferry) Loaded Geode settings", NotificationIcon::Success)->show(); });
        };

        co_return;
    };
};

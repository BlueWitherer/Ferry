#include "../SyncPopup.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

namespace cw::ferry {
    namespace util {
        template <typename T>  // thx cue owo
            requires(std::derived_from<T, cocos2d::CCNode>)
        static void resetNode(Ref<T>& node) {
            if (node) {
                node->removeFromParent();
                node = nullptr;
            };
        };

        static void removeOptionsLayer() {
            if (auto mol = CCScene::get()->getChildByID("MoreOptionsLayer")) mol->removeFromParent();
        };
    };
};

bool SyncPopup::init() {
    if (!Popup::init({250.f, 135.f})) return false;

    setID("sync-popup"_spr);
    setTitle("Ferry Save Manager");

    addSideArt(m_mainLayer, SideArt::All, SideArtStyle::PopupGold);

    auto discordBtn = Button::createWithSpriteFrameName(
        "gj_discordIcon_001.png",
        [](auto) {
            createQuickPopup(
                "Discord Community",
                "Join <cd>Cheeseworks</c>'s <cb>Discord server</c>?\n"
                "<cs>Get help, report bugs, and chat with other players!</c>",
                "Cancel",
                "OK",
                [](auto, bool ok) {
                    if (ok) web::openLinkInBrowser("https://www.dsc.gg/cheeseworks");
                });
        });
    discordBtn->setID("discord-btn");
    discordBtn->setScale(0.75f);
    discordBtn->setZOrder(1);

    m_mainLayer->addChildAtPosition(discordBtn, Anchor::BottomLeft, {15.f, 15.f}, false);

    auto infoBtn = Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [](auto) {
            createQuickPopup(
                "Help",
                "This is the <cg>Data Sync Pop-up</c>. Here, you can <cy>upload and download the game settings linked to your Geometry Dash account</c>.",
                "OK",
                nullptr,
                nullptr);
        });
    infoBtn->setID("info-btn");
    infoBtn->setScale(0.75f);
    infoBtn->setZOrder(9);

    m_mainLayer->addChildAtPosition(infoBtn, Anchor::TopRight, {-12.5f, -12.5f}, false);

    auto btns = std::to_array<SaveButtonData>(
        {
            {
                "upload-btn",
                "Upload Settings",
                "GJ_sRecentIcon_001.png",
                "GJ_button_03.png",
                [this](auto) {
                    startUploadTask(nullptr);
                },
            },
            {
                "download-btn",
                "Download Settings",
                "GJ_sDownloadIcon_001.png",
                "GJ_button_01.png",
                [this](auto) {
                    startDownloadTask(nullptr);
                },
            },
        });

    auto menuLayout = ColumnLayout::create()
                          ->setGap(7.5f)
                          ->setAutoScale(false)
                          ->setAutoGrowAxis(10.f)
                          ->setAxisReverse(true);

    auto menu = CCNode::create();
    menu->setID("btn-container");
    menu->setAnchorPoint({0.5, 0.5});
    menu->setContentSize({65.f, 65.f});
    menu->setLayout(menuLayout);

    m_mainLayer->addChildAtPosition(menu, Anchor::Center, {0.f, -8.75f}, false);

    for (auto& b : btns) {
        auto btnSprsLayout = RowLayout::create()
                                 ->setGap(2.5f)
                                 ->setAutoScale(false)
                                 ->setAutoGrowAxis(65.f);

        auto btnSprs = CCNode::create();
        btnSprs->setAnchorPoint({0.5, 0.5});
        btnSprs->setContentSize({65.f, 65.f});
        btnSprs->setLayout(btnSprsLayout);

        auto btnSprIcon = CCSprite::createWithSpriteFrameName(b.icon.c_str());
        btnSprIcon->setScale(0.75f);

        auto btnSprLabel = Label::create(std::move(b.text), "bigFont.fnt");
        btnSprLabel->setScale(0.475f);
        btnSprLabel->setAlignment(Label::Alignment::Center);

        btnSprs->addChild(btnSprIcon);
        btnSprs->addChild(btnSprLabel);

        btnSprs->updateLayout();

        auto btnSpr = NineSlice::create(b.background);
        btnSpr->setContentSize({btnSprs->getScaledContentWidth() + 10.f, btnSprs->getScaledContentHeight() + 12.5f});

        btnSpr->addChildAtPosition(btnSprs, Anchor::Center);

        auto btn = Button::createWithNode(
            btnSpr,
            std::move(b.callback));
        btn->setID(std::move(b.id));
        btn->setScale(0.875f);

        menu->addChild(btn);
    };

    menu->updateLayout();

    return true;
};

void SyncPopup::startUploadTask(Callback&& cb) {
    m_inProgress = true;
    m_downloadTask.cancel();

    util::removeOptionsLayer();
    util::resetNode(m_progressPopup);

    m_progressPopup = UploadActionPopup::create(this, "Uploading settings data...");
    m_progressPopup->show();

    m_uploadTask.spawn(
        SaveManager::get()->uploadGameVars(),
        [this](WebRes res) {
            m_inProgress = false;

            if (res.isOk()) {
                m_progressPopup->showSuccessMessage("Data saved to cloud!");
            } else {
                m_progressPopup->showFailMessage("Data save failed.");
                log::error("Couldn't save data: {}", res.getError());
            };
        });
};

void SyncPopup::startDownloadTask(Callback&& cb) {
    m_inProgress = true;
    m_uploadTask.cancel();

    util::removeOptionsLayer();
    util::resetNode(m_progressPopup);

    m_progressPopup = UploadActionPopup::create(this, "Downloading settings data...");
    m_progressPopup->show();

    m_downloadTask.spawn(
        SaveManager::get()->downloadGameVars(),
        [this](Result<StringMap<bool>> res) {
            auto const fallback = [this](std::string_view err) {
                m_inProgress = false;
                m_progressPopup->showFailMessage("Data save failed.");

                log::error("Couldn't apply settings data: {}", err);
            };

            if (res.isErr()) return fallback(res.unwrapErr());
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

            m_inProgress = false;
            m_progressPopup->showSuccessMessage("Data synced!");
        });
};

void SyncPopup::onClosePopup(UploadActionPopup* popup) {
    if (!popup->m_succeeded) {
        m_uploadTask.cancel();
        m_downloadTask.cancel();

        if (m_inProgress) Notification::create("Task cancelled", NotificationIcon::Error)->show();
    };

    util::resetNode(m_progressPopup);
    m_inProgress = false;
};

void SyncPopup::onExit() {
    m_uploadTask.cancel();
    m_downloadTask.cancel();

    util::resetNode(m_progressPopup);

    Popup::onExit();
};

SyncPopup* SyncPopup::create() {
    auto ret = new SyncPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};
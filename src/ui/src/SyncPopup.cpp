#include "../SyncPopup.hpp"

#include <Util.h>

#include <ranges>

#include <Geode/Geode.hpp>

#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

namespace cw::ferry {
    namespace impl {
        template <typename T>  // thx cue owo
            requires(std::derived_from<T, cocos2d::CCNode>)
        static void resetNode(Ref<T>& node) {
            if (node) {
                node->removeFromParent();
                node = nullptr;
            };
        };

        void rescale(CCNode* node, float targetSize) {
            auto size = node->getScaledContentSize();
            auto scale = targetSize / size.height;

            node->setScale(scale);
        };

        static void removeOptionsLayer() {
            if (auto mol = CCScene::get()->getChildByID("MoreOptionsLayer")) mol->removeFromParent();
        };

        static constexpr auto getSyncTypeName(SyncType type) noexcept {
            switch (type) {
                default: [[fallthrough]];

                case SyncType::GameSettings: return "Game Settings";
                case SyncType::GeodeSettings: return "Geode Settings";
                case SyncType::ModSettings: return "Mods' Settings";
                case SyncType::ModSaves: return "Mods' Save Data";
            };
        };

        static constexpr auto getSyncTypeID(SyncType type) noexcept {
            switch (type) {
                default: [[fallthrough]];

                case SyncType::GameSettings: return "gd-settings";
                case SyncType::GeodeSettings: return "geode-settings";
                case SyncType::ModSettings: return "mods-settings";
                case SyncType::ModSaves: return "mods-save-data";
            };
        };
    };
};

bool SyncSelect::init(SyncType type, Callback&& cb) {
    m_type = type;
    m_callback = std::move(cb);

    if (!CCMenu::init()) return false;

    setID(impl::getSyncTypeID(m_type));
    setContentSize({50.f, 32.5f});
    setAnchorPoint({0.5, 0.5});

    auto bg = NineSlice::create("geode.loader/black-square.png");
    bg->setZOrder(-9);
    bg->setContentSize(getScaledContentSize());

    addChildAtPosition(bg, Anchor::Center);

    auto toggle = CCMenuItemExt::createTogglerWithStandardSprites(
        0.825f,
        [this](CCMenuItemToggler* sender) {
            auto on = !sender->isToggled();

            mod->setSavedValue(impl::getSyncTypeID(m_type), on);
            return m_callback(m_type, on);
        });
    toggle->setID(fmt::format("{}-toggler", impl::getSyncTypeID(m_type)));
    toggle->setZOrder(9);

    addChildAtPosition(toggle, Anchor::Left, {toggle->getScaledContentWidth() * 0.625f, 0.f});

    auto saved = mod->getSavedValue<bool>(impl::getSyncTypeID(m_type), m_type == SyncType::GameSettings);

    toggle->toggle(saved);
    m_callback(type, saved);

    auto label = Label::create(impl::getSyncTypeName(m_type), "bigFont.fnt");
    label->setID("name-label");
    label->setScale(0.375f);
    label->setAnchorPoint({0, 0.5});
    label->setMaxWidth(57.5f);

    addChildAtPosition(label, Anchor::Left, {toggle->getScaledContentWidth() * 1.25f, 0.f});

    setContentWidth((toggle->getScaledContentWidth() * 1.125f) + label->getScaledContentWidth() * 1.125f);

    bg->setContentSize(getScaledContentSize());

    updateLayout();

    return true;
};

SyncSelect* SyncSelect::create(SyncType type, Callback&& cb) {
    auto ret = new SyncSelect();
    if (ret->init(type, std::move(cb))) {
        ret->autorelease();
        return ret;
    };

    delete ret;
    return nullptr;
};

bool SyncPopup::init() {
    if (!Popup::init({280.f, 235.f})) return false;

    setID("sync-menu"_spr);
    setTitle("Ferry");

    auto menuLayout = ColumnLayout::create()
                          ->setGap(6.25f)
                          ->setAutoScale(false)
                          ->setAutoGrowAxis(10.f)
                          ->setAxisReverse(true);

    auto menu = CCNode::create();
    menu->setID("btn-container");
    menu->setAnchorPoint({0.5, 0.5});
    menu->setContentSize({170.f, 65.f});
    menu->setLayout(menuLayout);

    m_mainLayer->addChildAtPosition(menu, Anchor::Center, {0.f, 30.f});

    auto btns = std::array{
        SaveButtonData{
            "upload-btn",
            "Sync to Cloud",
            "d_artCloud_01_001.png",
            "GJ_button_02.png",
            [this](auto) {
                if (m_toSync.lock()->empty()) return Notification::create("No option selected", NotificationIcon::Error)->show();

                createQuickPopup(
                    "Upload Data",
                    "Sync current settings <cy>with the cloud</c>?\n"
                    "<cr>Currently saved data will be overwritten</c>.",
                    "Cancel",
                    "Yes",
                    [this](auto, bool ok) {
                        if (ok) {
                            m_inProgress = true;

                            impl::removeOptionsLayer();
                            impl::resetNode(m_progressPopup);

                            m_progressPopup = UploadActionPopup::create(this, "Preparing upload...");
                            m_progressPopup->show();

                            m_tasks.spawn(
                                runUploadTasks(),
                                [this](Result<> res) {
                                    m_inProgress = false;

                                    if (res.isErr()) {
                                        log::error("Upload(s) failed: {}", res.unwrapErr());
                                        m_progressPopup->showFailMessage("An error occurred");
                                    } else {
                                        m_progressPopup->showSuccessMessage("Sync complete!");
                                    };
                                });
                        };
                    });
            },
        },
        SaveButtonData{
            "download-btn",
            "Load to Game",
            "geode.loader/install.png",
            "GJ_button_01.png",
            [this](auto) {
                if (m_toSync.lock()->empty()) return Notification::create("No option selected", NotificationIcon::Error)->show();

                createQuickPopup(
                    "Download Data",
                    "Sync cloud-saved settings <cg>to your game</c>?\n"
                    "<cr>Current game settings will be overridden</c>.",
                    "Cancel",
                    "Yes",
                    [this](auto, bool ok) {
                        if (ok) {
                            m_inProgress = true;

                            impl::removeOptionsLayer();
                            impl::resetNode(m_progressPopup);

                            m_progressPopup = UploadActionPopup::create(this, "Preparing download...");
                            m_progressPopup->show();

                            m_tasks.spawn(
                                runDownloadTasks(),
                                [this](Result<> res) {
                                    m_inProgress = false;

                                    if (res.isErr()) {
                                        log::error("Download(s) failed: {}", res.unwrapErr());
                                        m_progressPopup->showFailMessage("An error occurred");
                                    } else {
                                        m_progressPopup->showSuccessMessage("Sync complete!");
                                    };
                                });
                        };
                    });
            },
        },
    };

    for (auto& b : btns) {
        auto btnSprsLayout = RowLayout::create()
                                 ->setGap(2.5f)
                                 ->setAutoScale(false)
                                 ->setAutoGrowAxis(65.f);

        auto btnSprs = CCNode::create();
        btnSprs->setAnchorPoint({0.5, 0.5});
        btnSprs->setContentSize({menu->getScaledContentWidth() - 15.f, 65.f});
        btnSprs->setLayout(btnSprsLayout);

        auto btnSprIcon = CCSprite::createWithSpriteFrameName(b.icon.c_str());

        impl::rescale(btnSprIcon, 12.5f);

        auto btnSprLabel = Label::create(std::move(b.text), "bigFont.fnt");
        btnSprLabel->setScale(0.475f);
        btnSprLabel->setAlignment(Label::Alignment::Center);

        btnSprs->addChild(btnSprIcon);
        btnSprs->addChild(btnSprLabel);

        btnSprs->updateLayout();

        auto btnSpr = NineSlice::create(b.background);
        btnSpr->setContentSize({menu->getScaledContentWidth() - 12.5f, btnSprs->getScaledContentHeight() + 12.5f});

        btnSpr->addChildAtPosition(btnSprs, Anchor::Center);

        auto btn = Button::createWithNode(
            btnSpr,
            std::move(b.callback));
        btn->setID(std::move(b.id));
        btn->setScale(0.875f);
        btn->setScaleMultiplier(1.125f);

        menu->addChild(btn);
    };

    menu->updateLayout();

    auto infoLabel = Label::createRich(
        "Sync your <cg>game settings</c> with <cf>Ferry's cloud service</c>.\n"
        "Saving or loading data will always result in <cr>overrides</c>.",
        "geode.loader/mdFontB.fnt");
    infoLabel->setScale(0.4f);
    infoLabel->setAlignment(Label::Alignment::Center);
    infoLabel->setAnchorPoint({0.5, 1});

    m_mainLayer->addChildAtPosition(infoLabel, Anchor::Top, {0.f, -32.5f});

    auto gjam = GJAccountManager::sharedState();

    auto loginLabel = Label::createRich(
        fmt::format(
            "Saved data from <cf>Ferry</c> is linked to your GD account.\n"
            "Logged in as <cc>{}</c>.",
            gjam->m_username),
        "chatFont.fnt");
    loginLabel->setScale(0.5f);
    loginLabel->setAlignment(Label::Alignment::Center);
    loginLabel->setAnchorPoint({0.5, 0});

    m_mainLayer->addChildAtPosition(loginLabel, Anchor::Bottom, {0.f, 12.5f});

    auto toggleMenuLayout = RowLayout::create()
                                ->setGap(3.75f)
                                ->setAutoScale(false)
                                ->setAxisAlignment(AxisAlignment::Even)
                                ->setGrowCrossAxis(true);

    auto toggleMenu = CCNode::create();
    toggleMenu->setID("toggle-container");
    toggleMenu->setZOrder(1);
    toggleMenu->setAnchorPoint({0.5, 0.5});
    toggleMenu->setContentSize({m_mainLayer->getScaledContentWidth() * 0.625f, 32.5f});
    toggleMenu->setLayout(toggleMenuLayout);

    m_mainLayer->addChildAtPosition(toggleMenu, Anchor::Center, {0.f, -52.5f});

    static constexpr SyncType types[] = {
        SyncType::GameSettings,
        SyncType::GeodeSettings,
        SyncType::ModSettings,
    };

    for (auto t : types) {
        auto toggle = SyncSelect::create(t, [this](SyncType t, bool on) {
            auto toSync = m_toSync.lock();
            auto i = static_cast<uint8_t>(t);

            if (on) {
                (*toSync)[i] = t;
            } else {
                if (auto const it = toSync->find(i); it != toSync->end()) toSync->erase(it);
            };
        });
        toggle->setScale(0.825f);

        toggleMenu->addChild(toggle);
    };

    toggleMenu->updateLayout();

    auto toggleMenuLabel = Label::create("What data would you like to sync?", "chatFont.fnt");
    toggleMenuLabel->setScale(0.625f);
    toggleMenuLabel->setAlignment(Label::Alignment::Center);

    m_mainLayer->addChildAtPosition(toggleMenuLabel, Anchor::Center, {0.f, -40.f + (toggleMenu->getScaledContentHeight() * 0.5f)});

    auto linkBtnMenuLayout = ColumnLayout::create()
                                 ->setGap(2.f)
                                 ->setAutoScale(false)
                                 ->setAutoGrowAxis(0.f);

    auto linkBtnMenu = CCNode::create();
    linkBtnMenu->setID("link-container");
    linkBtnMenu->setContentSize({12.5f, 1.25f});
    linkBtnMenu->setZOrder(1);
    linkBtnMenu->setLayout(linkBtnMenuLayout);

    m_mainLayer->addChildAtPosition(linkBtnMenu, Anchor::BottomLeft, {5.f, 5.f});

    auto linkBtns = std::array{
        LinkButton{
            "discord-btn",
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
            },
        },
        LinkButton{
            "support-me-btn",
            "geode.loader/gift.png",
            [](auto) {
                openSupportPopup(mod);
            },
        },
    };

    for (auto& linkBtn : linkBtns) {
        auto b = Button::createWithSpriteFrameName(
            linkBtn.sprite,
            std::move(linkBtn.callback));
        b->setID(std::move(linkBtn.id));
        b->setScale(0.75f);

        linkBtnMenu->addChild(b);
    };

    linkBtnMenu->updateLayout();

    auto infoBtn = Button::createWithSpriteFrameName(
        "GJ_infoIcon_001.png",
        [](auto) {
            createQuickPopup(
                "Help",
                "This is the <cg>Ferry Sync Menu</c>. You can <cy>upload and download your game settings data</c> here, which is linked to <cg>your Geometry Dash account</c>.",
                "OK",
                nullptr,
                nullptr);
        });
    infoBtn->setID("info-btn");
    infoBtn->setScale(0.75f);
    infoBtn->setZOrder(9);

    m_mainLayer->addChildAtPosition(infoBtn, Anchor::TopRight, {-15.f, -15.f});

    auto settingsBtn = Button::createWithNode(
        CircleButtonSprite::createWithSpriteFrameName(
            "geode.loader/settings.png"),
        [](auto) {
            openSettingsPopup(mod);
        });
    settingsBtn->setID("mod-settings-btn");
    settingsBtn->setScale(0.625f);

    m_mainLayer->addChildAtPosition(settingsBtn, Anchor::BottomRight);

    return true;
};

asp::SmallVec<SyncType, 4> SyncPopup::getSyncTypes() const {
    auto toSync = m_toSync.lock();

    asp::SmallVec<SyncType, 4> out;
    for (auto const& type : *toSync | std::views::values) out.push_back(type);

    return out;
};

arc::Future<Result<>> SyncPopup::runTaskForIndex(uint8_t i, bool upload) {
    switch (static_cast<SyncType>(i)) {
        default: co_return Err("Unknown error");

        case SyncType::GameSettings: co_return co_await (upload ? startGVUploadTask() : startGVDownloadTask());
        case SyncType::GeodeSettings: co_return co_await (upload ? startGeodeUploadTask() : startGeodeDownloadTask());
        case SyncType::ModSettings: co_return co_await (upload ? startModUploadTask() : startModDownloadTask());
    };
};

arc::Future<Result<>> SyncPopup::runUploadTasks() {
    auto toSync = m_toSync.lock();
    for (auto const& t : *toSync | std::views::keys) {
        GEODE_CO_UNWRAP(co_await runTaskForIndex(t, true));
    };

    co_return Ok();
};

arc::Future<Result<>> SyncPopup::runDownloadTasks() {
    auto toSync = m_toSync.lock();
    for (auto const& t : *toSync | std::views::keys) {
        GEODE_CO_UNWRAP(co_await runTaskForIndex(t));
    };

    co_return Ok();
};

arc::Future<Result<>> SyncPopup::startGVUploadTask() {
    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Uploading game settings data..."); });

    auto const res = co_await save::uploadGameVars();
    if (res.isErr()) co_return Err(res.getError());

    co_return Ok();
};

arc::Future<Result<>> SyncPopup::startGVDownloadTask() {
    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Downloading game settings data..."); });

    GEODE_CO_UNWRAP_INTO(auto const res, co_await save::downloadGameVars());

    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Applying game settings..."); });

    save::applyGameVars(res);
    co_return Ok();
};

arc::Future<Result<>> SyncPopup::startGeodeUploadTask() {
    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Uploading Geode settings data..."); });

    auto const res = co_await save::geode::uploadSettings();
    if (res.isErr()) co_return Err(res.getError());

    co_return Ok();
};

arc::Future<Result<>> SyncPopup::startGeodeDownloadTask() {
    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Downloading Geode settings data..."); });

    GEODE_CO_UNWRAP_INTO(auto const res, co_await save::geode::downloadSettings());

    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Applying Geode settings..."); });

    save::geode::applySettings(CW_GEODE_ID, save::geode::filterSettings(CW_GEODE_ID, res));
    co_return Ok();
};

arc::Future<Result<>> SyncPopup::startModUploadTask() {
    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Uploading mod settings data..."); });

    auto const res = co_await save::geode::mods::uploadSettings();
    if (res.isErr()) co_return Err(res.getError());

    co_return Ok();
};

arc::Future<Result<>> SyncPopup::startModDownloadTask() {
    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Downloading mod settings data..."); });

    GEODE_CO_UNWRAP_INTO(auto const res, co_await save::geode::mods::downloadSettings());

    co_await async::waitForMainThread([this]() { m_progressPopup->m_textArea->setString("Applying mod settings..."); });

    for (auto const& [modID, data] : res) {
        save::geode::applySettings(modID, save::geode::filterSettings(modID, data));
    };

    co_return Ok();
};

void SyncPopup::onClosePopup(UploadActionPopup* popup) {
    if (!popup->m_succeeded) {
        m_tasks.cancel();

        if (m_inProgress) Notification::create("Task cancelled", NotificationIcon::Error)->show();
    };

    impl::resetNode(m_progressPopup);
    m_inProgress = false;
};

void SyncPopup::onExit() {
    m_tasks.cancel();

    impl::resetNode(m_progressPopup);

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
#pragma once

#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::ferry {
    enum class SyncType : uint8_t {
        GameSettings,
        GeodeSettings,
        ModSettings,
        ModSaves,
    };

    namespace ui {

        class SyncPopup final : public geode::Popup, public UploadPopupDelegate {
            struct SaveButtonData final {
                std::string id;
                std::string text;
                std::string icon;
                std::string background;
                geode::Button::ButtonCallback callback;
            };

            struct LinkButton final {
                std::string id;
                std::string sprite;
                geode::Button::ButtonCallback callback;
            };

        private:
            bool m_inProgress = false;
            geode::Ref<UploadActionPopup> m_progressPopup = nullptr;

            geode::async::TaskHolder<WebRes> m_uploadTask;
            geode::async::TaskHolder<geode::Result<geode::utils::StringMap<bool>>> m_downloadVarsTask;
            geode::async::TaskHolder<geode::Result<matjson::Value>> m_downloadSettingsTask;

            std::unordered_set<SyncType> m_toSync;

            asp::SmallVec<SyncType, 4> getSyncTypes() const;

        protected:
            void startUploadTasks();
            void startDownloadTasks();

            void startGVUploadTask();
            void startGVDownloadTask();

            void startGeodeUploadTask();
            void startGeodeDownloadTask();

            void onClosePopup(UploadActionPopup* popup) override;

            void onExit() override;

            bool init() override;

        public:
            static SyncPopup* create();
        };

        class SyncSelect final : public cocos2d::CCMenu {
            using Callback = geode::Function<void(SyncType, bool)>;

        private:
            SyncType m_type;
            Callback m_callback = nullptr;

        protected:
            void onToggle(cocos2d::CCObject* sender);

            bool init(SyncType type, Callback&& cb);

        public:
            static SyncSelect* create(SyncType type, Callback&& cb);
        };
    };
};
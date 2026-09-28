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
            std::atomic<bool> m_inProgress = false;
            geode::Ref<UploadActionPopup> m_progressPopup = nullptr;

            geode::async::TaskHolder<geode::Result<>> m_tasks;

            asp::Mutex<std::map<uint8_t, SyncType>> m_toSync;

            asp::SmallVec<SyncType, 4> getSyncTypes() const;

            arc::Future<geode::Result<>> runTaskForIndex(uint8_t i, bool upload = false);

            arc::Future<geode::Result<>> startGVUploadTask();
            arc::Future<geode::Result<>> startGVDownloadTask();

            arc::Future<geode::Result<>> startGeodeUploadTask();
            arc::Future<geode::Result<>> startGeodeDownloadTask();

        protected:
            arc::Future<geode::Result<>> runUploadTasks();
            arc::Future<geode::Result<>> runDownloadTasks();

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
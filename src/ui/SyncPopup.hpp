#pragma once

#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::ferry {
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
        geode::async::TaskHolder<geode::Result<geode::utils::StringMap<bool>>> m_downloadTask;

    protected:
        void startUploadTask();
        void startDownloadTask();

        void onClosePopup(UploadActionPopup* popup) override;

        void onExit() override;

        bool init() override;

    public:
        static SyncPopup* create();
    };
};
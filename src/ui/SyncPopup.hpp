#pragma once

#include <util/WebRes.hpp>

#include <Geode/Geode.hpp>

namespace cw::ferry {
    class SyncPopup final : public geode::Popup {
        using Callback = geode::CopyableFunction<void(WebRes)>;

    private:
        geode::Ref<UploadActionPopup> m_progressPopup = nullptr;

        geode::async::TaskHolder<WebRes> m_uploadTask;
        geode::async::TaskHolder<geode::Result<geode::utils::StringMap<bool>>> m_downloadTask;

    protected:
        void startUploadTask(Callback&& cb);
        void startDownloadTask(Callback&& cb);

        void onExit() override;

        bool init() override;

    public:
        static SyncPopup* create();
    };
};
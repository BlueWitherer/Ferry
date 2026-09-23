#pragma once

#include "base/Singleton.hpp"

#include "WebRes.hpp"

#include <Geode/Geode.hpp>

namespace cw::ferry {
    class SaveManager final : public base::Singleton<SaveManager> {
    private:
        geode::async::TaskHolder<geode::utils::web::WebResponse> m_gvSyncTask;

    public:
        arc::Future<WebRes> uploadGameVars();
    };
};
#pragma once

#include "WebRes.hpp"

#include <Geode/Geode.hpp>

namespace cw::ferry {
    namespace save {
        arc::Future<WebRes> uploadGameVars();
        arc::Future<geode::Result<geode::utils::StringMap<bool>>> downloadGameVars();

        namespace geode {
            arc::Future<WebRes> uploadSettings();
            arc::Future<::geode::Result<matjson::Value>> downloadSettings();

            void applySettings(std::string_view modID, matjson::Value const& data);
            void applySettings(::geode::Mod* mod, matjson::Value data);

            matjson::Value filterSettings(std::string_view modID, matjson::Value const& data);
            matjson::Value filterSettings(::geode::Mod* mod, matjson::Value const& data);

            matjson::Value& getSettings(std::string_view modID);
            matjson::Value& getSettings(::geode::Mod* mod);

            namespace mods {
                arc::Future<WebRes> uploadSettings();
                arc::Future<::geode::Result<matjson::Value>> downloadSettings();

                matjson::Value getAllSettings();
            };
        };

        void applyGameVars(::geode::utils::StringMap<bool> const& vars);
    };
};
#pragma once

#include <Geode/Geode.hpp>

namespace cw::ferry {
    struct WebRes final {
    private:
        matjson::Value m_payload;
        std::string m_error;

    public:
        WebRes(matjson::Value payload = std::nullptr_t(), std::string error = "");

        template <typename T>
        geode::Result<T> getPayload() const {
            return m_payload.as<T>();
        };

        matjson::Value const& getPayloadValue() const noexcept;
        geode::ZStringView getError() const noexcept;

        bool isOk() const noexcept;
        bool isErr() const noexcept;
    };

    namespace webres {
        WebRes processResp(geode::utils::web::WebResponse res);
    };
};

template <>
struct matjson::Serialize<cw::ferry::WebRes> final {
    static geode::Result<cw::ferry::WebRes> fromJson(matjson::Value const& value);
    static matjson::Value toJson(cw::ferry::WebRes const& value);
};
#include "../SaveManager.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

arc::Future<WebRes> SaveManager::uploadGameVars() {
    auto tokenRes = co_await argon::startAuth();
    if (tokenRes.isErr()) co_return WebRes(std::nullptr_t(), std::move(tokenRes).unwrapErr(), 402);

    auto token = std::move(tokenRes).unwrap();

    auto const accountID = *co_await async::waitForMainThread<int>([]() {
        return GJAccountManager::sharedState()->m_accountID;
    });

    auto vars = *co_await async::waitForMainThread<StringMap<bool>>([]() {
        StringMap<bool> vars;

        auto gm = GameManager::sharedState();

        for (auto const& [key, val] : gm->m_valueKeeper->asExt()) {
            if (str::startsWith(key, "gv_0")) {
                if (auto v = typeinfo_cast<CCString*>(val)) vars[str::filter(key, "0123456789")] = v->boolValue();
            };
        };

        log::info("{}", vars);
        return vars;
    });

    dbuf::ByteWriter bw;

    bw.writeU64(vars.size());

    for (auto const& [str, value] : vars) {
        bw.writeStringU8(str);
        bw.writeBool(value);
    };

    auto res = co_await request::base()
                   .body(bw.writtenVec())
                   .param("account_id", accountID)
                   .param("authtoken", std::move(token))
                   .onProgress([](web::WebProgress const& prog) {
                       auto val = prog.uploadProgress();
                       if (val.has_value()) async::waitForMainThread([&val]() {
                           ProgressEvent().send(std::move(val).value());
                       });
                   })
                   .post("http://localhost:6767/api/v1/upload");  // still testing, ferry.cheeseworks.gay soon!

    auto webResp = webres::processResp(res);

    if (webResp.isOk()) {
        co_await async::waitForMainThread([]() {
            Notification::create("Successfully synced game settings!", NotificationIcon::Success)->show();
        });
    };

    co_return webResp;
};

arc::Future<Result<StringMap<bool>>> SaveManager::downloadGameVars() {
    GEODE_CO_UNWRAP_INTO(std::string token, co_await argon::startAuth());

    auto const accountID = *co_await async::waitForMainThread<int>([]() {
        return GJAccountManager::sharedState()->m_accountID;
    });

    auto res = co_await request::base()
                   .param("account_id", accountID)
                   .param("authtoken", std::move(token))
                   .onProgress([](web::WebProgress const& prog) {
                       auto val = prog.downloadProgress();
                       if (val.has_value()) async::waitForMainThread([&val]() {
                           ProgressEvent().send(std::move(val).value());
                       });
                   })
                   .get("http://localhost:6767/api/v1/download");  // still testing, ferry.cheeseworks.gay soon!

    if (res.error()) {
        auto const webResp = webres::processResp(res);
        co_return Err("{}: {}", webResp.getCode(), webResp.getError());
    };

    dbuf::ByteReader br{res.data()};

    GEODE_CO_UNWRAP_INTO(size_t size, br.readU64());

    log::debug("Received {} game variables", size);
    if (size <= 0) co_return Err("Map stream has invalid size");

    StringMap<bool> vars;
    vars.reserve(size);

    for (size_t i = 0; i < size; ++i) {
        GEODE_CO_UNWRAP_INTO(std::string key, br.readStringU8());
        GEODE_CO_UNWRAP_INTO(bool val, br.readBool());

        vars[std::move(key)] = val;
    };

    co_return Ok(vars);
};

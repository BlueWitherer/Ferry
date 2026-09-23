#include "../SaveManager.hpp"

#include <Util.h>

#include <Geode/Geode.hpp>

using namespace geode::prelude;
using namespace cw::ferry;

arc::Future<WebRes> SaveManager::uploadGameVars() {
    auto tokenRes = co_await argon::startAuth();
    if (tokenRes.isErr()) co_return WebRes(std::nullptr_t(), std::move(tokenRes).unwrapErr());

    auto const token = std::move(tokenRes).unwrap();

    auto const acc = *co_await async::waitForMainThread<argon::AccountData>([]() {
        return argon::getGameAccountData();
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

    auto onProg = [](web::WebProgress const& prog) {
        log::debug("{}%", prog.uploadProgress());
    };

    auto res = co_await request::base()
                   .body(bw.writtenVec())
                   .param("account_id", acc.accountId)
                   .param("authtoken", token)
                   .onProgress(std::move(onProg))
                   .post("http://localhost:6767/api/v1/upload");  // still testing, ferry.cheeseworks.gay soon!

    auto webResp = webres::processResp(res);

    if (webResp.isOk()) {
        co_await async::waitForMainThread([]() {
            Notification::create("Successfully synced game settings!", NotificationIcon::Success)->show();
        });
    };

    co_return webResp;
};
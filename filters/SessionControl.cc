/**
 *
 *  SessionControl.cc
 *
 */

#include "SessionControl.h"

using namespace drogon;

void SessionControl::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    if(req->session()->find("login")){
        fccb();
    } else {
        LOG_DEBUG << "user has not logged in, jump to login page";
        auto res = HttpResponse::newRedirectionResponse("/login");
        fcb(res);
    }
}

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
        auto res = HttpResponse::newRedirectionResponse("/login");
        fcb(res);
    }
}

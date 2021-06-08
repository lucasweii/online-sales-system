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
    if(req->session()->find("loggin")){
        fccb();
    } else {
        auto res = HttpResponse::newRedirectionResponse("/");
        fcb(res);
    }
}

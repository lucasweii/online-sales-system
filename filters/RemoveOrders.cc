/**
 *
 *  RemoveOrders.cc
 *
 */

#include "RemoveOrders.h"

using namespace drogon;

void RemoveOrders::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    //Edit your logic here
    if (1)
    {
        //Passed
        req->session()->erase("orders");
        fccb();
        return;
    }
    //Check failed
    auto res = drogon::HttpResponse::newHttpResponse();
    res->setStatusCode(k500InternalServerError);
    fcb(res);
}

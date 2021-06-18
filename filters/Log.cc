/**
 *
 *  log.cc
 *
 */

#include "Log.h"
using namespace drogon;
void Log::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    auto method = req->getMethod();
    switch (method) {
        case Get:
            LOG_DEBUG << "Get " << req->getPath();
            break;
        case Post:
            LOG_DEBUG << "Post " << req->getPath();
            break;
        case Put:
            LOG_DEBUG << "Put " << req->getPath();
            break;
    }
    fccb();
}

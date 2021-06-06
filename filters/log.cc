/**
 *
 *  log.cc
 *
 */

#include "log.h"
#include <spdlog/spdlog.h>
using namespace drogon;
void log::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    auto method = req->getMethod();
    switch (method) {
        case Get:
            spdlog::info("Get {}", req->getPath());
            break;
        case Post:
            spdlog::info("Post {}", req->getPath());
            break;
        case Put:
            spdlog::info("Put {}", req->getPath());
            break;
    }
    fccb();
}

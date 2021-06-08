/**
 *
 *  log.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;


class Log : public HttpFilter<Log>
{
  public:
    Log() {}
    virtual void doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) override;
};


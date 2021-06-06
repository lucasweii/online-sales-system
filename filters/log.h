/**
 *
 *  log.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;


class log : public HttpFilter<log>
{
  public:
    log() {}
    virtual void doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) override;
};


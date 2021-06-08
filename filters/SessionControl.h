/**
 *
 *  SessionControl.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;


class SessionControl : public HttpFilter<SessionControl>
{
  public:
    SessionControl() {}
    virtual void doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) override;
};


/**
 *
 *  RemoveOrders.h
 *
 */

#pragma once

#include <drogon/HttpFilter.h>
using namespace drogon;


class RemoveOrders : public HttpFilter<RemoveOrders>
{
  public:
    RemoveOrders() {}
    virtual void doFilter(const HttpRequestPtr &req,
                          FilterCallback &&fcb,
                          FilterChainCallback &&fccb) override;
};


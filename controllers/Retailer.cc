#include "Retailer.h"
#include <fmt/format.h>
//add definition of your processing function here
using namespace std;

void Retailer::newGoods(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    if(req->method() == Get) {
        auto resp = HttpResponse::newHttpViewResponse("new_goods.csp");
        callback(resp);
    } else if(req->method() == Post) {
        auto param = req->getParameters();
        for(const auto &it : param){
            fmt::print("{}:{}\n", it.first, it.second);
        }
        auto resp = HttpResponse::newHttpResponse();
        resp->setStatusCode(k200OK);
        resp->setBody("add new goods success!");
        callback(resp);
    }
}

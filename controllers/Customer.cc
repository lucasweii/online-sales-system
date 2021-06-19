#include "Customer.h"
#include "../MyUtils.h"
#include <fmt/format.h>
using namespace std;
using namespace fmt;
using namespace orm;
//add definition of your processing function here
void Customer::index(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback) {
    auto DbPtr = app().getDbClient();
    Result goods_ret = DbPtr->execSqlSync("select * from `goods`");
    vector<unordered_map<string, string>> goods_data;
    for(auto &goods : goods_ret){
        goods_data.emplace_back();
        goods_data.back()["goods_id"] = to_string(goods["id"].as<int>());
        goods_data.back()["goods_name"] = goods["name"].as<string>();
        double price = goods["price"].as<double>();
        double discount = goods["discount"].as<double>();
        if(discount == 0) {
            discount = 1;
        }
        price *= discount;
        goods_data.back()["goods_price"] = MyUtils::double2str(price, 2);
        goods_data.back()["goods_stock"] = to_string(goods["stock"].as<int>());
        goods_data.back()["goods_pic"] = goods["pic_path"].as<string>();
        goods_data.back()["goods_category"] = goods["category"].as<string>();
        goods_data.back()["goods_sales"] = to_string(goods["sales_num"].as<int>());
    }

    HttpViewData data;
    data.insert("user_pic", req->session()->get<string>("user_pic"));
    data.insert("user_name", req->session()->get<string>("user_name"));
    data.insert("goods_data", goods_data);
    auto resp = HttpResponse::newHttpViewResponse("index_customer.csp", data);
    callback(resp);
}

#pragma once
#include <drogon/HttpController.h>
using namespace drogon;
class Image:public drogon::HttpController<Image>
{
  public:
    METHOD_LIST_BEGIN
    //use METHOD_ADD to add your custom processing function here;
    //METHOD_ADD(Image::get,"/{2}/{1}",Get);//path is /Image/{arg2}/{arg1}
    //METHOD_ADD(Image::your_method_name,"/{1}/{2}/list",Get);//path is /Image/{arg1}/{arg2}/list
    //ADD_METHOD_TO(Image::your_method_name,"/absolute/path/{1}/{2}/list",Get);//path is /absolute/path/{arg1}/{arg2}/list
    ADD_METHOD_TO(Image::uploadImage, "/upload_picture?{1:image_type}", Post, "Log");
    METHOD_LIST_END
    // your declaration of processing function maybe like this:
    // void get(const HttpRequestPtr& req,std::function<void (const HttpResponsePtr &)> &&callback,int p1,std::string p2);
    // void your_method_name(const HttpRequestPtr& req,std::function<void (const HttpResponsePtr &)> &&callback,double p1,int p2) const;
    void uploadImage(const HttpRequestPtr& req, std::function<void (const HttpResponsePtr &)> &&callback, const std::string& image_type);
};

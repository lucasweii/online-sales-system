#include "Image.h"
//add definition of your processing function here
void Image::uploadImage(const HttpRequestPtr &req, std::function<void(const HttpResponsePtr &)> &&callback, const std::string& image_type) {
    MultiPartParser fileUpload;
    if (fileUpload.parse(req) != 0 || fileUpload.getFiles().size() != 1)
    {
        auto resp = HttpResponse::newHttpResponse();
        resp->setBody("Must only be 1 file");
        resp->setStatusCode(k403Forbidden);
        callback(resp);
        return;
    }
    auto para = req->getParameters();
    auto &file = fileUpload.getFiles()[0];
    file.save(std::string("./views/resources/") + image_type + "/" + para["name"]);
    auto resp = HttpResponse::newHttpResponse();
    resp->setStatusCode(k200OK);
    callback(resp);
}

#ifndef __ESPICIALWEBDAV_H
#define __ESPICIALWEBDAV_H

#include <ESPWebDAV.h>

class ESPIcialWebDAV: public ESPWebDAV
{
public:

    using ActivityCallback = std::function<void(const char* request)>;
    void setActivityCallback(const ActivityCallback& cb)
    {
        activityFn = cb;
    }

    void handleClient();

protected:

    bool parseRequest();

    ActivityCallback activityFn = nullptr;
};

#endif // __ESPICIALWEBDAV_H

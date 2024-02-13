/*
    ESP8266WebServer.h - Dead simple web-server.
    Supports only one simultaneous client, knows how to handle GET and POST.

    Copyright (c) 2014 Ivan Grokhotkov. All rights reserved.
    Simplified/Adapted for ESPWebDav:
    Copyright (c) 2018 Gurpreet Bal https://github.com/ardyesp/ESPWebDAV
    Copyright (c) 2020 David Gauchard https://github.com/d-a-v/ESPWebDAV

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
    Modified 8 May 2015 by Hristo Gochkov (proper post and file upload handling)
*/

#include "ESPIcialWebDAV.h"

bool ESPIcialWebDAV::parseRequest()
{
    if (activityFn)
      activityFn("ESPIcialWebDAV::parseRequest");

    return ESPWebDAV::parseRequest();
}

void ESPIcialWebDAV::handleClient()
{
    if (!server)
        return;

    if (server->hasClient())
    {
        if (!locClient || !locClient.available())
        {
            // no or sleeping current client
            // take it over
            locClient = server->accept();
            m_persistent_timer_ms = millis();
            DBG_PRINT("NEW CLIENT-------------------------------------------------------");
        }
    }

    if (!locClient || !locClient.available())
        return;

    // extract uri, headers etc
    parseRequest();

    if (!m_persistent)
        // close the connection
        locClient.stop();
}

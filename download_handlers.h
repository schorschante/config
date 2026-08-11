#pragma once
#include "esphome/components/web_server_base/web_server_base.h"
#include "esphome/components/web_server_idf/web_server_idf.h"

using esphome::web_server_idf::AsyncWebHandler;
using esphome::web_server_idf::AsyncWebServerRequest;

// suspend-server.py eingebettet
static const char SUSPEND_SCRIPT[] =
"#!/usr/bin/env python3\n"
"from flask import Flask\n"
"import subprocess\n"
"\n"
"app = Flask(__name__)\n"
"\n"
"@app.route('/suspend', methods=['GET', 'POST'])\n"
"def suspend():\n"
"    print('PC wird suspendiert...')\n"
"    subprocess.run(['systemctl', 'suspend'])\n"
"    return 'Suspending system...', 200\n"
"\n"
"@app.route('/status')\n"
"def status():\n"
"    return 'Server running', 200\n"
"\n"
"if __name__ == '__main__':\n"
"    print('PC Suspend Server laeuft auf Port 5000')\n"
"    app.run(host='0.0.0.0', port=5000)\n";

// Einfache HTML-Seite mit Download-Link und Link zurueck zur ESPHome-UI
static const char DOWNLOADS_HTML[] =
"<!DOCTYPE html><html><head><meta charset=UTF-8>"
"<title>Downloads</title>"
"<style>body{font-family:sans-serif;padding:2em;max-width:600px;margin:0 auto;}"
"a.btn{display:inline-block;padding:10px 16px;background:#1976d2;color:white;"
"border-radius:4px;text-decoration:none;font-size:14px;"
"box-shadow:0 2px 8px rgba(0,0,0,0.3);margin:8px 0;}"
"a.back{display:inline-block;margin-top:1em;color:#1976d2;text-decoration:none;}"
"</style></head>"
"<body>"
"<h2>Downloads</h2>"
"<p><a class='btn' href='/dl' download='suspend-server.py'>"
"&#8595; suspend-server.py herunterladen</a></p>"
"<p>Python Flask-Service der auf dem PC laeuft und Suspend-Befehle vom ESP32 entgegennimmt.</p>"
"<p><a class='back' href='/'>&#8592; Zurueck zur ESPHome-UI</a></p>"
"</body></html>";

// Handler: /dl -> suspend-server.py als Download
class DownloadHandler : public AsyncWebHandler {
 public:
  bool canHandle(AsyncWebServerRequest *request) const override {
    char buf[AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(buf) == "/dl";
  }
  void handleRequest(AsyncWebServerRequest *request) override {
    AsyncWebServerResponse *r = request->beginResponse(
        200, "text/x-python", std::string(SUSPEND_SCRIPT));
    r->addHeader("Content-Disposition",
                 "attachment; filename=\"suspend-server.py\"");
    request->send(r);
  }
};

// Handler: /downloads -> HTML-Seite mit Download-Link
class DownloadsPageHandler : public AsyncWebHandler {
 public:
  bool canHandle(AsyncWebServerRequest *request) const override {
    char buf[AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(buf) == "/downloads";
  }
  void handleRequest(AsyncWebServerRequest *request) override {
    request->send(200, "text/html", DOWNLOADS_HTML);
  }
};

// Handler: /ui.js -> laedt echtes ESPHome-CDN-Script nach und fuegt Download-Button ein
static const char UI_JS[] =
"(function(){"
"  var s=document.createElement('script');"
"  s.src='https://oi.esphome.io/v2/www.js';"
"  document.head.appendChild(s);"
"  customElements.whenDefined('esp-app').then(function(){"
"    var a=document.createElement('a');"
"    a.href='/dl';"
"    a.download='suspend-server.py';"
"    a.textContent='↓ suspend-server.py';"
"    a.style.cssText='position:fixed;bottom:16px;right:16px;z-index:9999;padding:10px 16px;background:#1976d2;color:#fff;border-radius:4px;text-decoration:none;font-family:sans-serif;font-size:14px;box-shadow:0 2px 8px rgba(0,0,0,.4);';"
"    document.body.appendChild(a);"
"  });"
"})();";

class UiJsHandler : public AsyncWebHandler {
 public:
  bool canHandle(AsyncWebServerRequest *request) const override {
    char buf[AsyncWebServerRequest::URL_BUF_SIZE];
    return request->url_to(buf) == "/ui.js";
  }
  void handleRequest(AsyncWebServerRequest *request) override {
    AsyncWebServerResponse *r = request->beginResponse(
        200, "application/javascript", std::string(UI_JS));
    r->addHeader("Cache-Control", "no-cache");
    request->send(r);
  }
};

// Einmal aus on_boot aufrufen -- nutzt globale WebServerBase-Instanz
void register_download_handlers() {
  auto *base = esphome::web_server_base::global_web_server_base;
  base->add_handler(new DownloadHandler());
  base->add_handler(new DownloadsPageHandler());
  base->add_handler(new UiJsHandler());
}

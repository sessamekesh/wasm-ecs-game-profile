#ifndef IGDEMO_PLATFORM_WEB_RAII_FETCH_H
#define IGDEMO_PLATFORM_WEB_RAII_FETCH_H

#include <emscripten/fetch.h>
#include <igasync/promise.h>
#include <igdemo/igdemo-app.h>

namespace igdemo::web {

std::shared_ptr<
    igasync::Promise<std::variant<std::string, igdemo::FileReadError>>>
read_file(std::string path);

}

#endif

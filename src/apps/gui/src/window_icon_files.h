#pragma once

#include "embedded_file.h"

#include <vector>

using WindowIconFiles = std::vector<EmbeddedFile>;
const WindowIconFiles& get_window_icon_files();

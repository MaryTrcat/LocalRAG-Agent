#pragma once

#include "localrag/ui/DesktopTypes.h"
#include <QString>

namespace localrag {

class ConfigStore final {
public:
    static DesktopConfig load(const QString& path, QString* warning = nullptr);
    static bool save(const QString& path, const DesktopConfig& config, QString* error = nullptr);
};

} // namespace localrag

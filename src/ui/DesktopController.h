#pragma once

#include "localrag/ui/DesktopTypes.h"
#include <QString>
#include <memory>

namespace localrag {

class SQLiteStorage;
class FakeEmbeddingClient;


class DesktopController final {
public:
    explicit DesktopController(DesktopConfig config);
    ~DesktopController();

    void initialize();
    DesktopSnapshot snapshot() const;
    int importDocument(const QString& filePath);
    DesktopAnswer ask(const QString& question);
    void resetConversation();
    const DesktopConfig& config() const { return config_; }

private:
    DesktopConfig config_;
    std::unique_ptr<SQLiteStorage> storage_;
    std::unique_ptr<FakeEmbeddingClient> embedding_;
};

} // namespace localrag

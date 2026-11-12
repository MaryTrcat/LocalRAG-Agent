#pragma once

#include "localrag/ui/DesktopTypes.h"
#include <QWidget>
#include <functional>

class QLabel;
class QListWidget;
class QPushButton;

namespace localrag {

class KnowledgeSidebar final : public QWidget {
public:
    explicit KnowledgeSidebar(QWidget* parent = nullptr);
    void setDocuments(const QVector<DesktopDocument>& documents);
    void setStats(const DesktopStats& stats);
    void setBusy(bool busy);
    void showInlineError(const QString& message);
    void setImportCallback(std::function<void(const QString&)> callback);
    void setSettingsCallback(std::function<void()> callback);

private:
    QLabel* statusLabel_ = nullptr;
    QLabel* documentStat_ = nullptr;
    QLabel* chunkStat_ = nullptr;
    QLabel* errorLabel_ = nullptr;
    QListWidget* documentList_ = nullptr;
    QPushButton* importButton_ = nullptr;
    std::function<void(const QString&)> importCallback_;
    std::function<void()> settingsCallback_;
};

} // namespace localrag

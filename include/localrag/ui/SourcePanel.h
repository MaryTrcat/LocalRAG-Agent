#pragma once

#include "localrag/ui/DesktopTypes.h"
#include <QWidget>

class QLabel;
class QListWidget;

namespace localrag {

class SourcePanel final : public QWidget {
public:
    explicit SourcePanel(QWidget* parent = nullptr);
    void setSources(const QVector<DesktopSource>& sources);
    void clearSources();

private:
    QLabel* summaryLabel_ = nullptr;
    QLabel* emptyLabel_ = nullptr;
    QListWidget* sourceList_ = nullptr;
};

} // namespace localrag

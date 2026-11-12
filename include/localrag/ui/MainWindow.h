#pragma once

#include <QMainWindow>
#include <QLabel>
#include <memory>

#include "localrag/ui/DesktopTypes.h"

namespace localrag {

class MainWindow final : public QMainWindow {
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    void showSettings();
    void refreshView();
    void setBusy(bool busy);

    DesktopConfig config_;
    QString configPath_;
    class DesktopController* controller_ = nullptr;
    class KnowledgeSidebar* sidebar_ = nullptr;
    class ChatPanel* chat_ = nullptr;
    class SourcePanel* sources_ = nullptr;
    QLabel* status_ = nullptr;
};

} // namespace localrag

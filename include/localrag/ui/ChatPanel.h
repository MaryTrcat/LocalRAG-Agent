#pragma once

#include "localrag/ui/DesktopTypes.h"
#include <QWidget>
#include <functional>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTextBrowser;

namespace localrag {

class ChatPanel final : public QWidget {
public:
    explicit ChatPanel(QWidget* parent = nullptr);
    void setAskCallback(std::function<void(const QString&)> callback);
    void setResetCallback(std::function<void()> callback);
    void setBusy(bool busy);
    void showAnswer(const DesktopAnswer& answer);
    void showError(const QString& message);
    void resetConversation();

private:
    void appendMessage(const QString& label, const QString& text, const QString& objectName);
    QTextBrowser* transcript_ = nullptr;
    QPlainTextEdit* input_ = nullptr;
    QPushButton* sendButton_ = nullptr;
    QLabel* stateLabel_ = nullptr;
    std::function<void(const QString&)> askCallback_;
    std::function<void()> resetCallback_;
};

} // namespace localrag

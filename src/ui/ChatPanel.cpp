#include "localrag/ui/ChatPanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QTextBrowser>
#include <QVBoxLayout>

namespace localrag {

ChatPanel::ChatPanel(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("chatPanel"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(28, 24, 28, 20);
    root->setSpacing(14);

    auto* header = new QHBoxLayout();
    auto* title = new QLabel(QStringLiteral("知识库问答"), this);
    title->setObjectName(QStringLiteral("panelTitle"));
    header->addWidget(title);
    header->addStretch();
    auto* reset = new QPushButton(style()->standardIcon(QStyle::SP_BrowserReload), QStringLiteral("新会话"), this);
    reset->setObjectName(QStringLiteral("secondaryButton"));
    reset->setToolTip(QStringLiteral("清空当前问答记录"));
    connect(reset, &QPushButton::clicked, this, [this] {
        resetConversation();
        if (resetCallback_) resetCallback_();
    });
    header->addWidget(reset);
    root->addLayout(header);

    stateLabel_ = new QLabel(QStringLiteral("导入资料后开始提问"), this);
    stateLabel_->setObjectName(QStringLiteral("mutedLabel"));
    root->addWidget(stateLabel_);

    transcript_ = new QTextBrowser(this);
    transcript_->setObjectName(QStringLiteral("transcript"));
    transcript_->setOpenExternalLinks(false);
    root->addWidget(transcript_, 1);

    auto* composer = new QHBoxLayout();
    input_ = new QPlainTextEdit(this);
    input_->setObjectName(QStringLiteral("questionInput"));
    input_->setPlaceholderText(QStringLiteral("询问你的本地资料…"));
    input_->setFixedHeight(70);
    input_->setAccessibleName(QStringLiteral("问题输入框"));
    composer->addWidget(input_, 1);
    sendButton_ = new QPushButton(style()->standardIcon(QStyle::SP_ArrowForward), QStringLiteral("发送"), this);
    sendButton_->setObjectName(QStringLiteral("accentButton"));
    sendButton_->setMinimumSize(92, 52);
    connect(sendButton_, &QPushButton::clicked, this, [this] {
        const auto question = input_->toPlainText().trimmed();
        if (question.isEmpty()) {
            stateLabel_->setText(QStringLiteral("请输入问题后再发送"));
            input_->setFocus();
            return;
        }
        appendMessage(QStringLiteral("你"), question, QStringLiteral("userMessage"));
        input_->clear();
        if (askCallback_) askCallback_(question);
    });
    composer->addWidget(sendButton_);
    root->addLayout(composer);
}

void ChatPanel::appendMessage(const QString& label, const QString& text, const QString& objectName) {
    transcript_->append(QStringLiteral("<div class=\"%1\"><b>%2</b><br/>%3</div><br/>")
                            .arg(objectName, label, text.toHtmlEscaped().replace("\n", "<br/>")));
}

void ChatPanel::setAskCallback(std::function<void(const QString&)> callback) { askCallback_ = std::move(callback); }
void ChatPanel::setResetCallback(std::function<void()> callback) { resetCallback_ = std::move(callback); }

void ChatPanel::setBusy(bool busy) {
    input_->setEnabled(!busy);
    sendButton_->setEnabled(!busy);
    stateLabel_->setText(busy ? QStringLiteral("正在检索并生成回答…") : QStringLiteral("就绪"));
}

void ChatPanel::showAnswer(const DesktopAnswer& answer) {
    appendMessage(QStringLiteral("LocalRAG"), answer.answer, QStringLiteral("assistantMessage"));
    stateLabel_->setText(answer.fakeMode ? QStringLiteral("模拟回答模式 · 可在设置中配置 API") : QStringLiteral("API 回答已完成"));
}

void ChatPanel::showError(const QString& message) {
    stateLabel_->setText(message);
    stateLabel_->setObjectName(QStringLiteral("errorLabel"));
    stateLabel_->style()->unpolish(stateLabel_);
    stateLabel_->style()->polish(stateLabel_);
}

void ChatPanel::resetConversation() {
    transcript_->clear();
    stateLabel_->setText(QStringLiteral("导入资料后开始提问"));
    stateLabel_->setObjectName(QStringLiteral("mutedLabel"));
}

} // namespace localrag

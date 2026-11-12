#include "localrag/ui/MainWindow.h"

#include "localrag/ui/ChatPanel.h"
#include "ConfigStore.h"
#include "DesktopController.h"
#include "localrag/ui/KnowledgeSidebar.h"
#include "localrag/ui/SourcePanel.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QDir>
#include <stdexcept>

namespace localrag {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent) {
    setObjectName(QStringLiteral("mainWindow"));
    setWindowTitle(QStringLiteral("LocalRAG Desktop"));
    setMinimumSize(1180, 720);
    configPath_ = QDir::current().filePath(QStringLiteral("config/localrag.json"));
    QString warning;
    config_ = ConfigStore::load(configPath_, &warning);
    controller_ = new DesktopController(config_);

    auto* outer = new QSplitter(Qt::Horizontal, this);
    outer->setObjectName(QStringLiteral("mainSplitter"));
    outer->setOpaqueResize(true);
    sidebar_ = new KnowledgeSidebar(outer);
    auto* right = new QSplitter(Qt::Horizontal, outer);
    right->setObjectName(QStringLiteral("contentSplitter"));
    right->setOpaqueResize(true);
    chat_ = new ChatPanel(right);
    sources_ = new SourcePanel(right);
    outer->addWidget(sidebar_);
    outer->addWidget(right);
    outer->setStretchFactor(0, 0);
    outer->setStretchFactor(1, 1);
    right->setStretchFactor(0, 1);
    right->setStretchFactor(1, 0);
    outer->setSizes({260, 1});
    right->setSizes({700, 320});
    setCentralWidget(outer);

    status_ = new QLabel(QStringLiteral("正在初始化知识库…"), this);
    statusBar()->addWidget(status_, 1);
    statusBar()->setObjectName(QStringLiteral("statusBar"));

    sidebar_->setImportCallback([this](const QString& path) {
        setBusy(true);
        try {
            const auto count = controller_->importDocument(path);
            refreshView();
            status_->setText(QStringLiteral("已导入 %1 个文本片段").arg(count));
        } catch (const std::exception& error) {
            const auto message = QString::fromLocal8Bit(error.what());
            sidebar_->showInlineError(message);
            status_->setText(message);
        }
        setBusy(false);
    });
    sidebar_->setSettingsCallback([this] { showSettings(); });
    chat_->setAskCallback([this](const QString& question) {
        setBusy(true);
        try {
            const auto answer = controller_->ask(question);
            chat_->showAnswer(answer);
            sources_->setSources(answer.sources);
            status_->setText(answer.fakeMode ? QStringLiteral("模拟回答完成 · 配置 API 后可连接真实模型") : QStringLiteral("回答完成"));
        } catch (const std::exception& error) {
            const auto message = QString::fromLocal8Bit(error.what());
            chat_->showError(message);
            status_->setText(message);
        }
        setBusy(false);
    });
    chat_->setResetCallback([this] {
        controller_->resetConversation();
        sources_->clearSources();
        status_->setText(QStringLiteral("已新建会话"));
    });

    if (!warning.isEmpty()) status_->setText(warning);
    try {
        controller_->initialize();
        refreshView();
    } catch (const std::exception& error) {
        status_->setText(QStringLiteral("初始化失败：%1").arg(QString::fromLocal8Bit(error.what())));
    }

    setStyleSheet(QStringLiteral(
        "QMainWindow, QWidget#mainWindow { background:#eef1ef; color:#18302d; }"
        "QSplitter::handle { background:#d8e0dc; width:1px; }"
        "QWidget#knowledgeSidebar, QWidget#sourcePanel { background:#e4eae6; }"
        "QWidget#chatPanel { background:#f5f6f3; }"
        "QLabel#brandLabel { color:#18302d; font-size:22px; font-weight:700; letter-spacing:2px; }"
        "QLabel#panelTitle { color:#18302d; font-size:20px; font-weight:700; }"
        "QLabel#sectionLabel { color:#61716d; font-size:11px; font-weight:700; }"
        "QLabel#mutedLabel { color:#61716d; }"
        "QLabel#statusBadge { color:#28584f; background:#d1e5dc; padding:5px 9px; border-radius:8px; }"
        "QLabel#errorLabel { color:#9a4b3e; background:#f3ddd7; padding:8px; border-radius:8px; }"
        "QLabel#emptyState { color:#71817b; background:#edf1ee; padding:14px; border-radius:8px; }"
        "QListWidget { background:transparent; border:0; outline:0; }"
        "QListWidget::item { padding:10px; border-radius:8px; color:#29413b; }"
        "QListWidget::item:selected { background:#d1e5dc; color:#18302d; }"
        "QWidget#statsBox, QWidget#sourceList { background:#edf1ee; border:1px solid #d4dfd9; border-radius:8px; }"
        "QPushButton { color:#23413b; background:#f7f8f5; border:1px solid #c5d2cb; border-radius:8px; padding:7px 11px; }"
        "QPushButton:hover { background:#e7eeea; } QPushButton:pressed { background:#d7e5dd; }"
        "QPushButton#accentButton { color:#18302d; background:#c08d43; border:0; font-weight:700; }"
        "QPushButton#accentButton:hover { background:#d2a05a; }"
        "QPushButton#linkButton { border:0; background:transparent; color:#9a6a2e; padding:3px; text-align:left; }"
        "QTextBrowser#transcript { background:#ffffff; border:1px solid #d7e0db; border-radius:8px; padding:14px; }"
        "QPlainTextEdit#questionInput { background:#ffffff; border:1px solid #c5d2cb; border-radius:8px; padding:8px; }"
        "QPlainTextEdit#questionInput:focus { border:2px solid #c08d43; }"
        "QLabel#userMessage { color:#537065; background:#e5eee9; padding:10px; border-radius:8px; }"
        "QLabel#assistantMessage { color:#18302d; background:#ffffff; padding:10px; border:1px solid #d7e0db; border-radius:8px; }"
        "QLabel#sourceTitle { color:#18302d; font-weight:700; } QLabel#scoreLabel { color:#a36f2e; font-weight:700; }"
        "QLabel#sourcePreview { color:#38534b; background:#f5f7f4; padding:8px; border-radius:6px; }"
    ));
}

MainWindow::~MainWindow() {
    delete controller_;
}

void MainWindow::refreshView() {
    const auto view = controller_->snapshot();
    sidebar_->setDocuments(view.documents);
    sidebar_->setStats(view.stats);
}

void MainWindow::setBusy(bool busy) {
    sidebar_->setBusy(busy);
    chat_->setBusy(busy);
    if (busy) status_->setText(QStringLiteral("处理中…"));
}

void MainWindow::showSettings() {
    QDialog dialog(this);
    dialog.setWindowTitle(QStringLiteral("LocalRAG 设置"));
    dialog.setMinimumWidth(480);
    auto* layout = new QVBoxLayout(&dialog);
    auto* form = new QFormLayout();
    auto* baseUrl = new QLineEdit(config_.baseUrl, &dialog);
    auto* model = new QLineEdit(config_.chatModel, &dialog);
    auto* envName = new QLineEdit(config_.apiKeyEnv, &dialog);
    auto* topK = new QSpinBox(&dialog); topK->setRange(1, 20); topK->setValue(config_.topK);
    auto* chunkSize = new QSpinBox(&dialog); chunkSize->setRange(50, 10000); chunkSize->setValue(config_.chunkSize);
    auto* overlap = new QSpinBox(&dialog); overlap->setRange(0, 9999); overlap->setValue(config_.chunkOverlap);
    form->addRow(QStringLiteral("API Base URL"), baseUrl);
    form->addRow(QStringLiteral("聊天模型"), model);
    form->addRow(QStringLiteral("API Key 环境变量"), envName);
    form->addRow(QStringLiteral("检索数量 Top-K"), topK);
    form->addRow(QStringLiteral("切片长度"), chunkSize);
    form->addRow(QStringLiteral("切片重叠"), overlap);
    layout->addLayout(form);
    auto* hint = new QLabel(QStringLiteral("API Key 不保存在配置文件中，只从指定环境变量读取。"), &dialog);
    hint->setObjectName(QStringLiteral("mutedLabel"));
    hint->setWordWrap(true);
    layout->addWidget(hint);
    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&] {
        if (overlap->value() >= chunkSize->value()) {
            QMessageBox::warning(&dialog, QStringLiteral("设置无效"), QStringLiteral("切片重叠必须小于切片长度。"));
            return;
        }
        config_.baseUrl = baseUrl->text().trimmed();
        config_.chatModel = model->text().trimmed();
        config_.apiKeyEnv = envName->text().trimmed();
        config_.topK = topK->value();
        config_.chunkSize = chunkSize->value();
        config_.chunkOverlap = overlap->value();
        QString error;
        if (!ConfigStore::save(configPath_, config_, &error)) {
            QMessageBox::critical(&dialog, QStringLiteral("保存失败"), error);
            return;
        }
        delete controller_;
        controller_ = new DesktopController(config_);
        controller_->initialize();
        refreshView();
        status_->setText(QStringLiteral("设置已保存"));
        dialog.accept();
    });
    dialog.exec();
}

} // namespace localrag

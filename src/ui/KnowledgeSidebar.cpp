#include "localrag/ui/KnowledgeSidebar.h"

#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStyle>
#include <QVBoxLayout>

namespace localrag {

KnowledgeSidebar::KnowledgeSidebar(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("knowledgeSidebar"));
    setMinimumWidth(230);
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(22, 22, 18, 18);
    root->setSpacing(14);

    auto* brand = new QLabel(QStringLiteral("LOCALRAG\n研究台"), this);
    brand->setObjectName(QStringLiteral("brandLabel"));
    root->addWidget(brand);

    auto* descriptor = new QLabel(QStringLiteral("本地知识库 · 证据优先"), this);
    descriptor->setObjectName(QStringLiteral("mutedLabel"));
    root->addWidget(descriptor);

    auto* statusRow = new QHBoxLayout();
    statusLabel_ = new QLabel(QStringLiteral("就绪"), this);
    statusLabel_->setObjectName(QStringLiteral("statusBadge"));
    statusRow->addWidget(statusLabel_);
    statusRow->addStretch();
    auto* settings = new QPushButton(style()->standardIcon(QStyle::SP_FileDialogDetailedView), QStringLiteral("设置"), this);
    settings->setObjectName(QStringLiteral("secondaryButton"));
    settings->setToolTip(QStringLiteral("打开模型与检索设置"));
    connect(settings, &QPushButton::clicked, this, [this] {
        if (settingsCallback_) settingsCallback_();
    });
    statusRow->addWidget(settings);
    root->addLayout(statusRow);

    importButton_ = new QPushButton(style()->standardIcon(QStyle::SP_DialogOpenButton), QStringLiteral("导入资料"), this);
    importButton_->setObjectName(QStringLiteral("accentButton"));
    importButton_->setMinimumHeight(38);
    importButton_->setAccessibleName(QStringLiteral("导入 Markdown 或 TXT 文件"));
    connect(importButton_, &QPushButton::clicked, this, [this] {
        const auto file = QFileDialog::getOpenFileName(this, QStringLiteral("导入资料"), QString(), QStringLiteral("Markdown/Text (*.md *.txt)"));
        if (!file.isEmpty() && importCallback_) importCallback_(file);
    });
    root->addWidget(importButton_);

    auto* section = new QLabel(QStringLiteral("知识库文件"), this);
    section->setObjectName(QStringLiteral("sectionLabel"));
    root->addWidget(section);

    documentList_ = new QListWidget(this);
    documentList_->setObjectName(QStringLiteral("documentList"));
    documentList_->setAccessibleName(QStringLiteral("知识库文档列表"));
    root->addWidget(documentList_, 1);

    auto* statsBox = new QWidget(this);
    statsBox->setObjectName(QStringLiteral("statsBox"));
    auto* stats = new QFormLayout(statsBox);
    stats->setContentsMargins(12, 10, 12, 10);
    documentStat_ = new QLabel(QStringLiteral("0"), statsBox);
    chunkStat_ = new QLabel(QStringLiteral("0"), statsBox);
    stats->addRow(QStringLiteral("文档"), documentStat_);
    stats->addRow(QStringLiteral("文本片段"), chunkStat_);
    root->addWidget(statsBox);

    errorLabel_ = new QLabel(this);
    errorLabel_->setObjectName(QStringLiteral("errorLabel"));
    errorLabel_->setWordWrap(true);
    errorLabel_->hide();
    root->addWidget(errorLabel_);
}

void KnowledgeSidebar::setDocuments(const QVector<DesktopDocument>& documents) {
    documentList_->clear();
    for (const auto& document : documents) {
        auto* item = new QListWidgetItem(QStringLiteral("%1\n%2 个片段").arg(document.title).arg(document.chunkCount), documentList_);
        item->setToolTip(document.filePath);
        item->setData(Qt::UserRole, document.id);
    }
    if (documents.isEmpty()) {
        auto* item = new QListWidgetItem(QStringLiteral("还没有导入资料"), documentList_);
        item->setFlags(Qt::NoItemFlags);
    }
}

void KnowledgeSidebar::setStats(const DesktopStats& stats) {
    documentStat_->setText(QString::number(stats.documents));
    chunkStat_->setText(QString::number(stats.chunks));
}

void KnowledgeSidebar::setBusy(bool busy) {
    importButton_->setEnabled(!busy);
    statusLabel_->setText(busy ? QStringLiteral("处理中") : QStringLiteral("就绪"));
}

void KnowledgeSidebar::showInlineError(const QString& message) {
    errorLabel_->setText(message);
    errorLabel_->show();
}

void KnowledgeSidebar::setImportCallback(std::function<void(const QString&)> callback) { importCallback_ = std::move(callback); }
void KnowledgeSidebar::setSettingsCallback(std::function<void()> callback) { settingsCallback_ = std::move(callback); }

} // namespace localrag

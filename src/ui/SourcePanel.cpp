#include "localrag/ui/SourcePanel.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>

namespace localrag {

SourcePanel::SourcePanel(QWidget* parent)
    : QWidget(parent) {
    setObjectName(QStringLiteral("sourcePanel"));
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(18, 24, 18, 20);
    root->setSpacing(12);
    auto* title = new QLabel(QStringLiteral("检索来源"), this);
    title->setObjectName(QStringLiteral("panelTitle"));
    root->addWidget(title);
    summaryLabel_ = new QLabel(QStringLiteral("等待回答"), this);
    summaryLabel_->setObjectName(QStringLiteral("mutedLabel"));
    root->addWidget(summaryLabel_);
    emptyLabel_ = new QLabel(QStringLiteral("本次回答没有可展示的来源"), this);
    emptyLabel_->setObjectName(QStringLiteral("emptyState"));
    emptyLabel_->setWordWrap(true);
    root->addWidget(emptyLabel_);
    sourceList_ = new QListWidget(this);
    sourceList_->setObjectName(QStringLiteral("sourceList"));
    sourceList_->setSpacing(8);
    root->addWidget(sourceList_, 1);
}

void SourcePanel::clearSources() {
    sourceList_->clear();
    summaryLabel_->setText(QStringLiteral("等待回答"));
    emptyLabel_->show();
}

void SourcePanel::setSources(const QVector<DesktopSource>& sources) {
    sourceList_->clear();
    summaryLabel_->setText(QStringLiteral("%1 个来源").arg(sources.size()));
    emptyLabel_->setVisible(sources.isEmpty());
    for (const auto& source : sources) {
        auto* item = new QListWidgetItem(sourceList_);
        auto* row = new QWidget(sourceList_);
        auto* layout = new QVBoxLayout(row);
        layout->setContentsMargins(10, 9, 10, 9);
        auto* header = new QHBoxLayout();
        auto* title = new QLabel(QStringLiteral("[%1] %2").arg(source.rank).arg(source.documentTitle), row);
        title->setObjectName(QStringLiteral("sourceTitle"));
        header->addWidget(title, 1);
        auto* score = new QLabel(QStringLiteral("%.2f").arg(source.score, 0, 'f', 2), row);
        score->setObjectName(QStringLiteral("scoreLabel"));
        header->addWidget(score);
        layout->addLayout(header);
        auto* path = new QLabel(source.filePath, row);
        path->setObjectName(QStringLiteral("mutedLabel"));
        path->setWordWrap(true);
        layout->addWidget(path);
        auto* preview = new QLabel(source.content, row);
        preview->setObjectName(QStringLiteral("sourcePreview"));
        preview->setWordWrap(true);
        preview->setMaximumHeight(54);
        preview->hide();
        layout->addWidget(preview);
        auto* expand = new QPushButton(QStringLiteral("展开片段"), row);
        expand->setObjectName(QStringLiteral("linkButton"));
        connect(expand, &QPushButton::clicked, row, [preview, expand] {
            const bool visible = !preview->isVisible();
            preview->setVisible(visible);
            expand->setText(visible ? QStringLiteral("收起片段") : QStringLiteral("展开片段"));
        });
        layout->addWidget(expand, 0, Qt::AlignLeft);
        item->setSizeHint(row->sizeHint());
        sourceList_->addItem(item);
        sourceList_->setItemWidget(item, row);
    }
}

} // namespace localrag

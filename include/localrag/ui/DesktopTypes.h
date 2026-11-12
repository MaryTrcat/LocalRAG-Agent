#pragma once

#include <QString>
#include <QVector>

namespace localrag {

struct DesktopDocument {
    qint64 id = 0;
    QString title;
    QString filePath;
    QString fileType;
    int chunkCount = 0;
};

struct DesktopSource {
    qint64 chunkId = 0;
    int rank = 0;
    float score = 0.0F;
    QString documentTitle;
    QString filePath;
    QString content;
};

struct DesktopStats {
    int documents = 0;
    int chunks = 0;
    int messages = 0;
    int retrievalLogs = 0;
};

struct DesktopAnswer {
    QString answer;
    QVector<DesktopSource> sources;
    bool fakeMode = false;
};

struct DesktopSnapshot {
    QVector<DesktopDocument> documents;
    DesktopStats stats;
};

struct DesktopConfig {
    QString databasePath = QStringLiteral("data/localrag.db");
    QString baseUrl = QStringLiteral("https://api.deepseek.com/v1");
    QString apiKeyEnv = QStringLiteral("LOCALRAG_API_KEY");
    QString chatModel = QStringLiteral("deepseek-chat");
    int topK = 3;
    int chunkSize = 800;
    int chunkOverlap = 120;
};

} // namespace localrag

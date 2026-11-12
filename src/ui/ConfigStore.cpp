#include "ConfigStore.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QDir>
#include <QProcessEnvironment>

namespace localrag {

namespace {

int environmentInt(const QString& name, int current, QString* warning) {
    const auto value = QProcessEnvironment::systemEnvironment().value(name);
    if (value.isEmpty()) {
        return current;
    }
    bool ok = false;
    const int parsed = value.toInt(&ok);
    if (!ok || parsed <= 0) {
        if (warning != nullptr) {
            *warning += QStringLiteral("忽略无效的 %1；").arg(name);
        }
        return current;
    }
    return parsed;
}

} // namespace

DesktopConfig ConfigStore::load(const QString& path, QString* warning) {
    DesktopConfig config;
    QFile file(path);
    if (file.exists() && file.open(QIODevice::ReadOnly)) {
        QJsonParseError parseError{};
        const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error == QJsonParseError::NoError && document.isObject()) {
            const auto root = document.object();
            const auto database = root.value(QStringLiteral("database")).toObject();
            const auto llm = root.value(QStringLiteral("llm")).toObject();
            const auto retrieval = root.value(QStringLiteral("retrieval")).toObject();
            config.databasePath = database.value(QStringLiteral("sqlite_path")).toString(config.databasePath);
            config.baseUrl = llm.value(QStringLiteral("base_url")).toString(config.baseUrl);
            config.apiKeyEnv = llm.value(QStringLiteral("api_key_env")).toString(config.apiKeyEnv);
            config.chatModel = llm.value(QStringLiteral("chat_model")).toString(config.chatModel);
            config.topK = retrieval.value(QStringLiteral("top_k")).toInt(config.topK);
            config.chunkSize = retrieval.value(QStringLiteral("chunk_size")).toInt(config.chunkSize);
            config.chunkOverlap = retrieval.value(QStringLiteral("chunk_overlap")).toInt(config.chunkOverlap);
        } else if (warning != nullptr) {
            *warning = QStringLiteral("配置文件格式无效，已使用默认设置；");
        }
    }

    const auto env = QProcessEnvironment::systemEnvironment();
    config.baseUrl = env.value(QStringLiteral("LOCALRAG_BASE_URL"), config.baseUrl);
    config.chatModel = env.value(QStringLiteral("LOCALRAG_CHAT_MODEL"), config.chatModel);
    config.topK = environmentInt(QStringLiteral("LOCALRAG_TOP_K"), config.topK, warning);
    config.chunkSize = environmentInt(QStringLiteral("LOCALRAG_CHUNK_SIZE"), config.chunkSize, warning);
    config.chunkOverlap = environmentInt(QStringLiteral("LOCALRAG_CHUNK_OVERLAP"), config.chunkOverlap, warning);
    return config;
}

bool ConfigStore::save(const QString& path, const DesktopConfig& config, QString* error) {
    const QFileInfo info(path);
    if (!QDir().mkpath(info.absolutePath())) {
        if (error != nullptr) {
            *error = QStringLiteral("无法创建配置目录：%1").arg(info.absolutePath());
        }
        return false;
    }

    QJsonObject database{{QStringLiteral("sqlite_path"), config.databasePath}};
    QJsonObject llm{{QStringLiteral("base_url"), config.baseUrl},
                    {QStringLiteral("api_key_env"), config.apiKeyEnv},
                    {QStringLiteral("chat_model"), config.chatModel}};
    QJsonObject retrieval{{QStringLiteral("top_k"), config.topK},
                          {QStringLiteral("chunk_size"), config.chunkSize},
                          {QStringLiteral("chunk_overlap"), config.chunkOverlap}};
    QJsonObject root{{QStringLiteral("database"), database},
                     {QStringLiteral("llm"), llm},
                     {QStringLiteral("retrieval"), retrieval}};

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(root).toJson(QJsonDocument::Indented)) < 0 || !file.commit()) {
        if (error != nullptr) {
            *error = QStringLiteral("无法写入配置文件：%1").arg(path);
        }
        return false;
    }
    return true;
}

} // namespace localrag

#pragma once

#include "RNetMsgBroker_global.h"
#include "canframe.h"
#include "rnetmsgbroker.h"

#include <QJsonObject>
#include <QString>

#include <memory>

class RNETMSGBROKER_EXPORT RNetFrame : public CanFrame
{
public:
    enum class Detail {
        Short,
        Medium,
        Long,
        Full
    };

    using JsonCatalogRef = std::shared_ptr<const RNetMsgBroker>;

    explicit RNetFrame(const CanFrame &canFrame,
                       JsonCatalogRef json = defaultJsonCatalog());

    RNetFrame(const CanFrame &canFrame,
              JsonCatalogRef json,
              const QJsonObject &eigenschaften);

    ~RNetFrame() = default;

    QString toString(Detail detail) const;
    QString toString(const QString &detail) const;

    // Compatibility for existing QtRNetAnalyzer model code.
    const QString &toString() const;

    QString name() const;
    quint32 getKey() const;

    const QJsonObject &eigenschaften() const;
    const QJsonObject &properties() const;
    JsonCatalogRef json() const;

    static std::unique_ptr<RNetFrame> decodeRNetMessage(const CanFrame &frame);

    static void setDefaultJsonCatalog(JsonCatalogRef json);
    static JsonCatalogRef defaultJsonCatalog();
    static void clearDefaultJsonCatalog();

    static Detail detailFromString(QString detail);

private:
    RNetMsgBroker::CanMsg toBrokerMsg() const;
    QString brokerText(bool full) const;
    QString rawText() const;
    static QString fallbackText(const CanFrame &frame);
    static QString hexBytes(const QByteArray &data);
    static quint32 keyMaskFromJson(const QJsonObject &object, quint32 fallback);
    static bool jsonToUInt(const QJsonValue &value, quint32 *out);
    static QString nameFromJson(const QJsonObject &object);
    static QString nameFromText(QString text);

private:
    JsonCatalogRef m_json;
    QJsonObject m_eigenschaften;
    mutable QString m_defaultTextCache;
};

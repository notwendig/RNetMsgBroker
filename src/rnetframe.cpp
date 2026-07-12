#include "rnetframe.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonValue>
#include <QMutex>
#include <QMutexLocker>
#include <QRegularExpression>
#include <QStringList>

#include <cmath>
#include <limits>

namespace {
QMutex &defaultCatalogMutex()
{
    static QMutex mutex;
    return mutex;
}

std::shared_ptr<const RNetMsgBroker> &defaultCatalogStorage()
{
    static std::shared_ptr<const RNetMsgBroker> catalog;
    return catalog;
}

QStringList defaultJsonCandidates()
{
    QStringList paths;

    const QString envPath = qEnvironmentVariable("RNETMSGBROKER_JSON").trimmed();
    if (!envPath.isEmpty())
        paths << envPath;

    const QString appDir = QCoreApplication::applicationDirPath();
    if (!appDir.isEmpty()) {
        paths << appDir + QStringLiteral("/R-Net.json");
        paths << appDir + QStringLiteral("/../R-Net.json");
        paths << appDir + QStringLiteral("/../../external/RNetMsgBroker/R-Net.json");
        paths << appDir + QStringLiteral("/../../../RNetMsgBroker/R-Net.json");
    }

    const QString cwd = QDir::currentPath();
    if (!cwd.isEmpty()) {
        paths << cwd + QStringLiteral("/R-Net.json");
        paths << cwd + QStringLiteral("/external/RNetMsgBroker/R-Net.json");
        paths << cwd + QStringLiteral("/../RNetMsgBroker/R-Net.json");
    }

    paths.removeDuplicates();
    return paths;
}

QString valueText(const QJsonObject &object, const QString &key)
{
    const QJsonValue value = object.value(key);
    if (value.isString())
        return value.toString();
    if (value.isDouble())
        return QString::number(static_cast<quint64>(value.toDouble()));
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    return QString();
}
}

RNetFrame::RNetFrame(const CanFrame &canFrame, JsonCatalogRef json)
    : CanFrame(canFrame),
      m_json(std::move(json))
{
    if (m_json)
        m_eigenschaften = m_json->properties(toBrokerMsg());
}

RNetFrame::RNetFrame(const CanFrame &canFrame,
                     JsonCatalogRef json,
                     const QJsonObject &eigenschaften)
    : CanFrame(canFrame),
      m_json(std::move(json)),
      m_eigenschaften(eigenschaften)
{
}

QString RNetFrame::toString(Detail detail) const
{
    switch (detail) {
    case Detail::Short:
        return name();

    case Detail::Medium:
        return brokerText(false);

    case Detail::Long: {
        QString text = brokerText(false);
        const QString raw = rawText();
        if (!raw.isEmpty()) {
            if (!text.isEmpty())
                text += QStringLiteral("; ");
            text += raw;
        }
        return text;
    }

    case Detail::Full:
        return brokerText(true);
    }

    return brokerText(false);
}

QString RNetFrame::toString(const QString &detail) const
{
    return toString(detailFromString(detail));
}

const QString &RNetFrame::toString() const
{
    m_defaultTextCache = toString(Detail::Medium);
    return m_defaultTextCache;
}

QString RNetFrame::name() const
{
    const QString jsonName = nameFromJson(m_eigenschaften);
    if (!jsonName.isEmpty())
        return jsonName;

    return nameFromText(brokerText(false));
}

quint32 RNetFrame::getKey() const
{
    enum : quint32 {
        msk_ext = 0x40000000u,
        msk_rtr = 0x20000000u,
        msk_id  = 0x1FFFFFFFu
    };

    const quint32 mask = keyMaskFromJson(m_eigenschaften, msk_id);
    return (id & mask) | (extended ? msk_ext : 0u) | (remote ? msk_rtr : 0u);
}

const QJsonObject &RNetFrame::eigenschaften() const
{
    return m_eigenschaften;
}

const QJsonObject &RNetFrame::properties() const
{
    return m_eigenschaften;
}

RNetFrame::JsonCatalogRef RNetFrame::json() const
{
    return m_json;
}

std::unique_ptr<RNetFrame> RNetFrame::decodeRNetMessage(const CanFrame &frame)
{
    return std::make_unique<RNetFrame>(frame, defaultJsonCatalog());
}

void RNetFrame::setDefaultJsonCatalog(JsonCatalogRef json)
{
    QMutexLocker locker(&defaultCatalogMutex());
    defaultCatalogStorage() = std::move(json);
}

RNetFrame::JsonCatalogRef RNetFrame::defaultJsonCatalog()
{
    QMutexLocker locker(&defaultCatalogMutex());

    if (defaultCatalogStorage())
        return defaultCatalogStorage();

    auto catalog = std::make_shared<RNetMsgBroker>();

    QString error;
    for (const QString &path : defaultJsonCandidates()) {
        if (!QFileInfo::exists(path))
            continue;

        if (catalog->readJson(path, &error)) {
            defaultCatalogStorage() = catalog;
            return catalog;
        }
    }

    // Return an empty broker. Decoding then yields UNKNOWN/fallback text,
    // but callers still have a valid JSON-catalog reference member.
    defaultCatalogStorage() = catalog;
    return catalog;
}

void RNetFrame::clearDefaultJsonCatalog()
{
    QMutexLocker locker(&defaultCatalogMutex());
    defaultCatalogStorage().reset();
}

RNetFrame::Detail RNetFrame::detailFromString(QString detail)
{
    detail = detail.trimmed().toLower();

    if (detail == QStringLiteral("shot") || detail == QStringLiteral("short") || detail == QStringLiteral("kurz"))
        return Detail::Short;
    if (detail == QStringLiteral("long") || detail == QStringLiteral("lang"))
        return Detail::Long;
    if (detail == QStringLiteral("full") || detail == QStringLiteral("voll"))
        return Detail::Full;

    return Detail::Medium;
}

RNetMsgBroker::CanMsg RNetFrame::toBrokerMsg() const
{
    RNetMsgBroker::CanMsg msg;
    msg.canId = id;
    msg.data = data;
    msg.extended = extended;
    msg.remote = remote;
    return msg;
}

QString RNetFrame::brokerText(bool full) const
{
    if (!m_json)
        return fallbackText(*this);

    QString text = m_json->toString(toBrokerMsg(), full).trimmed();
    if (text.isEmpty())
        text = fallbackText(*this);
    return text;
}

QString RNetFrame::rawText() const
{
    QStringList parts;
    parts << QStringLiteral("id=0x%1").arg(id, 8, 16, QLatin1Char('0')).toUpper();
    parts << (extended ? QStringLiteral("EXT") : QStringLiteral("STD"));
    if (remote)
        parts << QStringLiteral("RTR");
    parts << QStringLiteral("dlc=%1").arg(data.size());
    if (!data.isEmpty())
        parts << QStringLiteral("data=%1").arg(hexBytes(data));
    return parts.join(QStringLiteral(", "));
}

QString RNetFrame::fallbackText(const CanFrame &frame)
{
    QStringList parts;
    parts << QStringLiteral("UNKNOWN");
    parts << QStringLiteral("id=0x%1").arg(frame.id, 8, 16, QLatin1Char('0')).toUpper();
    parts << (frame.extended ? QStringLiteral("EXT") : QStringLiteral("STD"));
    if (frame.remote)
        parts << QStringLiteral("RTR");
    if (!frame.data.isEmpty())
        parts << QStringLiteral("data=%1").arg(hexBytes(frame.data));
    return parts.join(QStringLiteral(" "));
}

QString RNetFrame::hexBytes(const QByteArray &data)
{
    QStringList bytes;
    bytes.reserve(data.size());
    for (unsigned char byte : data) {
        bytes << QStringLiteral("%1")
                     .arg(static_cast<quint8>(byte), 2, 16, QLatin1Char('0'))
                     .toUpper();
    }
    return bytes.join(QLatin1Char(' '));
}

quint32 RNetFrame::keyMaskFromJson(const QJsonObject &object, quint32 fallback)
{
    static const QStringList keys = {
        QStringLiteral("key-mask"),
        QStringLiteral("key_mask"),
        QStringLiteral("keyMask"),
        QStringLiteral("Id-and-mask"),
        QStringLiteral("id-and-mask"),
        QStringLiteral("id_and_mask"),
        QStringLiteral("mask")
    };

    for (const QString &key : keys) {
        quint32 value = 0;
        if (jsonToUInt(object.value(key), &value))
            return value;
    }

    return fallback;
}

bool RNetFrame::jsonToUInt(const QJsonValue &value, quint32 *out)
{
    if (!out)
        return false;

    quint64 number = 0;
    if (value.isDouble()) {
        const double d = value.toDouble();
        if (!std::isfinite(d) || d < 0.0)
            return false;

        const double rounded = std::floor(d);
        if (d != rounded || d > static_cast<double>(std::numeric_limits<quint32>::max()))
            return false;

        number = static_cast<quint64>(rounded);
    } else if (value.isString()) {
        QString text = value.toString().trimmed();
        if (text.startsWith(QLatin1Char('-')))
            return false;
        if (text.startsWith(QLatin1Char('+')))
            text.remove(0, 1);

        bool forcedHex = false;
        if (text.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
            text.remove(0, 2);
            forcedHex = true;
        }
        if (text.isEmpty())
            return false;

        bool ok = false;
        const int base = forcedHex || text.contains(QRegularExpression(QStringLiteral("[A-Fa-f]"))) ? 16 : 10;
        number = text.toULongLong(&ok, base);
        if (!ok || number > std::numeric_limits<quint32>::max())
            return false;
    } else {
        return false;
    }

    *out = static_cast<quint32>(number);
    return true;
}

QString RNetFrame::nameFromJson(const QJsonObject &object)
{
    static const QStringList keys = {
        QStringLiteral("rnet_name"),
        QStringLiteral("rnet-name"),
        QStringLiteral("name")
    };

    for (const QString &key : keys) {
        const QString text = valueText(object, key).trimmed();
        if (!text.isEmpty())
            return text;
    }

    return QString();
}

QString RNetFrame::nameFromText(QString text)
{
    text = text.trimmed();
    if (text.isEmpty())
        return QStringLiteral("UNKNOWN");

    qsizetype end = text.size();
    const QList<qsizetype> cuts = {
        text.indexOf(QLatin1Char(' ')),
        text.indexOf(QLatin1Char('(')),
        text.indexOf(QLatin1Char(';')),
        text.indexOf(QLatin1Char('{'))
    };

    for (qsizetype cut : cuts) {
        if (cut >= 0 && cut < end)
            end = cut;
    }

    return text.left(end).trimmed();
}

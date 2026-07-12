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
#include <QJsonArray>
#include <QMap>
#include <algorithm>
#include <QJsonObject>
#include <QByteArray>
#include <QLatin1Char>

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


namespace {

bool rnetFrameValueByNames(const QJsonObject &object, std::initializer_list<const char *> names, QJsonValue *out)
{
    for (const char *name : names) {
        const QString key = QString::fromLatin1(name);
        if (object.contains(key)) {
            if (out)
                *out = object.value(key);
            return true;
        }
    }
    return false;
}

bool rnetFrameStringByNames(const QJsonObject &object, std::initializer_list<const char *> names, QString *out)
{
    QJsonValue value;
    if (!rnetFrameValueByNames(object, names, &value))
        return false;

    QString text;
    if (value.isString())
        text = value.toString();
    else if (value.isDouble())
        text = QString::number(value.toDouble(), 'g', 15);
    else if (value.isBool())
        text = value.toBool() ? QStringLiteral("true") : QStringLiteral("false");

    text = text.trimmed();
    if (text.isEmpty())
        return false;
    if (out)
        *out = text;
    return true;
}

bool rnetFrameUInt64FromJson(const QJsonValue &value, quint64 *out)
{
    if (value.isDouble()) {
        const double d = value.toDouble();
        if (!std::isfinite(d) || d < 0.0)
            return false;
        const double rounded = std::floor(d);
        if (!qFuzzyCompare(d + 1.0, rounded + 1.0))
            return false;
        constexpr double kMaxExactJsonInteger = 9007199254740992.0; // 2^53
        if (d > kMaxExactJsonInteger)
            return false;
        if (out)
            *out = static_cast<quint64>(rounded);
        return true;
    }

    if (!value.isString())
        return false;

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
    const quint64 parsed = text.toULongLong(&ok, base);
    if (!ok)
        return false;
    if (out)
        *out = parsed;
    return true;
}

bool rnetFrameUInt64ByNames(const QJsonObject &object, std::initializer_list<const char *> names, quint64 *out)
{
    QJsonValue value;
    if (!rnetFrameValueByNames(object, names, &value))
        return false;
    return rnetFrameUInt64FromJson(value, out);
}

bool rnetFrameIntByNames(const QJsonObject &object, std::initializer_list<const char *> names, qint64 *out)
{
    QJsonValue value;
    if (!rnetFrameValueByNames(object, names, &value))
        return false;

    if (value.isDouble()) {
        const double d = value.toDouble();
        if (!std::isfinite(d))
            return false;
        const double rounded = std::floor(d);
        if (!qFuzzyCompare(d + 1.0, rounded + 1.0))
            return false;
        if (out)
            *out = static_cast<qint64>(rounded);
        return true;
    }

    if (!value.isString())
        return false;

    QString text = value.toString().trimmed();
    if (text.isEmpty())
        return false;

    bool ok = false;
    const qint64 parsed = text.toLongLong(&ok, 0);
    if (!ok)
        return false;
    if (out)
        *out = parsed;
    return true;
}

bool rnetFrameBoolByNames(const QJsonObject &object, std::initializer_list<const char *> names, bool *out)
{
    QJsonValue value;
    if (!rnetFrameValueByNames(object, names, &value))
        return false;

    if (value.isBool()) {
        if (out)
            *out = value.toBool();
        return true;
    }
    if (value.isDouble()) {
        if (out)
            *out = !qFuzzyIsNull(value.toDouble());
        return true;
    }
    if (value.isString()) {
        const QString text = value.toString().trimmed().toLower();
        if (text == QStringLiteral("true") || text == QStringLiteral("yes") || text == QStringLiteral("1") || text == QStringLiteral("on")) {
            if (out)
                *out = true;
            return true;
        }
        if (text == QStringLiteral("false") || text == QStringLiteral("no") || text == QStringLiteral("0") || text == QStringLiteral("off")) {
            if (out)
                *out = false;
            return true;
        }
    }
    return false;
}

int rnetFrameTrailingShift(quint64 mask)
{
    if (mask == 0)
        return 0;
    int shift = 0;
    while (shift < 63 && ((mask >> shift) & 1ull) == 0ull)
        ++shift;
    return shift;
}

quint64 rnetFrameBytesToInteger(const QByteArray &data, int offset, int width, bool bigEndian)
{
    if (offset < 0)
        offset = 0;
    if (offset >= data.size())
        return 0;

    int available = data.size() - offset;
    if (width <= 0 || width > available)
        width = available;
    width = std::min(width, 8);

    quint64 value = 0;
    if (bigEndian) {
        for (int i = 0; i < width; ++i)
            value = (value << 8) | static_cast<quint8>(data.at(offset + i));
    } else {
        for (int i = 0; i < width; ++i)
            value |= static_cast<quint64>(static_cast<quint8>(data.at(offset + i))) << (8 * i);
    }
    return value;
}

QString rnetFrameHexValue(quint64 value)
{
    return QStringLiteral("0x%1").arg(value, 0, 16);
}

qint64 rnetFrameSignExtend(quint64 value, int bitCount)
{
    if (bitCount <= 0 || bitCount >= 64)
        return static_cast<qint64>(value);
    const quint64 signBit = 1ull << (bitCount - 1);
    const quint64 mask = (1ull << bitCount) - 1ull;
    value &= mask;
    if (value & signBit)
        value |= ~mask;
    return static_cast<qint64>(value);
}

bool rnetFrameBigEndianFromJson(const QJsonObject &object, bool fallback)
{
    bool direct = fallback;
    if (rnetFrameBoolByNames(object, {"big_endian", "big-endian"}, &direct))
        return direct;

    QString endian;
    if (!rnetFrameStringByNames(object, {"endian", "byte_order", "byte-order"}, &endian))
        return fallback;
    endian = endian.trimmed().toLower();
    if (endian == QStringLiteral("b") || endian == QStringLiteral("big") || endian == QStringLiteral("be") || endian == QStringLiteral("msb"))
        return true;
    if (endian == QStringLiteral("l") || endian == QStringLiteral("little") || endian == QStringLiteral("le") || endian == QStringLiteral("lsb"))
        return false;
    return fallback;
}

int rnetFrameBitCount(quint64 mask)
{
    mask >>= rnetFrameTrailingShift(mask);
    int bits = 0;
    while (mask != 0) {
        ++bits;
        mask >>= 1;
    }
    return std::max(1, bits);
}

} // namespace

const QMap<QString, QString> &RNetFrame::idParts() const
{
    return i_parts;
}

const QMap<QString, QString> &RNetFrame::decodedFields() const
{
    return fields;
}

void RNetFrame::interpretJsonPartsAndFields()
{
    i_parts.clear();
    fields.clear();

    QJsonValue idPartsValue;
    if (rnetFrameValueByNames(m_eigenschaften, {"id_parts", "id-parts", "idParts"}, &idPartsValue) && idPartsValue.isArray()) {
        const QJsonArray idPartsArray = idPartsValue.toArray();
        int index = 0;
        for (const QJsonValue &value : idPartsArray) {
            if (!value.isObject()) {
                ++index;
                continue;
            }
            const QJsonObject item = value.toObject();

            quint64 mask = 0;
            if (!rnetFrameUInt64ByNames(item, {"mask", "bit_mask", "bit-mask"}, &mask)) {
                ++index;
                continue;
            }

            qint64 shift64 = rnetFrameTrailingShift(mask);
            rnetFrameIntByNames(item, {"shift"}, &shift64);
            int shift = static_cast<int>(std::clamp<qint64>(shift64, -63, 63));

            QString name;
            rnetFrameStringByNames(item, {"name", "sig_name", "sig-name"}, &name);
            if (name.isEmpty())
                name = QStringLiteral("id_%1").arg(index);

            quint64 decoded = static_cast<quint64>(id) & mask;
            if (shift > 0)
                decoded >>= shift;
            else if (shift < 0)
                decoded <<= -shift;

            i_parts.insert(name, QString::number(decoded));
            ++index;
        }
    }

    QJsonValue fieldsValue;
    if (rnetFrameValueByNames(m_eigenschaften, {"fields"}, &fieldsValue) && fieldsValue.isArray()) {
        const QJsonArray fieldsArray = fieldsValue.toArray();
        int index = 0;
        for (const QJsonValue &value : fieldsArray) {
            if (!value.isObject()) {
                ++index;
                continue;
            }
            const QJsonObject item = value.toObject();

            QString name;
            rnetFrameStringByNames(item, {"sig_name", "sig-name", "name"}, &name);
            if (name.isEmpty())
                name = QStringLiteral("field%1").arg(index);

            QString source;
            rnetFrameStringByNames(item, {"source"}, &source);
            source = source.trimmed().toLower();
            if (source.isEmpty())
                source = QStringLiteral("data");

            quint64 bitMask = 0xFFFFFFFFFFFFFFFFull;
            int shift = 0;
            if (rnetFrameUInt64ByNames(item, {"bit_mask", "bit-mask", "mask"}, &bitMask))
                shift = rnetFrameTrailingShift(bitMask);

            qint64 shift64 = shift;
            if (rnetFrameIntByNames(item, {"shift"}, &shift64))
                shift = static_cast<int>(std::clamp<qint64>(shift64, -63, 63));

            quint64 offsetValue = 0;
            quint64 widthValue = 0;
            const bool hasOffset = rnetFrameUInt64ByNames(item, {"offset", "byte", "byte_offset"}, &offsetValue);
            const bool hasWidth = rnetFrameUInt64ByNames(item, {"width", "size", "len", "length"}, &widthValue);
            const bool hasByteWindow = hasOffset || hasWidth;
            const int offset = static_cast<int>(std::min<quint64>(offsetValue, 63));
            const int width = static_cast<int>(std::min<quint64>(widthValue, 8));
            const bool bigEndian = rnetFrameBigEndianFromJson(item, false);

            quint64 raw = 0;
            if (source == QStringLiteral("id") || source == QStringLiteral("can_id") || source == QStringLiteral("canid")) {
                raw = id;
            } else if (hasByteWindow) {
                raw = rnetFrameBytesToInteger(data, offset, width, bigEndian);
            } else {
                raw = rnetFrameBytesToInteger(data, 0, data.size(), true);
            }

            quint64 decoded = raw & bitMask;
            if (shift > 0)
                decoded >>= shift;
            else if (shift < 0)
                decoded <<= -shift;

            bool isSigned = false;
            rnetFrameBoolByNames(item, {"signed", "is_signed"}, &isSigned);

            QString rendered;
            if (isSigned)
                rendered = QString::number(rnetFrameSignExtend(decoded, rnetFrameBitCount(bitMask)));
            else
                rendered = rnetFrameHexValue(decoded);

            QString unit;
            if (rnetFrameStringByNames(item, {"einheit", "unit", "units"}, &unit))
                rendered += unit;

            fields.insert(name, rendered);
            ++index;
        }
    }
}

RNetFrame::RNetFrame(const CanFrame &canFrame, JsonCatalogRef json)
    : CanFrame(canFrame),
      m_json(std::move(json))
{
    if (m_json)
        m_eigenschaften = m_json->properties(toBrokerMsg());

    interpretJsonPartsAndFields();}

RNetFrame::RNetFrame(const CanFrame &canFrame,
                     JsonCatalogRef json,
                     const QJsonObject &eigenschaften)
    : CanFrame(canFrame),
      m_json(std::move(json)),
      m_eigenschaften(eigenschaften)
{
}


namespace {

QString rnetFrameDisplayValue(const QJsonValue &value)
{
    if (value.isString())
        return value.toString().trimmed();
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', 15);
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    return QString();
}

QString rnetFrameDisplayProperty(const QJsonObject &object, std::initializer_list<const char *> names)
{
    for (const char *name : names) {
        const QString key = QString::fromLatin1(name);
        if (!object.contains(key))
            continue;
        const QString text = rnetFrameDisplayValue(object.value(key));
        if (!text.isEmpty())
            return text;
    }
    return QString();
}

void rnetFrameAppendMapValues(QStringList &parts, const QMap<QString, QString> &values)
{
    for (auto it = values.cbegin(); it != values.cend(); ++it) {
        const QString key = it.key().trimmed();
        const QString value = it.value().trimmed();
        if (key.isEmpty())
            continue;
        if (value.isEmpty())
            parts << key;
        else
            parts << QStringLiteral("%1=%2").arg(key, value);
    }
}


QString rnetFrameHexBytes(const QByteArray &data)
{
    QStringList parts;
    parts.reserve(data.size());
    for (int i = 0; i < data.size(); ++i) {
        const uint value = static_cast<unsigned char>(data.at(i));
        parts << QStringLiteral("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
    }
    return parts.join(QStringLiteral(" "));
}

QString rnetFrameFullJsonSummary(const QJsonObject &object, const CanFrame &frame)
{
    QStringList parts;

    auto add = [&](const char *label, std::initializer_list<const char *> keys) {
        const QString value = rnetFrameDisplayProperty(object, keys);
        if (!value.isEmpty())
            parts << QStringLiteral("%1=%2").arg(QString::fromLatin1(label), value);
    };

    add("typ", {"typ", "type", "frametype", "frame_type"});
    add("phase", {"phase", "pfase"});
    add("quelle", {"quelle", "source", "src"});
    add("senke", {"senke", "target", "dst", "destination"});
    add("zyklus", {"zyklus", "cycle"});
    add("einheit", {"einheit", "unit", "units"});
    add("desc", {"desc", "descript", "description"});
    add("id", {"id"});
    add("mask", {"Id-and-mask", "id-and-mask", "id_and_mask", "key-mask", "key_mask", "mask"});
    add("result", {"id-result", "id_result", "result"});

    parts << (frame.extended ? QStringLiteral("EXT") : QStringLiteral("STD"));
    if (frame.remote)
        parts << QStringLiteral("RTR");
    parts << QStringLiteral("dlc=%1").arg(frame.data.size());
    if (!frame.data.isEmpty())
        parts << QStringLiteral("data=%1").arg(rnetFrameHexBytes(frame.data));

    return parts.isEmpty() ? QString() : QStringLiteral("{%1}").arg(parts.join(QStringLiteral(", ")));
}

QString rnetFrameMapsText(const QString &frameName,
                          const QMap<QString, QString> &idParts,
                          const QMap<QString, QString> &fields,
                          const QJsonObject &properties,
                          const CanFrame &frame,
                          bool full)
{
    QStringList parts;
    const QString name = frameName.trimmed();
    parts << (name.isEmpty() ? QStringLiteral("UNKNOWN") : name);

    rnetFrameAppendMapValues(parts, idParts);
    rnetFrameAppendMapValues(parts, fields);

    QString text = parts.join(QStringLiteral("; "));
    if (full) {
        const QString summary = rnetFrameFullJsonSummary(properties, frame);
        if (!summary.isEmpty()) {
            if (!text.isEmpty())
                text += QStringLiteral("; ");
            text += summary;
        }
    }
    return text.trimmed();
}

} // namespace

QString RNetFrame::toString(Detail detail) const
{
    const bool hasDecodedMaps = !i_parts.isEmpty() || !fields.isEmpty();

    auto decodedMapText = [this](bool full) -> QString {
        return rnetFrameMapsText(name(), i_parts, fields, m_eigenschaften, *this, full);
    };

    switch (detail) {
    case Detail::Short:
        return name();

    case Detail::Medium:
        return hasDecodedMaps ? decodedMapText(false) : brokerText(false);

    case Detail::Long: {
        QString text = hasDecodedMaps ? decodedMapText(false) : brokerText(false);
        const QString raw = rawText();
        if (!raw.isEmpty()) {
            if (!text.isEmpty())
                text += QStringLiteral("; ");
            text += raw;
        }
        return text;
    }

    case Detail::Full:
        return hasDecodedMaps ? decodedMapText(true) : brokerText(true);
    }

    return hasDecodedMaps ? decodedMapText(false) : brokerText(false);
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

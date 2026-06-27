#ifndef RNETMSGBROKER_H
#define RNETMSGBROKER_H

#include "RNetMsgBroker_global.h"

#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMultiMap>
#include <QObject>
#include <QString>
#include <QStringList>

class RNETMSGBROKER_EXPORT RNetMsgBroker : public QObject
{
    Q_OBJECT

public:
    struct CanMsg
    {
        quint32 canId = 0;
        QByteArray data;
        bool extended = false;
        bool remote = false;
    };

    explicit RNetMsgBroker(QObject *parent = nullptr);

    bool readJson(const QString &jsonFilename, QString *errorString = nullptr);

    bool add(const QJsonObject &json, QString *errorString = nullptr);
    bool add(const QJsonDocument &json, QString *errorString = nullptr);
    bool del(const QJsonObject &json, QString *errorString = nullptr);

    QString toString(const CanMsg &msg, bool full = false) const;
    QString toString(quint32 canId, const QByteArray &data, bool extended = false, bool remote = false, bool full = false) const;

    static QString versionString();

    int count() const;
    void clear();
    QStringList names() const;

private:
    enum class Endian
    {
        Little,
        Big
    };

    struct IdPart
    {
        quint32 mask = 0;
        QString name;
        Endian endian = Endian::Little;
        int shift = 0;
    };

    struct Field
    {
        QString name;
        QString source = QStringLiteral("data");
        quint64 bitMask = 0;
        int shift = 0;
        int offset = 0;
        int width = 0;
        bool bigEndian = false;
        bool isSigned = false;
        bool hasByteWindow = false;
        QString cycle;       // Alt-JSON: fields[].zyklu/zyklus; neu bevorzugt: Definition::cycle
        QString unit;        // JSON: unit/einheit
        QString description; // JSON: descipt/descript/description/desc/beschreibung
    };

    struct DataMatcher
    {
        bool enabled = false;
        quint64 value = 0;
        quint64 mask = 0xFFFFFFFFFFFFFFFFull;
        int offset = 0;
        int width = 0;
        bool bigEndian = false;
    };

    struct Definition
    {
        quint32 idAndMask = 0;
        quint32 idResult = 0;
        quint32 keyMask = 0xFFFFFFFFu;
        QString rnetName;
        QString frameType;
        QString summary;
        QString cycle; // JSON: frames[].zyklus, z.B. "20ms" oder "bei Änderung"
        int extended = -1; // -1 = any, 0 = standard, 1 = extended
        int remote = -1;   // -1 = any, 0 = data, 1 = RTR
        QList<IdPart> idParts;
        QList<Field> fields;
        DataMatcher dataMatcher;
        QJsonObject originalJson;
    };

    using DefinitionMap = QMultiMap<quint32, Definition>;

    bool addObjectToMap(const QJsonObject &json, DefinitionMap *target, QString *errorString) const;
    bool parseDefinition(const QJsonObject &json, Definition *definition, QString *errorString) const;
    bool matches(const Definition &definition, const CanMsg &msg) const;
    QString render(const Definition &definition, const CanMsg &msg, bool full) const;

    static QString hex32(quint32 value);
    static QString hex64(quint64 value);
    static QString hexValue(quint64 value);
    static QString hexBytes(const QByteArray &data);
    static bool valueByNames(const QJsonObject &object, const QStringList &names, QJsonValue *value);
    static bool stringByNames(const QJsonObject &object, const QStringList &names, QString *text);
    static bool textByNames(const QJsonObject &object, const QStringList &names, QString *text);
    static QString jsonValueToText(const QJsonValue &value);
    static bool boolByNames(const QJsonObject &object, const QStringList &names, bool *value);
    static bool numberByNames(const QJsonObject &object, const QStringList &names, quint64 *value);
    static bool jsonValueToUInt64(const QJsonValue &value, quint64 *number);
    static int trailingShift(quint64 mask);
    static int popCount32(quint32 value);
    static Endian endianFromJson(const QJsonObject &object, Endian fallback = Endian::Little);
    static quint64 bytesToInteger(const QByteArray &data, int offset, int width, bool bigEndian);
    static qint64 signExtend(quint64 value, int bitCount);
    static QByteArray relaxedJsonToStrict(const QByteArray &data);
    static QString stripLineComments(const QString &text);
    static QList<QJsonObject> objectsFromDocument(const QJsonDocument &doc, QString *errorString);

    DefinitionMap m_definitionsByIdResult;
};

#endif // RNETMSGBROKER_H

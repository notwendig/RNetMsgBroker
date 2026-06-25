#include "rnetmsgbroker.h"

#include <QCoreApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringList>

static bool parseUInt(const QString &text, quint32 *value)
{
    QString s = text.trimmed();
    bool ok = false;
    quint64 parsed = 0;
    if (s.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
        parsed = s.mid(2).toULongLong(&ok, 16);
    else if (s.contains(QRegularExpression(QStringLiteral("[A-Fa-f]"))))
        parsed = s.toULongLong(&ok, 16);
    else
        parsed = s.toULongLong(&ok, 10);
    if (!ok || parsed > 0x1FFFFFFFu)
        return false;
    if (value)
        *value = static_cast<quint32>(parsed);
    return true;
}

static bool parsePayload(const QString &text, QByteArray *payload)
{
    QByteArray out;
    QString s = text.trimmed();
    s.replace(QLatin1Char(':'), QLatin1Char(' '));
    s.replace(QLatin1Char(','), QLatin1Char(' '));
    s.replace(QLatin1Char(';'), QLatin1Char(' '));

    QStringList tokens = s.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (tokens.size() == 1) {
        QString compact = tokens.first();
        if (compact.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
            compact = compact.mid(2);
        if (compact.size() > 2 && compact.size() % 2 == 0) {
            tokens.clear();
            for (int i = 0; i < compact.size(); i += 2)
                tokens << compact.mid(i, 2);
        }
    }

    for (QString token : tokens) {
        if (token.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
            token = token.mid(2);
        bool ok = false;
        const int byteValue = token.toInt(&ok, 16);
        if (!ok || byteValue < 0 || byteValue > 255)
            return false;
        out.append(static_cast<char>(byteValue));
    }

    if (payload)
        *payload = out;
    return true;
}

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("RNetMsgBrokerTest"));
    QCoreApplication::setApplicationVersion(RNetMsgBroker::versionString());

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Kleine Test-App fuer RNetMsgBroker"));
    parser.addHelpOption();

    QCommandLineOption jsonOption({QStringLiteral("j"), QStringLiteral("json")},
                                  QStringLiteral("R-Net JSON-Datei"),
                                  QStringLiteral("file"));
    QCommandLineOption idOption({QStringLiteral("i"), QStringLiteral("id")},
                                QStringLiteral("CAN-ID, z.B. 0x0C000400"),
                                QStringLiteral("canid"));
    QCommandLineOption dataOption({QStringLiteral("d"), QStringLiteral("data")},
                                  QStringLiteral("Payload, z.B. '0B 00 00 00' oder 0B000000"),
                                  QStringLiteral("bytes"));
    QCommandLineOption extendedOption({QStringLiteral("e"), QStringLiteral("extended")},
                                      QStringLiteral("CAN-ID als Extended Frame auswerten"));
    QCommandLineOption remoteOption({QStringLiteral("r"), QStringLiteral("rtr")},
                                    QStringLiteral("Remote-Transmission-Request / RTR"));

    parser.addOption(jsonOption);
    parser.addOption(idOption);
    parser.addOption(dataOption);
    parser.addOption(extendedOption);
    parser.addOption(remoteOption);
    parser.process(app);

    QString jsonFile = parser.value(jsonOption);
    if (jsonFile.isEmpty()) {
        const QString local = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("R-Net.json"));
        jsonFile = QFileInfo::exists(local) ? local : QStringLiteral("R-Net.json");
    }

    RNetMsgBroker broker;
    QString error;
    if (!broker.readJson(jsonFile, &error)) {
        qCritical().noquote() << QStringLiteral("readJson fehlgeschlagen: %1").arg(error);
        return 2;
    }

    qInfo().noquote() << QStringLiteral("geladen: %1 Definitionen aus %2").arg(broker.count()).arg(jsonFile);

    if (parser.isSet(idOption)) {
        quint32 canId = 0;
        if (!parseUInt(parser.value(idOption), &canId)) {
            qCritical().noquote() << QStringLiteral("ungueltige CAN-ID: %1").arg(parser.value(idOption));
            return 3;
        }
        QByteArray data;
        if (parser.isSet(dataOption) && !parsePayload(parser.value(dataOption), &data)) {
            qCritical().noquote() << QStringLiteral("ungueltige Payload-Daten: %1").arg(parser.value(dataOption));
            return 4;
        }
        qInfo().noquote() << broker.toString(canId, data, parser.isSet(extendedOption), parser.isSet(remoteOption));
        return 0;
    }

    const QList<RNetMsgBroker::CanMsg> samples = {
        {0x56710u, QByteArray::fromHex("00aabb"), false, false},
        {0x56760u, QByteArray::fromHex("001234"), false, false},
        {0x004u, QByteArray(), false, true},
        {0x060u, QByteArray::fromHex("01020304"), false, false},
        {0x0C000400u, QByteArray::fromHex("0B00000000000000"), true, false},
        {0x1F003000u, QByteArray::fromHex("1122334455667788"), true, false},
        {0x123u, QByteArray::fromHex("01020304"), false, false},
    };

    for (const RNetMsgBroker::CanMsg &sample : samples)
        qInfo().noquote() << broker.toString(sample);

    return 0;
}

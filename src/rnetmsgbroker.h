#ifndef RNETMSGBROKER_H
#define RNETMSGBROKER_H

/**
 * @file rnetmsgbroker.h
 * @brief JSON-driven decoder for R-Net/CAN frames.
 *
 * @details
 * RNetMsgBroker loads editable frame definitions from @c R-Net.json and renders
 * CAN frames into stable human-readable text. Definitions are matched by CAN-ID
 * mask/result pairs, optional frame-format flags, optional RTR flags and optional
 * payload/data matchers. The class is Qt Core only and can therefore be used by
 * GUI applications, command-line tools and test programs.
 *
 * The public API is intentionally small:
 * - call readJson() or add() to populate the catalogue,
 * - call toString() to decode a frame,
 * - call properties() to retrieve the original JSON object that matched a frame.
 */

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

/**
 * @brief Decode editable R-Net/CAN frame catalogues.
 *
 * RNetMsgBroker is a Qt Core helper around a JSON catalogue. It does not access
 * the CAN bus itself; callers pass already captured CAN frames as CanMsg values.
 * The broker then chooses the best matching JSON definition and renders either a
 * compact text or a full text with metadata.
 *
 * @par Matching order
 * All definitions are tested. If more than one definition matches, the highest
 * specificity score wins. The score is derived from:
 * - number of set bits in @c Id-and-mask,
 * - explicit @c extended and @c remote/RTR restrictions,
 * - presence and bit-count of a payload data matcher,
 * - number of decoded fields.
 *
 * @par Accepted JSON naming variants
 * The parser accepts several historic key spellings, for example
 * @c Id-and-mask, @c id-and-mask, @c id_and_mask and @c mask. Documentation and
 * new catalogue entries should use the canonical keys documented in
 * @c docs/RNET_JSON_SCHEMA.md.
 *
 * @warning This library decodes and formats frames. It does not validate whether
 * sending such frames is safe for a real wheelchair or R-Net installation.
 */
class RNETMSGBROKER_EXPORT RNetMsgBroker : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Minimal CAN message representation used by the decoder.
     *
     * @details
     * @c canId stores the raw 11-bit or 29-bit CAN identifier without SocketCAN
     * flag bits. @c extended selects 29-bit matching, @c remote marks RTR frames.
     * @c data is interpreted only when @c remote is @c false or when a definition
     * explicitly chooses to look at payload bytes.
     */
    struct CanMsg
    {
        /** @brief Raw CAN identifier. Valid R-Net extended IDs are <= 0x1FFFFFFF. */
        quint32 canId = 0;

        /** @brief Payload bytes in bus order. */
        QByteArray data;

        /** @brief @c true for extended/29-bit frames, @c false for standard/11-bit frames. */
        bool extended = false;

        /** @brief @c true for Remote Transmission Request frames. */
        bool remote = false;
    };

    /**
     * @brief Create an empty decoder catalogue.
     * @param parent Optional Qt parent object.
     *
     * The object is usable immediately, but decoding without definitions returns
     * an @c UNKNOWN text. Call readJson() or add() before normal decoding.
     */
    explicit RNetMsgBroker(QObject *parent = nullptr);

    /**
     * @brief Load all frame definitions from a JSON file.
     * @param jsonFilename Path to a UTF-8 JSON file.
     * @param errorString Optional output for a translated, human-readable error.
     * @return @c true when the complete file was parsed and installed.
     *
     * Loading is transactional. The current catalogue is replaced only after the
     * JSON document and every contained definition were parsed successfully. On
     * failure the old catalogue remains unchanged.
     *
     * Accepted top-level forms are an array of frame objects, an object with a
     * @c frames, @c definitions or @c messages array, or a single frame object.
     */
    bool readJson(const QString &jsonFilename, QString *errorString = nullptr);

    /**
     * @brief Add one frame definition at runtime.
     * @param json JSON object with at least @c Id-and-mask or an equivalent key.
     * @param errorString Optional output for parse errors.
     * @return @c true when the definition was inserted.
     *
     * Existing definitions are not removed. If the added definition overlaps an
     * older one, the normal specificity scoring decides which one wins at decode
     * time.
     */
    bool add(const QJsonObject &json, QString *errorString = nullptr);

    /**
     * @brief Add all frame definitions from a JSON document.
     * @param json JSON document in the same accepted forms as readJson().
     * @param errorString Optional output for parse errors.
     * @return @c true when all definitions were inserted.
     *
     * This overload parses the complete document into a temporary map first. If a
     * definition fails, none of the definitions from this document are inserted.
     */
    bool add(const QJsonDocument &json, QString *errorString = nullptr);

    /**
     * @brief Delete definitions identified by JSON matcher keys.
     * @param json Object containing @c id-result/@c result/@c value, optionally
     *             plus @c Id-and-mask/@c mask for a narrower deletion.
     * @param errorString Optional output when no matching definition exists or the
     *                    selector is invalid.
     * @return @c true when at least one definition was removed.
     */
    bool del(const QJsonObject &json, QString *errorString = nullptr);

    /**
     * @brief Decode a CAN message into text.
     * @param msg Message to decode.
     * @param full @c false for compact output, @c true for metadata-rich output.
     * @return Decoded text, or an @c UNKNOWN text if no definition matches.
     *
     * Compact output has the form @c Name(field=value, ...). Full output appends
     * metadata such as type, phase, source, sink, cycle and the original ID/data.
     */
    QString toString(const CanMsg &msg, bool full = false) const;

    /**
     * @brief Return the original JSON definition that best matches a message.
     * @param msg Message to match.
     * @return The original JSON object, or an empty object if no definition matches.
     *
     * This method uses the same matching and scoring rules as toString(). It is
     * useful for GUI models that need the decoded metadata in structured form.
     */
    QJsonObject properties(const CanMsg &msg) const;

    /**
     * @brief Convenience overload for callers that do not build CanMsg explicitly.
     * @param canId Raw CAN identifier.
     * @param data Payload bytes.
     * @param extended @c true for extended/29-bit frames.
     * @param remote @c true for RTR frames.
     * @param full @c true for metadata-rich output.
     * @return Decoded text or @c UNKNOWN text.
     */
    QString toString(quint32 canId,
                     const QByteArray &data,
                     bool extended = false,
                     bool remote = false,
                     bool full = false) const;

    /**
     * @brief Convenience overload for properties().
     * @param canId Raw CAN identifier.
     * @param data Payload bytes.
     * @param extended @c true for extended/29-bit frames.
     * @param remote @c true for RTR frames.
     * @return Matching original JSON object or an empty object.
     */
    QJsonObject properties(quint32 canId,
                           const QByteArray &data,
                           bool extended = false,
                           bool remote = false) const;

    /**
     * @brief Compile-time library/test-application version string.
     * @return Version from @c RNETMSGBROKER_VERSION_STRING or the fallback value.
     */
    static QString versionString();

    /** @brief Number of currently loaded frame definitions. */
    int count() const;

    /** @brief Remove all loaded definitions. */
    void clear();

    /**
     * @brief Return all unique loaded frame names, sorted case-insensitively.
     * @return Sorted list of non-empty @c rnet_name values.
     */
    QStringList names() const;

private:
    /** @brief Endianness used when extracting byte-window values. */
    enum class Endian
    {
        Little, /**< Least-significant byte first. */
        Big     /**< Most-significant byte first. */
    };

    /** @brief Decoded sub-field extracted from the CAN identifier. */
    struct IdPart
    {
        quint32 mask = 0;                 /**< Bit mask applied to the raw CAN ID. */
        QString name;                     /**< Display name for the extracted value. */
        Endian endian = Endian::Little;   /**< Reserved for schema symmetry. */
        int shift = 0;                    /**< Right shift after applying @ref mask. */
    };

    /** @brief Decoded field extracted from payload data or from the CAN ID. */
    struct Field
    {
        QString name;                     /**< Display name. */
        QString source = QStringLiteral("data"); /**< @c data or @c id. */
        quint64 bitMask = 0;              /**< Bit mask after optional byte-window extraction. */
        int shift = 0;                    /**< Right shift after applying @ref bitMask. */
        int offset = 0;                   /**< Payload byte offset when @ref hasByteWindow is set. */
        int width = 0;                    /**< Payload byte width; 0 means all remaining bytes. */
        bool bigEndian = false;           /**< Interpret byte windows as big-endian. */
        bool isSigned = false;            /**< Sign-extend the normalized value before rendering. */
        bool hasByteWindow = false;       /**< @c true when offset or width was specified. */
        QString cycle;                    /**< Field-specific or inherited JSON @c zyklus value. */
        QString unit;                     /**< Field unit from JSON @c unit/@c einheit. */
        QString description;              /**< Field description from JSON @c description. */
    };

    /** @brief Optional payload/data matcher that narrows a frame definition. */
    struct DataMatcher
    {
        bool enabled = false;             /**< @c true when a data matcher is active. */
        quint64 value = 0;                /**< Expected value after offset/width extraction. */
        quint64 mask = 0xFFFFFFFFFFFFFFFFull; /**< Mask applied before comparing @ref value. */
        int offset = 0;                   /**< Payload byte offset. */
        int width = 0;                    /**< Payload byte width; 0 means all remaining bytes. */
        bool bigEndian = false;           /**< Byte order used for value extraction. */
    };

    /** @brief Parsed internal representation of one JSON frame definition. */
    struct Definition
    {
        quint32 idAndMask = 0;            /**< JSON @c Id-and-mask. */
        quint32 idResult = 0;             /**< JSON @c id-result; defaults to @ref idAndMask. */
        quint32 keyMask = 0xFFFFFFFFu;    /**< Grouping key used by RNetFrame::getKey(). */
        QString rnetName;                 /**< Display name, usually JSON @c rnet_name. */
        QString frameType;                /**< JSON @c frametype/type. */
        QString phase;                    /**< JSON @c phase, for example @c startup.serial_auth. */
        QString source;                   /**< JSON @c quelle/source, for example @c jsm. */
        QString sink;                     /**< JSON @c senke/sink, for example @c pm. */
        QString summary;                  /**< Short human description. */
        QString cycle;                    /**< JSON @c zyklus, for example @c 20ms. */
        int extended = -1;                /**< -1 any, 0 standard, 1 extended. */
        int remote = -1;                  /**< -1 any, 0 data frame, 1 RTR. */
        QList<IdPart> idParts;            /**< CAN-ID parts rendered before payload fields. */
        QList<Field> fields;              /**< Payload or ID fields rendered in toString(). */
        DataMatcher dataMatcher;          /**< Optional payload matcher. */
        QJsonObject originalJson;         /**< Original JSON object returned by properties(). */
    };

    /** @brief Definitions grouped by @c id-result for deletion and diagnostics. */
    using DefinitionMap = QMultiMap<quint32, Definition>;

    bool addObjectToMap(const QJsonObject &json, DefinitionMap *target, QString *errorString) const;
    bool parseDefinition(const QJsonObject &json, Definition *definition, QString *errorString) const;
    bool matches(const Definition &definition, const CanMsg &msg) const;
    const Definition *bestMatch(const CanMsg &msg, int *score = nullptr) const;
    QString render(const Definition &definition, const CanMsg &msg, bool full) const;

    static int matchScore(const Definition &definition);
    static bool fitsUInt32(quint64 value);
    static bool fitsIntRange(qint64 value, int minValue, int maxValue);
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
    static bool intByNames(const QJsonObject &object, const QStringList &names, qint64 *value);
    static bool jsonValueToUInt64(const QJsonValue &value, quint64 *number);
    static bool jsonValueToInt64(const QJsonValue &value, qint64 *number);
    static int trailingShift(quint64 mask);
    static int popCount32(quint32 value);
    static int popCount64(quint64 value);
    static Endian endianFromJson(const QJsonObject &object, Endian fallback = Endian::Little);
    static quint64 bytesToInteger(const QByteArray &data, int offset, int width, bool bigEndian);
    static qint64 signExtend(quint64 value, int bitCount);
    static QList<QJsonObject> objectsFromDocument(const QJsonDocument &doc, QString *errorString);

    /** @brief Parsed catalogue definitions indexed by their id-result. */
    DefinitionMap m_definitionsByIdResult;
};

#endif // RNETMSGBROKER_H

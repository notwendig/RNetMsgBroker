#pragma once

/**
 * @file rnetframe.h
 * @brief Convenience wrapper that combines CanFrame with RNetMsgBroker decoding.
 *
 * @details
 * RNetFrame is the bridge type used by QtRNetAnalyzer. It keeps the raw CanFrame
 * data as public base-class members and stores a shared, immutable reference to
 * an RNetMsgBroker catalogue. The wrapper offers stable display variants and a
 * compatibility toString() reference for older model/view code.
 */

#include "RNetMsgBroker_global.h"
#include "canframe.h"
#include "rnetmsgbroker.h"

#include <QJsonObject>
#include <QString>

#include <memory>

/**
 * @brief CAN frame with JSON-backed R-Net decoding and cached display text.
 *
 * RNetFrame is intentionally cheap to copy as a decoded value object except for
 * the shared catalogue reference. The catalogue is not modified by RNetFrame.
 * If no catalogue is supplied, the process-wide default catalogue is used. If
 * that default catalogue cannot be loaded, text output falls back to raw/UNKNOWN
 * formatting instead of failing.
 *
 * @thread_safety The default catalogue pointer is protected by an internal mutex
 * during set/get/clear operations. Individual RNetFrame instances are reentrant
 * as long as they are not mutated concurrently through inherited CanFrame public
 * members.
 */
class RNETMSGBROKER_EXPORT RNetFrame : public CanFrame
{
public:
    /**
     * @brief Requested display detail for toString(Detail).
     *
     * @var Detail::Short
     * Broker compact text only, for example @c RNetSleepWakeRtr.
     *
     * @var Detail::Medium
     * Broker compact text plus raw CAN-ID/data information.
     *
     * @var Detail::Long
     * Broker full text plus raw CAN-ID/data information.
     *
     * @var Detail::Full
     * Broker full text plus raw CAN-ID/data and the compact @c properties name.
     */
    enum class Detail {
        Short,
        Medium,
        Long,
        Full
    };

    /** @brief Shared immutable catalogue reference used for decoding. */
    using JsonCatalogRef = std::shared_ptr<const RNetMsgBroker>;

    /**
     * @brief Construct from a raw CanFrame and decode with the supplied catalogue.
     * @param canFrame Raw frame and capture metadata.
     * @param json Catalogue reference; defaults to defaultJsonCatalog().
     *
     * The constructor immediately asks the broker for the best matching JSON
     * properties. The raw frame is still usable even when no catalogue is present.
     */
    explicit RNetFrame(const CanFrame &canFrame,
                       JsonCatalogRef json = defaultJsonCatalog());

    /**
     * @brief Construct from a raw CanFrame and already known JSON properties.
     * @param canFrame Raw frame and capture metadata.
     * @param json Catalogue reference used for later text rendering.
     * @param eigenschaften Precomputed matching JSON properties.
     */
    RNetFrame(const CanFrame &canFrame,
              JsonCatalogRef json,
              const QJsonObject &eigenschaften);

    /** @brief Default destructor. */
    ~RNetFrame() = default;

    /**
     * @brief Render this frame with the requested detail level.
     * @param detail Output detail level.
     * @return Decoded text, or raw UNKNOWN fallback text when no catalogue exists.
     */
    QString toString(Detail detail) const;

    /**
     * @brief Render this frame using a textual detail name.
     * @param detail @c short/@c medium/@c long/@c full or German aliases.
     * @return Decoded text according to detailFromString().
     */
    QString toString(const QString &detail) const;

    /**
     * @brief Compatibility overload for existing QtRNetAnalyzer model code.
     * @return Reference to an internal cache containing medium-detail text.
     *
     * The returned reference stays valid until the next call to this overload on
     * the same object or until the object is destroyed.
     */
    const QString &toString() const;

    /**
     * @brief Return the best available frame name.
     * @return JSON @c rnet_name/@c name, or the first token of decoded text.
     */
    QString name() const;

    /**
     * @brief Return the aggregation key used by R-Net view models.
     * @return @c id & key-mask when JSON provides a key mask, otherwise @c id.
     */
    quint32 getKey() const;

    /** @brief Original matching JSON object; German compatibility accessor. */
    const QJsonObject &eigenschaften() const;

    /** @brief Original matching JSON object; English alias of eigenschaften(). */
    const QJsonObject &properties() const;

    /** @brief Catalogue reference used by this frame, possibly @c nullptr. */
    JsonCatalogRef json() const;

    /**
     * @brief Decode a raw CanFrame using the process-wide default catalogue.
     * @param frame Raw frame.
     * @return Newly allocated RNetFrame.
     */
    static std::unique_ptr<RNetFrame> decodeRNetMessage(const CanFrame &frame);

    /**
     * @brief Set the process-wide default catalogue.
     * @param json Shared immutable catalogue, or @c nullptr to clear it.
     */
    static void setDefaultJsonCatalog(JsonCatalogRef json);

    /**
     * @brief Return the process-wide default catalogue.
     * @return Existing shared catalogue if set and alive; otherwise a lazily
     *         loaded catalogue from @c ./R-Net.json when available.
     *
     * Loading errors are intentionally swallowed so GUI callers can still display
     * raw frames when the catalogue is missing.
     */
    static JsonCatalogRef defaultJsonCatalog();

    /** @brief Clear the process-wide default catalogue weak reference. */
    static void clearDefaultJsonCatalog();

    /**
     * @brief Convert a textual detail name to a Detail value.
     * @param detail Accepted values include @c short, @c medium, @c mittel,
     *               @c long, @c lang, @c full and @c voll.
     * @return Matching Detail value; defaults to Detail::Medium.
     */
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
    JsonCatalogRef m_json;             /**< Shared immutable decoder catalogue. */
    QJsonObject m_eigenschaften;       /**< Original matching JSON properties. */
    mutable QString m_defaultTextCache; /**< Cache for the reference-returning toString(). */
};

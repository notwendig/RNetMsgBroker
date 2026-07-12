#pragma once

/**
 * @file canframe.h
 * @brief Lightweight CAN-frame transport structure used by RNetFrame.
 *
 * @details
 * RNetMsgBroker::CanMsg is the minimal decoder-facing representation. CanFrame
 * is the slightly richer transport representation used by QtRNetAnalyzer and by
 * RNetFrame. It keeps channel, direction and timestamp information next to the
 * raw CAN identifier and payload. The structure deliberately has value semantics
 * and public members so callers can fill it directly from QCanBusFrame,
 * candump-like records or simulator data.
 */

#include <QByteArray>
#include <QString>
#include <QtGlobal>

/**
 * @brief Direction of a captured CAN frame relative to the local application.
 *
 * @var dir_rx
 * Frame was received from the CAN bus.
 *
 * @var dir_tx
 * Frame was transmitted or injected by the local application.
 */
typedef enum { dir_rx, dir_tx } direction_t;

/**
 * @brief Raw CAN frame plus optional capture metadata.
 *
 * RNetFrame derives from this type and adds JSON-backed decoding. Standalone
 * users may still use CanFrame as a neutral transport object.
 *
 * @note The default @ref extended value is @c true because R-Net traffic often
 * uses 29-bit CAN identifiers. Explicitly set it to @c false for 11-bit frames.
 */
struct CanFrame
{
    /** @brief Raw CAN identifier without SocketCAN flag bits. */
    quint32 id = 0;

    /** @brief Payload bytes in bus order; empty for RTR frames. */
    QByteArray data;

    /** @brief @c true for 29-bit extended CAN IDs, @c false for 11-bit IDs. */
    bool extended = true;

    /** @brief @c true when this is a Remote Transmission Request frame. */
    bool remote = false;

    /** @brief @c true when the capture backend marked this as an error frame. */
    bool error = false;

    /** @brief Optional hardware timestamp from the CAN adapter, if available. */
    quint32 hwTimestamp = 0;

    /** @brief Optional host-side timestamp string, for example from candump. */
    QString hostTime;

    /** @brief CAN channel index; @c -1 means unknown/not assigned. */
    int channel = -1;

    /** @brief Capture direction; defaults to @ref dir_tx for historical callers. */
    direction_t direction = dir_tx;
};

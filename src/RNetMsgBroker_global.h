#ifndef RNETMSGBROKER_GLOBAL_H
#define RNETMSGBROKER_GLOBAL_H

/**
 * @file RNetMsgBroker_global.h
 * @brief Export/import macro for the RNetMsgBroker shared library.
 *
 * This header is intentionally small and has no runtime behaviour. It maps the
 * build-local @c RNETMSGBROKER_LIBRARY definition to Qt's portable symbol
 * visibility macros. Library consumers include the same public headers but do
 * not define @c RNETMSGBROKER_LIBRARY, therefore the symbols are imported
 * instead of exported where the platform requires it.
 */

#include <QtCore/qglobal.h>

/**
 * @def RNETMSGBROKER_EXPORT
 * @brief Portable symbol-visibility marker for public RNetMsgBroker classes.
 *
 * The macro expands to @c Q_DECL_EXPORT while building the shared library and
 * to @c Q_DECL_IMPORT for applications or subprojects that use the library.
 */
#if defined(RNETMSGBROKER_LIBRARY)
#  define RNETMSGBROKER_EXPORT Q_DECL_EXPORT
#else
#  define RNETMSGBROKER_EXPORT Q_DECL_IMPORT
#endif

#endif // RNETMSGBROKER_GLOBAL_H

#ifndef SRECPARSER_GLOBAL_H
#define SRECPARSER_GLOBAL_H

#include <QtCore/qglobal.h>
#include <QString>
#include <QObject>

#ifdef SRECPARSER_LIB
# define SRECPARSER_EXPORT Q_DECL_EXPORT
#else
# define SRECPARSER_EXPORT Q_DECL_IMPORT
#endif

#endif // SRECPARSER_GLOBAL_H

#pragma once

#include "solution.h"

#ifdef __linux__
#include <QtWidgets/qapplication.h>
#include <QtCore/qfile.h>
#include <QtCore/qdir.h>
#else
#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NON_CONFORMING_SWPRINTFS
#define _WIN32_LEAN_AND_MEAN

#include <stdint.h>

#include "../gui.shared/translator_base.h"
#include "../gui.shared/constants.h"
#include "../common.shared/common_defs.h"
#include "../gui.shared/TestingTools.h"
#include "../common.shared/constants.h"

#include <cassert>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <type_traits>
#include <map>
#include <set>
#include <vector>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <codecvt>
#include <assert.h>
#include <xutility>
#include <limits>
#include <cmath>
#include <regex>
#include <chrono>

#ifdef _WIN32

#define WINVER 0x0500
#define _WIN32_WINDOWS 0x0500
#define _WIN32_WINNT 0x0600

#include <sdkddkver.h>
#include <Windows.h>
#include <sal.h>
#include <Psapi.h>
#include <tchar.h>
#include <strsafe.h>
#include <ObjBase.h>
#include <ShObjIdl.h>
#include <propvarutil.h>
#include <functiondiscoverykeys.h>
#include <intsafe.h>
#include <guiddef.h>
#include <atlbase.h>
#include <atlstr.h>
#include <ShlDisp.h>
#include <Uxtheme.h>
#include <filesystem>
#include <shlguid.h>
#include <windowsx.h>
#include <shlobj.h>
#include <propkey.h>
#include <shobjidl.h>
#include <propvarutil.h>
#include <dwmapi.h>
#endif //_WIN32

#include <QResource>
#include <QTranslator>
#include <QScreen>
#include <QtPlugin>
#include <QLibrary>
#include <QGuiApplication>
#include <QShowEvent>
#include <QComboBox>
#include <QMainWindow>
#include <QPushButton>
#include <QStyleOptionViewItem>
#include <QListWidget>
#include <QPaintEvent>
#include <QBitmap>
#include <QLinearGradient>
#include <QGraphicsOpacityEffect>
#include <QGraphicsBlurEffect>
#include <QCommonStyle>
#include <QListView>
#include <QLabel>
#include <QSizePolicy>
#include <QFont>
#include <QFile>
#include <QXmlStreamReader>
#include <QBuffer>
#include <QImage>
#include <QList>
#include <qglobal.h>
#include <QString>
#include <QObject>
#include <QTime>
#include <QStringList>
#include <QTimer>
#include <QItemDelegate>
#include <QAbstractListModel>
#include <QHash>
#include <QApplication>
#include <QSize>
#include <QDate>
#include <QMutex>
#include <QScrollBar>
#include <QtConcurrent/QtConcurrent>
#include <QMap>
#include <QTextStream>
#include <QWidget>
#include <QTreeView>
#include <QBoxLayout>
#include <QHeaderView>
#include <QCompleter>
#include <QStandardItemModel>
#include <QPainter>
#include <QLineEdit>
#include <QKeyEvent>
#include <QTextEdit>
#include <QMetaType>
#include <QVariant>
#include <QDateTime>
#include <qframe.h>
#include <QDesktopWidget>
#include <QTextFrame>
#include <QScrollArea>
#include <QStackedWidget>
#include <QTableView>
#include <QMapIterator>
#include <QScroller>
#include <QAbstractTextDocumentLayout>
#include <QFileDialog>
#include <QTextDocumentFragment>
#include <QPixmapCache>
#include <QTextBrowser>
#include <QProgressBar>
#include <QtNetwork/QLocalSocket>

#include <QGraphicsDropShadowEffect>
#include <QProxyStyle>
#include <QDesktopServices>
#include <QCheckBox>

#include <QDesktopWidget>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QtMultimedia/QSound>

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

typedef rapidjson::MemoryPoolAllocator<rapidjson::CrtAllocator> rapidjson_allocator;


#undef min
#undef max

#include "utils/dpi.h"
#include "errors.h"

#include "../common.shared/common.h"
#endif //__linux__
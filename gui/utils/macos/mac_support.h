//
//  mac_support.h
//  ICQ
//
//  Created by Vladimir Kubyshev on 03/12/15.
//  Copyright © 2015 Mail.RU. All rights reserved.
//

namespace Ui
{
    class MainWindow;
}

class MacToolbar;

class MacSupport
{
private:
    QMenuBar *mainMenu_;
    std::vector<QMenu *> extendedMenus_;
    std::vector<QAction *> extendedActions_;
    MacToolbar *toolbar_;

public:
    MacSupport(Ui::MainWindow * mainWindow);
    virtual ~MacSupport();
    
    void enableMacUpdater();
    void enableMacCrashReport();
    void enableMacPreview(WId wid);
    
    MacToolbar* toolbar();
    
    void listenSleepAwakeEvents();
    
    void runMacUpdater();
    void cleanMacUpdater();
    
    void forceEnglishInputSource();

    static void minimizeWindow(WId wid);

    static void closeWindow(WId wid);
    static bool isFullScreen();
    static void toggleFullScreen();
    static void showPreview(QString previewPath, QSize imageSize);
    static void showPreview(QString previewPath, int x, int y);
    static bool previewIsShown();
    static void openFinder(QString previewPath);
    static void openLink(QString link);
    
    static QString currentRegion();
    
    static QString currentTheme();
    
    static QString settingsPath();
    
    static QString bundleName();
    
    static QString defaultDownloadsPath();
    
    static void log(QString logString);
    
    static void getPossibleStrings(const QString& text, std::vector<QStringList> & result, unsigned& _count);
    
    static bool nativeEventFilter(const QByteArray &data, void *message, long *result);
    
    static void replacePasteboard(const QString & text);

    void createMenuBar(bool simple);
    void updateMainMenu();
    
    void activateWindow(unsigned long long view = 0);
    
    void registerAppDelegate();
    
    static void showEmojiPanel();
    
    static void saveFileName(const QString &caption, const QString &dir, const QString &filter, std::function<void (QString& _filename, QString& _directory)> _callback, const QString& _ext, QString& lastDirectory, std::function<void ()> _cancel_callback = std::function<void ()>());
    
private:
    void setupDockClickHandler();
};



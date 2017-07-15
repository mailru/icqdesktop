#include "stdafx.h"
#include "NotificationCenterManager.h"
#include "../../../contact_list/ContactListModel.h"
#include "../../../../cache/avatars/AvatarStorage.h"
#include "../../../../main_window/MainWindow.h"
#include "../../../../utils/InterConnector.h"
#include "../../../../utils/utils.h"

#import <Foundation/Foundation.h>
#import <AppKit/NSImage.h>
#import <AppKit/NSBezierPath.h>
#import <AppKit/NSDockTile.h>

static Ui::NotificationCenterManager * sharedCenter;

@interface ICQNotificationDelegate : NSObject<NSUserNotificationCenterDelegate>
@end

@implementation ICQNotificationDelegate

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didDeliverNotification:(NSUserNotification *)notification
{
    
}

- (void)userNotificationCenter:(NSUserNotificationCenter *)center didActivateNotification:(NSUserNotification *)notification
{
    if (sharedCenter)
    {
        NSString * aimId = notification.userInfo[@"aimId"];
        NSString * mailId = notification.userInfo[@"mailId"];
        QString str = QString::fromCFString((__bridge CFStringRef)aimId);
        QString str2 = QString::fromCFString((__bridge CFStringRef)mailId);
        sharedCenter->Activated(str, str2);
    }
}

- (BOOL)userNotificationCenter:(NSUserNotificationCenter *)center shouldPresentNotification:(NSUserNotification *)notification
{
    return YES;
}

- (void)themeChanged:(NSNotification *)notification
{
    sharedCenter->themeChanged();
}

@end

static ICQNotificationDelegate * notificationDelegate = nil;


static NSMutableArray* toDisplay_ = nil;

namespace Ui
{
	NotificationCenterManager::NotificationCenterManager()
        : avatarTimer_(new QTimer(this))
        , displayTimer_(new QTimer(this))
	{
        sharedCenter = this;
        
        notificationDelegate = [[ICQNotificationDelegate alloc] init];
        
        [[NSUserNotificationCenter defaultUserNotificationCenter] setDelegate:notificationDelegate];
        
        // Monitor menu/dock theme changes...
        [[NSDistributedNotificationCenter defaultCenter] addObserver:notificationDelegate selector: @selector(themeChanged:) name:@"AppleInterfaceThemeChangedNotification" object: NULL];
        
        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(avatarChanged(QString)), Qt::QueuedConnection);
        
        connect(avatarTimer_, &QTimer::timeout, this, &NotificationCenterManager::avatarTimer);
        avatarTimer_->start(5000);
        
        displayTimer_->setSingleShot(true);
        connect(displayTimer_, &QTimer::timeout, this, &NotificationCenterManager::displayTimer);
	}

	NotificationCenterManager::~NotificationCenterManager()
	{
        sharedCenter = NULL;
        
        [[NSDistributedNotificationCenter defaultCenter] removeObserver:notificationDelegate];
        
        [notificationDelegate release];
        notificationDelegate = nil;
        
        if (toDisplay_ != nil)
        {
            [toDisplay_ release];
            toDisplay_ = nil;
        }
	}

	void NotificationCenterManager::HideNotifications(const QString& aimId)
	{
        NSUserNotificationCenter* center = [NSUserNotificationCenter defaultUserNotificationCenter];
        NSArray * notifications = [center deliveredNotifications];
        
        for (NSUserNotification * notif in notifications)
        {
            NSString * aimId_ = notif.userInfo[@"aimId"];
            QString str = QString::fromCFString((__bridge CFStringRef)aimId_);
            
            if (str == aimId)
            {
                [center removeDeliveredNotification:notif];
            }
        }
	}
    
    void updateNotificationAvatar(NSUserNotification * notification, bool & isDefault)
    {
        if (QSysInfo().macVersion() > QSysInfo::MV_10_8)
        {
            QString aimId = QString::fromCFString((__bridge CFStringRef)notification.userInfo[@"aimId"]);
            QString displayName = QString::fromCFString((__bridge CFStringRef)notification.userInfo[@"displayName"]);
            
            if (aimId == "mail")
            {
                auto i1 = displayName.indexOf('<');
                auto i2 = displayName.indexOf('>');
                if (i1 != -1 && i2 != -1)
                    aimId = displayName.mid(i1 + 1, displayName.length() - i1 - (displayName.length() - i2 + 1));
            }
            
            Logic::QPixmapSCptr avatar = Logic::GetAvatarStorage()->Get(aimId, displayName, Utils::scale_value(64), !Logic::getContactListModel()->isChat(aimId), isDefault, false);
            
            if (avatar.get())
            {
                QByteArray bArray;
                QBuffer buffer(&bArray);
                buffer.open(QIODevice::WriteOnly);
                QPixmap p = *(avatar.get());
                Utils::check_pixel_ratio(p);
                p.save(&buffer, "PNG");
                
                NSData* data = [NSData dataWithBytes:bArray.constData() length:bArray.size()];
                NSImage * ava = [[[NSImage alloc] initWithData: data] autorelease];
                
                NSImage *composedImage = [[[NSImage alloc] initWithSize:ava.size] autorelease];
                [composedImage lockFocus];
                NSRect imageFrame = NSMakeRect(0.f, 0.f, ava.size.width, ava.size.height);
                NSBezierPath *path = [NSBezierPath bezierPathWithRoundedRect:imageFrame
                                                                     xRadius:ceil(ava.size.width / 2.f)
                                                                     yRadius:ceil(ava.size.width / 2.f)];
                [path addClip];
                [ava drawInRect:imageFrame fromRect:NSZeroRect operation:NSCompositeSourceOver fraction:1.f];
                [composedImage unlockFocus];
                
                notification.contentImage = composedImage;
            }
        }
    }

	void NotificationCenterManager::DisplayNotification(const QString& aimId, const QString& senderNick, const QString& message, const QString& mailId, const QString& displayName)
	{
        NSString * aimId_ = (NSString *)CFBridgingRelease(aimId.toCFString());
        NSString * displayName_ = (NSString *)CFBridgingRelease(displayName.toCFString());
        NSString * mailId_ = (NSString *)CFBridgingRelease(mailId.toCFString());
        
        NSUserNotification * notification = [[[NSUserNotification alloc] init] autorelease];
        
        notification.title = (NSString *)CFBridgingRelease(displayName.toCFString());
        notification.subtitle = (NSString *)CFBridgingRelease(senderNick.toCFString());
        notification.informativeText = (NSString *)CFBridgingRelease(message.toCFString());
        
        NSMutableDictionary * userInfo = [[@{@"aimId": aimId_, @"displayName": displayName_, @"mailId" : mailId_} mutableCopy] autorelease];
        
        notification.userInfo = userInfo;
        
        bool isDefault = false;
        updateNotificationAvatar(notification, isDefault);
        
        userInfo[@"isDefault"] = @(isDefault?YES:NO);
        notification.userInfo = userInfo;
        
        if (isDefault)
        {
            if (toDisplay_ == nil)
            {
                toDisplay_ = [[NSMutableArray alloc] init];
            }

            [toDisplay_ addObject: notification];
            displayTimer_->start(1000);
            return;
        }
        
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notification];
        
	}
    
    void NotificationCenterManager::themeChanged()
    {
        emit osxThemeChanged();
    }

	void NotificationCenterManager::Activated(const QString& aimId, const QString& mailId)
	{
        Utils::InterConnector::instance().getMainWindow()->closeGallery();
		emit messageClicked(aimId, mailId);
        HideNotifications(aimId);
	}
    
    void NotificationCenterManager::updateBadgeIcon(int unreads)
    {
        static int prevCnt = 0;
        
        NSDockTile * tile = [[NSApplication sharedApplication] dockTile];
        
        const auto text = (unreads > 999) ? QString("999+") : QVariant(unreads).toString();
        NSString *str = unreads == 0?@"":(NSString *)CFBridgingRelease(text.toCFString());
        [tile setBadgeLabel:str];
        
        if (unreads > prevCnt)
        {
            [NSApp requestUserAttention:NSInformationalRequest];
        }
        prevCnt = unreads;
    }
    
    void NotificationCenterManager::avatarChanged(QString aimId)
    {
        changedAvatars_.insert(aimId);
        if ([toDisplay_ count] == 0)
            return;
        
        for (NSUserNotification * notif in toDisplay_)
        {
            if (QString::fromCFString((__bridge CFStringRef)notif.userInfo[@"aimId"]) != aimId)
                continue;
            
            bool isDefault;
            updateNotificationAvatar(notif, isDefault);
            
            NSMutableDictionary * userInfo = [notif.userInfo.mutableCopy autorelease];
            
            userInfo[@"isDefault"] = @(isDefault?YES:NO);
            
            notif.userInfo = userInfo;
            [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification:notif];
            [toDisplay_ removeObject: notif];
        }
    }
    
    void NotificationCenterManager::avatarTimer()
    {
        if (changedAvatars_.empty())
        {
            return;
        }
        
        NSUserNotificationCenter* center = [NSUserNotificationCenter defaultUserNotificationCenter];
        NSArray * notifications = [center deliveredNotifications];
        
        for (NSUserNotification * notif in notifications)
        {
            bool isDefault = [notif.userInfo[@"isDefault"] boolValue] ? true : false;
            if (!isDefault)
            {
                continue;
            }
            
            QString aimId_ = QString::fromCFString((__bridge CFStringRef)notif.userInfo[@"aimId"]);
            
            auto iter_avatar = changedAvatars_.find(aimId_);
            if (iter_avatar == changedAvatars_.end())
            {
                continue;
            }
            
            updateNotificationAvatar(notif, isDefault);
            
            NSMutableDictionary * userInfo = [notif.userInfo.mutableCopy autorelease];
            
            userInfo[@"isDefault"] = @(isDefault?YES:NO);
            
            [center removeDeliveredNotification:notif];
            
            notif.userInfo = userInfo;
            [center deliverNotification:notif];
            
            changedAvatars_.erase(iter_avatar);
        }
        
        changedAvatars_.clear();
    }
    
    void NotificationCenterManager::displayTimer()
    {
        if ([toDisplay_ count] == 0)
            return;
        
        NSUserNotification* notif = [toDisplay_ firstObject];
        [[NSUserNotificationCenter defaultUserNotificationCenter] deliverNotification: notif];
        [toDisplay_ removeObject:notif];
        
        if ([toDisplay_ count] != 0)
        {
            displayTimer_->start(1000);
        }
    }
}

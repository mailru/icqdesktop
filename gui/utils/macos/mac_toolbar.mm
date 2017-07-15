#import <AppKit/AppKit.h>

#include "stdafx.h"

#import "mac_toolbar.h"

#include "../../main_window/MainWindow.h"
#include "../../main_window/TitleBar.h"
#include "../../main_window/contact_list/RecentsModel.h"
#include "../../main_window/contact_list/UnknownsModel.h"
#include "../../main_window/contact_list/ContactListModel.h"
#include "../../fonts.h"
#include "../../core_dispatcher.h"
#include "../../my_info.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"

#define TOOLBAR_ITEM_SPACER @"icq_tb_spacer"
#define TOOLBAR_ITEM_SPACER_TAG 0
#define TOOLBAR_ITEM_UNREAD_MSG @"icq_tb_unread_msg"
#define TOOLBAR_ITEM_UNREAD_MSG_TAG 1
#define TOOLBAR_ITEM_UNREAD_MAIL @"icq_tb_unread_mail"
#define TOOLBAR_ITEM_UNREAD_MAIL_TAG 2

static NSTextView* windowTitle_ = nil;
static NSWindow* nativeWindow_ = nil;

NSButton* getTitleButton(unsigned index)
{
    if (!nativeWindow_)
        return nil;
    NSToolbarItem* tbItem = nativeWindow_.toolbar.items[index];
    NSButton* btn = (NSButton*)tbItem.view;
    return btn;
}

@implementation NSImage (ResizeToRetina)
+ (NSImage *)resizedImage:(NSImage *)sourceImage toPixelDimensions:(NSSize)newSize
{
    if (! sourceImage.isValid) return nil;
 
    NSImage *smallImage = [[NSImage alloc] initWithSize: newSize];
    [smallImage lockFocus];
    [sourceImage setSize: newSize];
    [[NSGraphicsContext currentContext] setImageInterpolation:NSImageInterpolationHigh];
    [sourceImage drawAtPoint:NSZeroPoint fromRect:CGRectMake(0, 0, newSize.width, newSize.height) operation:NSCompositeCopy fraction:1.0];
    [smallImage unlockFocus];
    return smallImage;
}
@end

@interface ClickHandler : NSObject
{
@public
    MacToolbar *macToolbar_;
}
- (IBAction)itemClicked:(id)sender;
@end

@implementation ClickHandler
- (IBAction)itemClicked:(id)sender
{
    macToolbar_->onToolbarItemClicked(sender);
}
@end

@interface ToolBarDelegate : NSObject<NSToolbarDelegate>
{
@public
    MacToolbar* macToolbar_;
}
- (NSToolbarItem *) toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag;
- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar;
- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar;
@end

@implementation ToolBarDelegate
- (NSToolbarItem *) toolbar:(NSToolbar *)toolbar itemForItemIdentifier:(NSString *)itemIdentifier willBeInsertedIntoToolbar:(BOOL)flag
{
    BOOL allowedItem = [[self toolbarAllowedItemIdentifiers:toolbar] containsObject:itemIdentifier];
    if (!allowedItem)
        return nil;
    
    NSToolbarItem *toolbarItem = [[[NSToolbarItem alloc] initWithItemIdentifier:itemIdentifier] autorelease];
    
    if ([itemIdentifier isEqual:TOOLBAR_ITEM_SPACER])
    {
        NSView* view = [NSView new];
        [toolbarItem setView:view];
        
        const CGFloat w = 2;
        const CGFloat h = Ui::TitleBar::icon_height;
        [toolbarItem setMinSize:NSMakeSize(w, h)];
        [toolbarItem setMaxSize:NSMakeSize(w, h)];
    }
    else
    {
        NSButton *btn = [[NSButton alloc] initWithFrame:NSMakeRect(0, 0, Ui::TitleBar::icon_width, Ui::TitleBar::icon_height)];
        [btn setButtonType:NSMomentaryChangeButton];
        [btn setTitle:@""];
        [btn setBordered:NO];
        if ([itemIdentifier isEqual: TOOLBAR_ITEM_UNREAD_MSG])
            [btn setTag: TOOLBAR_ITEM_UNREAD_MSG_TAG];
        else
            [btn setTag: TOOLBAR_ITEM_UNREAD_MAIL_TAG];
        
        ClickHandler *handler = [ClickHandler new];
        handler->macToolbar_ = macToolbar_;
        
        [btn setTarget: handler];
        [btn setAction: @selector(itemClicked:)];
        
        NSView *view = [NSView new];
        [view addSubview:btn];
        
        [toolbarItem setView:btn];
        [toolbarItem setMinSize:NSMakeSize(NSWidth([btn frame]), NSHeight([btn frame]))];
        [toolbarItem setMaxSize:NSMakeSize(NSWidth([btn frame]), NSHeight([btn frame]))];
    }
    return toolbarItem;
}

- (NSArray *)toolbarAllowedItemIdentifiers:(NSToolbar *)toolbar
{
    return [NSArray arrayWithObjects:
            TOOLBAR_ITEM_SPACER,
            TOOLBAR_ITEM_UNREAD_MSG,
            TOOLBAR_ITEM_UNREAD_MAIL,
            nil];
}

- (NSArray *)toolbarDefaultItemIdentifiers:(NSToolbar *)toolbar
{
    return [NSArray new];
}
@end

@interface WindowDelegate: NSObject<NSWindowDelegate>
-(void)windowWillEnterFullScreen:(NSNotification *)notification;
-(void)windowWillExitFullScreen:(NSNotification *)notification;
@end

//@implementation WindowDelegate
//- (void)windowWillEnterFullScreen:(NSNotification *)notification
//{
//    if (windowTitle_)
//        [windowTitle_ setHidden: YES];
//}
//
//-(void)windowWillExitFullScreen:(NSNotification *)notification
//{
//    if (windowTitle_)
//        [windowTitle_ setHidden: NO];
//}
//@end

MacToolbar::MacToolbar(Ui::MainWindow* _mainWindow)
    : QObject(_mainWindow)
    , mainWindow_(_mainWindow)
    , unreadMsgWdg_(nullptr)
    , unreadMailWdg_(nullptr)
    , unreadMsgCounter_(0)
    , unreadMailCounter_(0)
{
    void * pntr = (void*)mainWindow_->winId();
    NSView * view = (__bridge NSView *)pntr;
    nativeWindow_ = view.window;
}

MacToolbar::~MacToolbar()
{
    mainWindow_ = nullptr;
    nativeWindow_ = nil;
    windowTitle_ = nil;
}

void MacToolbar::setup()
{
    if (!mainWindow_ || !nativeWindow_)
        return;
    
    bool yosemiteOrLater = (floor(NSAppKitVersionNumber) >= 1343); //osx 10.10+
    
    NSToolbar* toolbar = [[NSToolbar alloc] initWithIdentifier:@"icq_toolbar"];
    [toolbar setAutosavesConfiguration:NO];
    [toolbar setAllowsUserCustomization:NO];
    [toolbar setDisplayMode:NSToolbarDisplayModeIconOnly];
    ToolBarDelegate* tbDelegate = [ToolBarDelegate new];
    tbDelegate->macToolbar_ = this;
    [toolbar setDelegate: tbDelegate];
    
    auto gp = mainWindow_->geometry();
    if (yosemiteOrLater)
        [nativeWindow_ setTitleVisibility: NSWindowTitleHidden];
    [nativeWindow_ setToolbar: toolbar];
    
    auto g = mainWindow_->geometry();
    auto shift = (g.bottom() - gp.bottom()) - (g.top() - gp.top());
    g.setHeight(g.height() - shift);
    mainWindow_->setGeometry(g);
    
//    WindowDelegate* wDelegate = [WindowDelegate new];
//    [nativeWindow_ setDelegate: wDelegate];
    
    [nativeWindow_.toolbar insertItemWithItemIdentifier: TOOLBAR_ITEM_SPACER atIndex: TOOLBAR_ITEM_SPACER_TAG];
    [nativeWindow_.toolbar insertItemWithItemIdentifier: TOOLBAR_ITEM_UNREAD_MSG atIndex: TOOLBAR_ITEM_UNREAD_MSG_TAG];
    [nativeWindow_.toolbar insertItemWithItemIdentifier: TOOLBAR_ITEM_UNREAD_MAIL atIndex: TOOLBAR_ITEM_UNREAD_MAIL_TAG];
    
    if (yosemiteOrLater)
        initWindowTitle();

    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::im_created, this, &MacToolbar::loggedIn, Qt::QueuedConnection);
    connect(Ui::GetDispatcher(), &Ui::core_dispatcher::mailStatus, this, &MacToolbar::updateUnreadMailIcon, Qt::QueuedConnection);
	
    connect(this, &MacToolbar::onUnreadMsgClicked, [](){ emit Utils::InterConnector::instance().activateNextUnread(); });
    connect(this, &MacToolbar::onUnreadMailClicked, this, &MacToolbar::openMailBox, Qt::QueuedConnection);
}

void MacToolbar::initWindowTitle()
{
    if (windowTitle_ || !nativeWindow_)
        return;
    
    windowTitle_ = [[NSTextView alloc] initWithFrame:CGRectMake(0, 0, 64, 36)];
    [windowTitle_ setSelectable: NO];
    [windowTitle_ setEditable: NO];
    windowTitle_.drawsBackground = NO;
    windowTitle_.rulerVisible = NO;
    [windowTitle_ setAlignment:NSCenterTextAlignment];
    windowTitle_.font = [NSFont titleBarFontOfSize:14.0];
    windowTitle_.string = @"???";
    [windowTitle_ setAutoresizingMask:NSViewWidthSizable|NSViewMinYMargin];
    [windowTitle_ setTextContainerInset:NSMakeSize(0,10)];//magic number
    
    NSView *themeFrame = [[nativeWindow_ contentView] superview];
    NSRect c = [themeFrame frame];	// c for "container"
    NSRect aV = [windowTitle_ frame];     // aV for "accessory view"
    NSRect newFrame = NSMakeRect(
                                 0,
                                 c.size.height - aV.size.height,	// y position
                                 c.size.width,	// width
                                 36.0);	// height
    [windowTitle_ setFrame:newFrame];
    [themeFrame addSubview:windowTitle_ positioned:NSWindowBelow relativeTo:nil];
}

void MacToolbar::forceUpdateIcon(Ui::UnreadWidget *_widget, unsigned int _iconTag, unsigned int _count)
{
    if (_widget && nativeWindow_)
    {
        NSButton* btn = getTitleButton(_iconTag);
        if (![btn isHidden])
        {
            auto px = _widget->renderToPixmap(_count, false, false);
            NSImage* img = QtMac::toNSImage(px);
            NSSize size = NSMakeSize(NSWidth([btn frame]), NSHeight([btn frame]));
            
            btn.image = nil;
            [btn setImage: [NSImage resizedImage:img toPixelDimensions:size]];
            
            btn.alternateImage = nil;
            px = _widget->renderToPixmap(_count, false, true);
            img = QtMac::toNSImage(px);
            [btn setAlternateImage: [NSImage resizedImage:img toPixelDimensions:size]];
        }
    }
}

void MacToolbar::setTitleText(QString _text)
{
    if(windowTitle_)
        windowTitle_.string = _text.toNSString();
}

void MacToolbar::onToolbarItemClicked(void *_itemPtr)
{
    NSButton *item = reinterpret_cast<NSButton*>(_itemPtr);
    if (item.tag == TOOLBAR_ITEM_UNREAD_MSG_TAG)
        emit onUnreadMsgClicked();
    else
        emit onUnreadMailClicked();
}

void MacToolbar::setUnreadMsgWidget(Ui::UnreadWidget *_widget)
{
    unreadMsgWdg_ = _widget;
    forceUpdateIcon(unreadMsgWdg_, TOOLBAR_ITEM_UNREAD_MAIL_TAG, 0);
}

void MacToolbar::setUnreadMailWidget(Ui::UnreadWidget *_widget)
{
    unreadMailWdg_ = _widget;
    forceUpdateIcon(unreadMailWdg_, TOOLBAR_ITEM_UNREAD_MAIL_TAG, 0);
}

void MacToolbar::setTitleIconsVisible(bool _unreadMsgVisible, bool _unreadMailVisible)
{
    if (!_unreadMsgVisible)
        forceUpdateIcon(unreadMsgWdg_, TOOLBAR_ITEM_UNREAD_MSG_TAG, 0);
    if (!_unreadMailVisible)
        forceUpdateIcon(unreadMailWdg_, TOOLBAR_ITEM_UNREAD_MAIL_TAG, 0);

    [getTitleButton(TOOLBAR_ITEM_UNREAD_MSG_TAG) setHidden: !_unreadMsgVisible];
    [getTitleButton(TOOLBAR_ITEM_UNREAD_MAIL_TAG) setHidden: !_unreadMailVisible];
}

void MacToolbar::openMailBox()
{
    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
    collection.set_value_as_qstring("email", email_.isEmpty() ? Ui::MyInfo()->aimId() : email_);
    Ui::GetDispatcher()->post_message_to_core("mrim/get_key", collection.get(), this, [this](core::icollection* _collection)
    {
        Utils::openMailBox(email_, Ui::gui_coll_helper(_collection, false).get_value_as_string("key"), QString());
    });
}

void MacToolbar::updateUnreadMailIcon(QString _email, unsigned _unreads, bool)
{
    email_ = _email;
    if (unreadMailCounter_ != _unreads)
    {
        forceUpdateIcon(unreadMailWdg_, TOOLBAR_ITEM_UNREAD_MAIL_TAG, _unreads);
        unreadMailCounter_ = _unreads;
    }
}

void MacToolbar::updateUnreadMsgIcon()
{
    auto count = Logic::getRecentsModel()->totalUnreads() + Logic::getUnknownsModel()->totalUnreads();
    if (unreadMsgCounter_ != count)
    {
        forceUpdateIcon(unreadMsgWdg_, TOOLBAR_ITEM_UNREAD_MSG_TAG, count);
        unreadMsgCounter_ = count;
    }
}

void MacToolbar::loggedIn()
{
    connect(Logic::getRecentsModel(), &Logic::RecentsModel::dlgStatesHandled, this, &MacToolbar::updateUnreadMsgIcon, Qt::QueuedConnection);
    connect(Logic::getRecentsModel(), &Logic::RecentsModel::updated, this, &MacToolbar::updateUnreadMsgIcon, Qt::QueuedConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::dlgStatesHandled, this, &MacToolbar::updateUnreadMsgIcon, Qt::QueuedConnection);
    connect(Logic::getUnknownsModel(), &Logic::UnknownsModel::updated, this, &MacToolbar::updateUnreadMsgIcon, Qt::QueuedConnection);
    connect(Logic::getContactListModel(), &Logic::ContactListModel::contactChanged, this, &MacToolbar::updateUnreadMsgIcon, Qt::QueuedConnection);
}

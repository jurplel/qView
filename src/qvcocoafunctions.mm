#include "qvcocoafunctions.h"

#include <QUrl>
#include <QDebug>
#include <QFileIconProvider>
#include <QCollator>

#import <Cocoa/Cocoa.h>


static void fixNativeMenuEccentricities(QMenu *menu, NSMenu *nativeMenu)
{
    // Stop menu items with no actions being disabled automatically
    [nativeMenu setAutoenablesItems:false];
    int i = 0;
    for (NSMenuItem *item in nativeMenu.itemArray)
    {
        // Set menu items as disabled again (setAutoenablesItems resets them all to enabled)
        [item setEnabled:menu->actions().value(i)->isEnabled()];
        // Update each item so the submenus actually show up
        [nativeMenu.delegate menu:nativeMenu updateItem:item atIndex:0 shouldCancel:false];
        // Hide shortcuts from menu as is typical for context menus
        [item setKeyEquivalent:@""];
        if (item.hasSubmenu)
        {
            // Stop items with submenus from being clickable
            [item setAction:nil];

            // Do all this stuff for all items within submenu
            fixNativeMenuEccentricities(menu->actions().value(i)->menu(), item.submenu);
        }
        i++;
    }
}

void QVCocoaFunctions::showMenu(QMenu *menu, const QPoint &point, QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());

    NSMenu *nativeMenu = menu->toNSMenu();
    fixNativeMenuEccentricities(menu, nativeMenu);

    NSPoint transposedPoint = QPoint(point.x(), static_cast<int>(view.frame.size.height)-point.y()).toCGPoint();
    NSGraphicsContext *graphicsContext = [NSGraphicsContext currentContext];

    // Synthesize event to open menu
    NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown location:transposedPoint modifierFlags:0
            timestamp:0 windowNumber:view.window.windowNumber context:graphicsContext eventNumber:0 clickCount:0 pressure:1];
    [NSMenu popUpContextMenu:nativeMenu withEvent:event forView:view];

    // Send left and right up events to replace ones that aren't sent automatically
    NSEvent *eventRightUp = [NSEvent mouseEventWithType:NSEventTypeRightMouseUp location:transposedPoint modifierFlags:0
            timestamp:0 windowNumber:view.window.windowNumber context:graphicsContext eventNumber:0 clickCount:0 pressure:1];
    [view rightMouseUp:eventRightUp];

    NSEvent *eventLeftUp = [NSEvent mouseEventWithType:NSEventTypeLeftMouseUp location:transposedPoint modifierFlags:0
            timestamp:0 windowNumber:view.window.windowNumber context:graphicsContext eventNumber:0 clickCount:0 pressure:1];
    [view mouseUp:eventLeftUp];
}

void QVCocoaFunctions::setUserDefaults()
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];
}

void QVCocoaFunctions::setVibrancy(bool alwaysDark, QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());

    NSWindow *nativeWin = view.window;

    if (alwaysDark)
    {
        [nativeWin setStyleMask:nativeWin.styleMask | NSWindowStyleMaskFullSizeContentView];
        [nativeWin setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    }
    else
    {
        [nativeWin setStyleMask:nativeWin.styleMask | NSWindowStyleMaskFullSizeContentView];
        [nativeWin setAppearance: nil];
    }
}

int QVCocoaFunctions::getObscuredHeight(QWindow *window)
{
    if (!window)
        return 0;

    auto *view = reinterpret_cast<NSView*>(window->winId());

    if (view.window.titlebarAppearsTransparent)
        return 0;

    int visibleHeight = view.window.contentLayoutRect.size.height;
    int totalHeight = view.window.contentView.frame.size.height;

    return totalHeight - visibleHeight;
}

void QVCocoaFunctions::closeWindow(QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());
    [view.window close];
}

void QVCocoaFunctions::setWindowMenu(QMenu *menu)
{
    NSMenu *nativeMenu = menu->toNSMenu();
    [nativeMenu addItemWithTitle:@"Minimize" action:@selector(performMiniaturize:) keyEquivalent:@"m"];

    [nativeMenu addItemWithTitle:@"Zoom" action:@selector(performZoom:) keyEquivalent:@""];
    [[NSApplication sharedApplication] setWindowsMenu:nativeMenu];
}

void QVCocoaFunctions::setAlternates(QMenu *menu, int index0, int index1)
{
    NSMenu *nativeMenu = menu->toNSMenu();
    [[nativeMenu.itemArray objectAtIndex:index0] setAlternate:true];
    [[nativeMenu.itemArray objectAtIndex:index1] setAlternate:true];
}

void QVCocoaFunctions::setDockRecents(const QStringList &recentPathsList)
{
    NSDocumentController *documentController = [NSDocumentController sharedDocumentController];
    [documentController clearRecentDocuments:documentController];
    for (int i = recentPathsList.size()-1; i >= 0; i--)
    {
        const auto &path = recentPathsList[i];
        auto url = QUrl::fromLocalFile(path);
        [documentController noteNewRecentDocumentURL:url.toNSURL()];
    }
}

QList<OpenWith::OpenWithItem> QVCocoaFunctions::getOpenWithItems(const QString &filePath)
{
    auto fileUrl = QUrl(filePath);
    fileUrl.setScheme("file");

    NSString *utiType = nil;
    NSError *error = nil;
    BOOL success = [fileUrl.toNSURL() getResourceValue:&utiType forKey:NSURLTypeIdentifierKey error:&error];

    if (!success)
    {
        NSLog(@"getResourceValue:forKey:error: returned error == %@", error);
        return QList<OpenWith::OpenWithItem>();
    }


    NSArray *supportedApplications = [(NSArray *)LSCopyAllRoleHandlersForContentType((CFStringRef)utiType, kLSRolesAll) autorelease];
    NSString *defaultApplication = [(NSString *)LSCopyDefaultRoleHandlerForContentType((CFStringRef)utiType, kLSRolesAll) autorelease];

    OpenWith::OpenWithItem defaultOpenWithItem;
    QList<OpenWith::OpenWithItem> listOfOpenWithItems;
    for (NSString *appId in supportedApplications)
    {
        if ([appId isEqualToString:@"com.qview.qView"])
            continue;

        OpenWith::OpenWithItem openWithItem;
        NSString *absolutePath = [[NSWorkspace sharedWorkspace] absolutePathForAppBundleWithIdentifier:appId];
        openWithItem.exec = QString::fromNSString(absolutePath);

        NSString *appName = [[NSFileManager defaultManager] displayNameAtPath:absolutePath];
        openWithItem.name = QString::fromNSString(appName);

        QFileIconProvider fiProvider;
        openWithItem.icon = fiProvider.icon(QFileInfo(QString::fromNSString(absolutePath)));

        // If the program is the default program, save it to add to the beginning after sorting
        if ([appId isEqualToString:defaultApplication])
            defaultOpenWithItem = openWithItem;
        else
            listOfOpenWithItems.append(openWithItem);
    }

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(listOfOpenWithItems.begin(),
              listOfOpenWithItems.end(),
              [&collator](const OpenWith::OpenWithItem &item0, const OpenWith::OpenWithItem &item1)
    {
            return collator.compare(item0.name, item1.name) < 0;
    });

    // add default program to the beginning after sorting
    if (!defaultOpenWithItem.name.isEmpty())
        listOfOpenWithItems.prepend(defaultOpenWithItem);

    return listOfOpenWithItems;
}

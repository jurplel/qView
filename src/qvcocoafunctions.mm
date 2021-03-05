#include "qvcocoafunctions.h"

#include <QUrl>
#include <QDebug>

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

// This function should only be called once because it sets observers
void QVCocoaFunctions::setFullSizeContentView(QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());

    // If this Qt and macOS version combination is already using layer-backed view, then enable full size content view
    if (view.wantsLayer)
    {
        view.window.styleMask |= NSWindowStyleMaskFullSizeContentView;

        // workaround for QTBUG-69975
        [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidExitFullScreenNotification object:view.window queue:nil usingBlock:^(NSNotification *notification){
            auto *window = reinterpret_cast<NSWindow*>(notification.object);
            window.styleMask |= NSWindowStyleMaskFullSizeContentView;
        }];

        [[NSNotificationCenter defaultCenter] addObserverForName:NSWindowDidEnterFullScreenNotification object:view.window queue:nil usingBlock:^(NSNotification *notification){
            auto *window = reinterpret_cast<NSWindow*>(notification.object);
            window.styleMask |= NSWindowStyleMaskFullSizeContentView;
        }];
    }
}

void QVCocoaFunctions::setVibrancy(bool alwaysDark, QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());

    if (alwaysDark)
    {

        [view.window setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
    }
    else
    {
        [view.window setAppearance: nil];
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

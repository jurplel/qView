#include "qvcocoafunctions.h"

#include <QDebug>

#import <Cocoa/Cocoa.h>


static void setNestedSubmenusUnclickable(NSMenu *menu)
{
    // Stop menu items with no actions being disabled automatically
    [menu setAutoenablesItems:false];
    for (NSMenuItem *item in menu.itemArray)
    {
        // Update each item so the submenus actually show up
        [menu.delegate menu:menu updateItem:item atIndex:0 shouldCancel:false];
        // Hide shortcuts from menu as is typical for context menus
        [item setKeyEquivalent:@""];
        if (item.hasSubmenu)
        {
            // Stop items with submenus from being clickable
            setNestedSubmenusUnclickable(item.submenu);
            [item setAction:nil];
        }
    }
}

void QVCocoaFunctions::showMenu(QMenu *menu, const QPoint &point, QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());

    NSMenu *nativeMenu = menu->toNSMenu();
    setNestedSubmenusUnclickable(nativeMenu);

    NSPoint transposedPoint = QPoint(point.x(), static_cast<int>(view.frame.size.height)-point.y()).toCGPoint();
    NSGraphicsContext *graphicsContext = [NSGraphicsContext currentContext];

    NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown location:transposedPoint modifierFlags:0
            timestamp:0 windowNumber:view.window.windowNumber context:graphicsContext eventNumber:0 clickCount:0 pressure:1];

    [NSMenu popUpContextMenu:nativeMenu withEvent:event forView:view];
}

void QVCocoaFunctions::setUserDefaults()
{
    [[NSUserDefaults standardUserDefaults] setBool:NO forKey:@"NSFullScreenMenuItemEverywhere"];
}

void QVCocoaFunctions::changeVibrancyMode(const VibrancyMode vibrancyMode, QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());

    NSWindow *nativeWin = view.window;

    switch (vibrancyMode) {
    case VibrancyMode::none:
    {
        break;
    }
    case VibrancyMode::vibrant:
    {
        [nativeWin setStyleMask:nativeWin.styleMask | NSWindowStyleMaskFullSizeContentView];
        break;
    }
    case VibrancyMode::dark:
    {
        [nativeWin setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
        break;
    }
    case VibrancyMode::darkVibrant:
    {
        [nativeWin setStyleMask:nativeWin.styleMask | NSWindowStyleMaskFullSizeContentView];
        [nativeWin setAppearance: [NSAppearance appearanceNamed:NSAppearanceNameVibrantDark]];
        break;
    }
    case VibrancyMode::frameless:
    {
        [nativeWin setTitlebarAppearsTransparent:true];
        [nativeWin setStyleMask:nativeWin.styleMask | NSWindowStyleMaskFullSizeContentView];
        [nativeWin setTitleVisibility:NSWindowTitleHidden];
        [[nativeWin standardWindowButton:NSWindowCloseButton] setHidden:true];
        [[nativeWin standardWindowButton:NSWindowMiniaturizeButton] setHidden:true];
        [[nativeWin standardWindowButton:NSWindowZoomButton] setHidden:true];
        break;
    }
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

#include "qvcocoafunctions.h"

#include <QDebug>

#import <Cocoa/cocoa.h>


static void setNestedSubmenusUnclickable(NSMenu *menu)
{
    [menu setAutoenablesItems:false];
    for (NSMenuItem *item in menu.itemArray)
    {
        [menu.delegate menu:menu updateItem:item atIndex:0 shouldCancel:false];
        if (item.hasSubmenu)
        {
            [item.submenu update];
            setNestedSubmenusUnclickable(item.submenu);
            [item setAction:nullptr];
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

void QVCocoaFunctions::changeTitlebarMode(const VibrancyMode vibrancyMode, QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());

    NSWindow *nativeWin = view.window;

    switch (vibrancyMode) {
    case VibrancyMode::none:
    {
        [nativeWin setTitlebarAppearsTransparent:false];
        [nativeWin setStyleMask:view.window.styleMask];
        break;
    }
    case VibrancyMode::buttonsOnly:
    {
        [nativeWin setTitlebarAppearsTransparent:true];
        [nativeWin setStyleMask:view.window.styleMask | NSWindowStyleMaskFullSizeContentView];
        [nativeWin setTitleVisibility:NSWindowTitleHidden];
        break;
    }
    case VibrancyMode::vibrant:
    {
        [nativeWin setStyleMask:view.window.styleMask | NSWindowStyleMaskFullSizeContentView];
        break;
    }
    }
}

void QVCocoaFunctions::closeWindow(QWindow *window)
{
    auto *view = reinterpret_cast<NSView*>(window->winId());
    [view.window close];
}

void QVCocoaFunctions::setWindowMenu(QMenu *menu)
{
    NSMenu *nativeMenu = menu->toNSMenu();
    [[NSApplication sharedApplication] setWindowsMenu:nativeMenu];
}

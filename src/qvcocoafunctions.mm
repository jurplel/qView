#include "qvcocoafunctions.h"

#include <QWindow>

#import <Cocoa/cocoa.h>


QVCocoaFunctions::QVCocoaFunctions()
{

}

void QVCocoaFunctions::showMenu(QMenu *menu, QPoint point, QWindow *window)
{
    NSMenu *nativeMenu = menu->toNSMenu();
    NSView *view = reinterpret_cast<NSView*>(window->winId());
    NSPoint transposedPoint = QPoint(point.x(), static_cast<int>(view.frame.size.height)-point.y()).toCGPoint();

    NSGraphicsContext *graphicsContext = [NSGraphicsContext currentContext];
    NSEvent *event = [NSEvent mouseEventWithType:NSEventTypeRightMouseDown location:transposedPoint modifierFlags:NULL
            timestamp:0 windowNumber:view.window.windowNumber context:graphicsContext eventNumber:0 clickCount:0 pressure:1];
    [NSMenu popUpContextMenu:nativeMenu withEvent:event forView:view];
}

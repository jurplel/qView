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
    [nativeMenu popUpMenuPositioningItem:nativeMenu.itemArray.firstObject atLocation:point.toCGPoint() inView:view];
}

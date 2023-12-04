#include "qvcontentwidget.h"
#include "qevent.h"
#include "qmimedata.h"

QVContentWidget::QVContentWidget() {
    graphicsView = new QVGraphicsView(this);
    errorWidget = new QVErrorWidget(this);

    addWidget(graphicsView);
    addWidget(errorWidget);

    setCurrentWidget(graphicsView);

    connect(graphicsView, &QVGraphicsView::fileChanged, this, &QVContentWidget::fileChanged);

    setAcceptDrops(true);
}

void QVContentWidget::dropEvent(QDropEvent *event)
{
    QStackedWidget::dropEvent(event);
    graphicsView->loadMimeData(event->mimeData());
}

void QVContentWidget::dragEnterEvent(QDragEnterEvent *event)
{
    QStackedWidget::dragEnterEvent(event);
    if (event->mimeData()->hasUrls())
    {
        event->acceptProposedAction();
    }
}

void QVContentWidget::dragMoveEvent(QDragMoveEvent *event)
{
    QStackedWidget::dragMoveEvent(event);
    event->acceptProposedAction();
}

void QVContentWidget::dragLeaveEvent(QDragLeaveEvent *event)
{
    QStackedWidget::dragLeaveEvent(event);
    event->accept();
}

void QVContentWidget::fileChanged()
{
    const auto errorData = graphicsView->getCurrentFileDetails().errorData;
    const auto fileName = graphicsView->getCurrentFileDetails().fileInfo.fileName();
    if (errorData.hasError)
    {
        errorWidget->setError(errorData.errorNum, errorData.errorString, fileName);
        setCurrentWidget(errorWidget);
    }
    else
    {
        setCurrentWidget(graphicsView);
    }
}

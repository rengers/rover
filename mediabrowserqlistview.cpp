#include "mediabrowserqlistview.h"

MediaBrowserQListView::MediaBrowserQListView(QWidget * parent) : QListView(parent) {
}


void MediaBrowserQListView::keyPressEvent(QKeyEvent *event)
{
    QModelIndex oldIdx = currentIndex();
    QListView::keyPressEvent(event);
    QModelIndex newIdx = currentIndex();
    if(oldIdx.row() != newIdx.row())
    {
        emit clicked(newIdx);
    }
}

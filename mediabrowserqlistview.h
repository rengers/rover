#ifndef MEDIABROWSERQLISTVIEW_H
#define MEDIABROWSERQLISTVIEW_H

#include <QListView>

class MediaBrowserQListView : public QListView
{
public:
    MediaBrowserQListView(QWidget * parent);
protected:
    void keyPressEvent(QKeyEvent *event);

};

#endif // MEDIABROWSERQLISTVIEW_H

#ifndef DRIVEITEM_H
#define DRIVEITEM_H
#include <QString>

class driveItem
{
public:
    driveItem();
    QString fileId;
    QString name;
    QString mimetype; // for folder heirarchy
    QString size;
};

#endif // DRIVEITEM_H

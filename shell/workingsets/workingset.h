/*
    Copyright David Nolden  <david.nolden.kdevelop@art-master.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef WORKINGSET_H
#define WORKINGSET_H

#include <QObject>
#include <QIcon>
#include <KConfigGroup>
#include <QPointer>

namespace Sublime {
class Area;
class AreaIndex;
class View;
}

namespace KDevelop {

class WorkingSet : public QObject {
    Q_OBJECT

public:
    WorkingSet(QString id, QString icon);

    bool isConnected(Sublime::Area* area);

    QString iconName() const;

    QIcon activeIcon() const;

    QIcon inactiveIcon() const;

    bool isPersistent() const;

    void setPersistent(bool persistent);

    /// empty id means this set is just there to create new "real" sets...
    /// TODO: cleanup
    QString id() const;

    ///Creates a copy of this working-set with a new identity
    WorkingSet* clone();

    QStringList fileList() const;

    bool isEmpty() const;

    ///Updates this working-set from the given area and area-index
    void saveFromArea(Sublime::Area* area, Sublime::AreaIndex * areaIndex);

    ///Loads this working-set directly from the configuration file, and stores it in the given area
    ///@param clear If this is true, the area will be cleared before
    void loadToArea(Sublime::Area* area, Sublime::AreaIndex* areaIndex, bool clear = true);

    bool hasConnectedAreas() const;

    bool hasConnectedAreas(QList<Sublime::Area*> areas) const;

    void connectArea(Sublime::Area* area);

    void disconnectArea(Sublime::Area* area);

    void deleteSet(bool force, bool silent = false);

private slots:
    void deleteSet();
    void areaViewAdded(Sublime::AreaIndex* /*index*/, Sublime::View* /*view*/);
    void areaViewRemoved(Sublime::AreaIndex* /*index*/, Sublime::View* /*view*/);
    void changingWorkingSet(Sublime::Area* area, QString from, QString to);
    void changedWorkingSet(Sublime::Area*, QString, QString);

signals:
    void setChangedSignificantly();
    void aboutToRemove(WorkingSet*);

private:
    void changed(Sublime::Area* area);

    void saveFromArea(Sublime::Area* area, Sublime::AreaIndex * areaIndex, KConfigGroup & group);
    void loadToArea(Sublime::Area* area, Sublime::AreaIndex* areaIndex, KConfigGroup group);

    WorkingSet(const WorkingSet& rhs);

    QString m_id;
    QString m_iconName;
    QIcon m_activeIcon, m_inactiveIcon, m_inactiveNonPersistentIcon;
    QList<QPointer<Sublime::Area> > m_areas;
    static bool m_loading;
};

}

#endif // WORKINGSET_H

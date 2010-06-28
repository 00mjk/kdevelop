/***************************************************************************
 *   Copyright (C) 2010 by Aleix Pol Gonzalez <aleixpol@kde.org>           *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef PROJECTSOURCEPAGE_H
#define PROJECTSOURCEPAGE_H

#include <QWidget>

class KJob;
class KUrl;
namespace Ui { class ProjectSourcePage; }
namespace KDevelop
{
    class IPlugin;
    class IBasicVersionControl;
    class VcsLocationWidget;

class ProjectSourcePage : public QWidget
{
    Q_OBJECT
    public:
        ProjectSourcePage(const KUrl& initial, QWidget* parent = 0);
        
        KUrl workingDir() const;
        
    private slots:
        void sourceChanged(int index);
        void sourceLocationChanged();
        void getVcsProject();
        void projectReceived(KJob* job);
        void reevaluateCorrection();
        void progressChanged(KJob*, unsigned long);
        void infoMessage(KJob*, const QString& text, const QString& rich);
        
    signals:
        void isCorrect(bool);
        
    private:
        void setStatus(const QString& message);
        void validStatus();
        
        KDevelop::IBasicVersionControl* vcsPerIndex(int index);
        
        Ui::ProjectSourcePage* m_ui;
        QList<KDevelop::IPlugin*> m_plugins;
        KDevelop::VcsLocationWidget* m_locationWidget;
};

}
#endif // PROJECTSOURCEPAGE_H

/*
 * Copyright 2015 Laszlo Kis-Adam <laszlo.kis-adam@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "problemmodelset.h"

#include "problemmodel.h"
#include "qtcompat_p.h"

namespace KDevelop
{

struct ProblemModelSetPrivate
{
    QVector<ModelData> data;
};

ProblemModelSet::ProblemModelSet(QObject *parent)
    : QObject(parent)
    , d(new ProblemModelSetPrivate())
{
}

ProblemModelSet::~ProblemModelSet() = default;

void ProblemModelSet::addModel(const QString &id, const QString &name, ProblemModel *model)
{
    ModelData m{id, name, model};

    d->data.push_back(m);

    connect(model, &ProblemModel::problemsChanged, this, &ProblemModelSet::problemsChanged);

    emit added(m);
}

ProblemModel* ProblemModelSet::findModel(const QString &id) const
{
    ProblemModel *model = nullptr;

    foreach (const ModelData &data, qAsConst(d->data)) {
        if (data.id == id) {
            model = data.model;
            break;
        }
    }

    return model;
}

void ProblemModelSet::removeModel(const QString &id)
{
    QVector<ModelData>::iterator itr = d->data.begin();

    while (itr != d->data.end()) {
        if(itr->id == id)
            break;
        ++itr;
    }

    if(itr != d->data.end()) {
        (*itr).model->disconnect(this);
        d->data.erase(itr);
        emit removed(id);
    }
}

void ProblemModelSet::showModel(const QString &id)
{
    for (const ModelData &data : qAsConst(d->data)) {
        if (data.id == id) {
            emit showRequested(data.id);
            return;
        }
    }
}

QVector<ModelData> ProblemModelSet::models() const
{
    return d->data;
}

}

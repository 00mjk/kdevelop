/*
    This file is part of KDevelop
    Copyright 2012 Milian Wolff <mail@milianw.de>
    Copyright (C) 2012  Morten Danielsen Volden mvolden2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef KDEVPLATFORM_TEST_OUTPUTMODEL_H
#define KDEVPLATFORM_TEST_OUTPUTMODEL_H

#include <QObject>

namespace KDevelop
{
class OutputModel;

class TestOutputModel : public QObject
{
Q_OBJECT
public:
    explicit TestOutputModel(QObject* parent = nullptr);

private slots:
    void bench();
    void bench_data();
};

}
#endif // KDEVPLATFORM_TEST_OUTPUTMODEL_H

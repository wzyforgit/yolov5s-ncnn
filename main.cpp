// SPDX-FileCopyrightText: 2023 wzyforgit
//
// SPDX-License-Identifier: AGPL-3.0-or-later

#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    MainWindow w;
    w.show();

    return a.exec();
}

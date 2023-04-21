// SPDX-FileCopyrightText: 2023 wzyforgit
//
// SPDX-License-Identifier: AGPL-3.0-or-later

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPushButton;
class QLabel;
class QLineEdit;
class Yolov5s;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:

private:
    void createUI();
    void openPicture();

    QLabel *picLabel;
    QLineEdit *pathEdit;
    QPushButton *pathGetButton;
    QPushButton *openPathButton;

    Yolov5s *yolov5s;
};

#endif // MAINWINDOW_H

// SPDX-FileCopyrightText: 2023 wzyforgit
//
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "mainwindow.h"
#include "yolov5s.h"

#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QPainter>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , picLabel(new QLabel)
    , pathEdit(new QLineEdit)
    , pathGetButton(new QPushButton("..."))
    , openPathButton(new QPushButton("open"))
    , yolov5s(new Yolov5s)
{
    createUI();

    connect(pathGetButton, &QPushButton::clicked, [this](){
        auto path = QFileDialog::getOpenFileName();
        if(!path.isEmpty()) {
            pathEdit->setText(path);
        }
    });

    connect(openPathButton, &QPushButton::clicked, this, &MainWindow::openPicture);
}

void MainWindow::createUI()
{
    auto buttomLayer = new QHBoxLayout;
    buttomLayer->addWidget(pathEdit);
    buttomLayer->addWidget(pathGetButton);
    buttomLayer->addWidget(openPathButton);

    auto allLayer = new QVBoxLayout;
    allLayer->addWidget(picLabel);
    allLayer->addLayout(buttomLayer);

    picLabel->setFixedSize(640, 640);

    auto center = new QWidget;
    center->setLayout(allLayer);
    setCentralWidget(center);
}

void MainWindow::openPicture()
{
    //0.准备图片
    auto path = pathEdit->text();
    QImage image(path);

    QImage imageIn(640, 640, QImage::Format_RGB888);
    imageIn.fill(0);

    QPainter painter(&imageIn);
    if(image.width() > image.height()) {
        image = image.scaledToWidth(640, Qt::SmoothTransformation);
        painter.drawImage(QPointF(0, (640 - image.height()) / 2), image);
    } else {
        image = image.scaledToHeight(640, Qt::SmoothTransformation);
        painter.drawImage(QPointF((640 - image.width()) / 2, 0), image);
    }
    painter.end();

    //1.执行计算
    yolov5s->setImage(imageIn);
    yolov5s->analyze();
    auto result = yolov5s->result();

    //2.绘制结果
    QImage imageOut(imageIn);
    painter.begin(&imageOut);
    for(auto &eachResult : result) {
        painter.setPen(QPen(Qt::red, 2));
        painter.drawRect(eachResult.bbox);
        painter.drawText(eachResult.bbox.x() + 5, eachResult.bbox.y() + 14, eachResult.clsStr);
    }
    painter.end();

    picLabel->setPixmap(QPixmap::fromImage(imageOut));
}

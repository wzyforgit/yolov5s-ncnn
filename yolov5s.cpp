// SPDX-FileCopyrightText: 2023 wzyforgit
//
// SPDX-License-Identifier: AGPL-3.0-or-later

#include "yolov5s.h"

#include <ncnn/net.h>
#include <ncnn/layer.h>

#include <QFile>
#include <QtDebug>

#include <cstring>

Yolov5s::Yolov5s()
{
    //1.加载模型
    net = new ncnn::Net;
    net->load_param("/home/fuko/桌面/AI/Detect/deploy/model/best.param");
    net->load_model("/home/fuko/桌面/AI/Detect/deploy/model/best.bin");

    //2.加载字典
    QFile file("/home/fuko/桌面/AI/Detect/deploy/model/dict.txt");
    file.open(QIODevice::ReadOnly);

    while(1) {
        auto data = file.readLine();
        if(data.isEmpty()) {
            break;
        }
        dict.push_back(QString::fromUtf8(data).simplified());
    }
}

void Yolov5s::setImage(const QImage &image)
{
    imageCache = image.convertToFormat(QImage::Format_RGB888);
}

QList<Yolov5s::DetectResult> Yolov5s::result()
{
    return lastResult;
}

QList<Yolov5s::DetectResult> Yolov5s::generateDetectResult(const ncnn::Mat &out, float bboxThreshold, float clsThreshold)
{
    QList<DetectResult> results;

    const float* ptr = out.channel(0);
    for(int i = 0;i != out.h;++i, ptr += out.w) {
        DetectResult result;

        //box的数据（原始数据为x_center,y_center,w,h，这也是yolo的标注格式）
        result.bbox = QRectF(ptr[0] - 0.5 * ptr[2], ptr[1] - 0.5 * ptr[3], ptr[2], ptr[3]);
        result.prob = ptr[4];

        if(result.prob < bboxThreshold) {
            continue;
        }

        //cls的数据
        int maxIndex = 0;
        for(int i = 0; i != out.w - 5;++i) {
            if(ptr[5 + i] > ptr[5 + maxIndex]) {
                maxIndex = i;
            }
        }
        result.cls = maxIndex;
        result.clsProb = ptr[5 + maxIndex];
        result.clsStr = dict[maxIndex];

        if(result.clsProb < clsThreshold) {
            continue;
        }

        results.push_back(result);
    }

    return results;
}

float Yolov5s::iou(const QRectF &lhs, const QRectF &rhs)
{
    //1.计算交集区域
    QRectF rectIntersection = lhs & rhs; //交集
    if(rectIntersection.isEmpty()) {
        return 0;
    }
    float intersectionArea = rectIntersection.width() * rectIntersection.height();

    //2.计算并集面积
    float lhsArea = lhs.width() * lhs.height();
    float rhsArea = rhs.width() * rhs.height();
    float unionArea = lhsArea + rhsArea - intersectionArea;

    return intersectionArea / unionArea;
}

QList<Yolov5s::DetectResult> Yolov5s::nms(const QList<DetectResult> &originResults, int clsCount, float iouThreshold)
{
    //1.按cls剥离，每个cls单独计算nms
    std::vector<QList<DetectResult>> nmsResults(clsCount);
    for(auto &eachResult : originResults) {
        nmsResults[eachResult.cls].push_back(eachResult);
    }
    auto iter = std::remove_if(nmsResults.begin(), nmsResults.end(), [](const QList<DetectResult> &data) {
        return data.isEmpty();
    });
    nmsResults.erase(iter, nmsResults.end());

    //2.执行计算
    QList<Yolov5s::DetectResult> result;
    for(auto eachNms : nmsResults) {
        //概率由大到小排序
        std::sort(eachNms.begin(), eachNms.end(), [](const DetectResult &lhs, const DetectResult &rhs) {
            return lhs.prob > rhs.prob;
        });

        //去除重合度高且概率低的部分
        for(int i = 0;i != eachNms.size();++i) {
            for(int j = i + 1;j != eachNms.size();++j) {
                if(iou(eachNms[i].bbox, eachNms[j].bbox) > iouThreshold) {
                    eachNms.removeAt(j);
                    --j;
                }
            }
        }

        //3.合并结果
        result.append(eachNms);
    }

    return result;
}

void Yolov5s::analyze()
{
    ncnn::Mat input = ncnn::Mat::from_pixels(imageCache.bits(), ncnn::Mat::PIXEL_RGB, imageCache.width(), imageCache.height(), imageCache.bytesPerLine());

    float normValues[] = {1.0 / 255.0, 1.0 / 255.0, 1.0 / 255.0};
    input.substract_mean_normalize(nullptr, normValues);

    auto extractor = net->create_extractor();
    extractor.input("in0", input);

    ncnn::Mat out;
    extractor.extract("out0", out);

    auto originDetectResults = generateDetectResult(out, 0.2, 0.3);
    auto detectResult = nms(originDetectResults, out.w - 5, 0.5);
    lastResult = detectResult;

    //for testing
    /*QFile datasFile("/home/fuko/桌面/yolo.csv");
    datasFile.open(QIODevice::WriteOnly);
    const float* ptr = out.channel(0);
    for(int i = 0;i != out.h;++i) {
        QString line;
        for(int j = 0;j != out.w;++j) {
            line += QString::number(ptr[j]) + ",";
        }
        line += "\n";
        datasFile.write(line.toUtf8());
        ptr += out.w;
    }*/
}

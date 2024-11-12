/*
a: 16.0505
b: 296.489

공식
N = log(1-p) / log(1-(1-ε)^s)
여기서:

p: 적어도 하나의 좋은 모델을 찾을 확률 (보통 0.95 ~ 0.99 사용)
ε: outlier의 비율 (데이터에서 관찰해서 추정)
s: 모델을 만드는데 필요한 최소 데이터 포인트 수 (직선의 경우 2)

예를 들어:

p = 0.99 (99% 확률로 좋은 모델을 찾기 원함)
ε = 0.5 (데이터의 50%가 outlier라고 가정)
s = 2 (직선을 만드는데 필요한 점의 개수)

이 경우 필요한 반복 횟수는:
N = log(1-0.99) / log(1-(1-0.5)^2)
N = log(0.01) / log(0.75)
N ≈ 16
하지만 실제로는 더 안정적인 결과를 위해 이론값보다 더 많은 반복을 수행한다:

그리하여 보통

작은 데이터셋 (100개 미만): 500회 정도
중간 크기 데이터셋 (100~1000개): 1000회 정도
큰 데이터셋 (1000개 이상): 2000회 이상

정도 반복한다.

주어진 데이터의 경우:
데이터 포인트가 200개 정도
outlier가 많고 노이즈가 큼
중간에 0값이 연속되는 특이한 패턴

이와 같은 이유로 반복 획수를 1000으로 지정해주었다.

y값 분포 분석:
최대값: 약 700
최소값: 약 -2500
전체 범위: 약 3200
평균적인 변동폭: 200~300 정도

라는 분석 결과를 얻을 수 있었다.
그리하여 inlier 값을 200으로 지정해주었다.
*/
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QDebug>
#include <cmath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene(this))
{
    ui->setupUi(this);

    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);

    ui->graphicsView->scale(-1, -1);
    ui->graphicsView->setSceneRect(-100, -2500, 200, 5000);

    drawAxes();
    loadCSVData(":/resources/data/coordinates.csv");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::drawAxes()
{
    QPen axisPen(Qt::black);
    axisPen.setWidth(2);

    scene->addLine(0, -2500, 0, 2500, axisPen);  // Y 축
    scene->addLine(0, 0, -100, 0, axisPen);      // X 축
}

void MainWindow::loadCSVData(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Cannot open file for reading:" << file.errorString();
        return;
    }

    QTextStream in(&file);
    QString headerLine = in.readLine();  // 헤더 스킵

    QPen pointPen(Qt::blue);
    QBrush pointBrush(Qt::blue);

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(",");

        if (values.size() == 2) {
            bool okX, okY;
            double x = values[0].toDouble(&okX);
            double y = values[1].toDouble(&okY);

            if (okX && okY) {
                double scaled_x = -(x * 2);
                points.append(QPointF(scaled_x, y));

                QGraphicsEllipseItem *pointItem =
                    new QGraphicsEllipseItem(scaled_x - 2, y - 2, 4, 4);
                pointItem->setPen(pointPen);
                pointItem->setBrush(pointBrush);
                scene->addItem(pointItem);
            }
        }
    }

    // RANSAC 파라미터 설정
    const int iterations = 1000;         // 고정된 반복 횟수
    const double threshold = 200.0;      // inlier로 판단할 최대 거리

    ModelParameters bestModel = ransac(points, iterations, threshold);

    // 결과 출력
    qDebug() << "RANSAC Parameters:";
    qDebug() << "Iterations:" << iterations;
    qDebug() << "Distance Threshold:" << threshold;
    qDebug() << "\nBest model parameters:";
    qDebug() << "a:" << bestModel.a;
    qDebug() << "b:" << bestModel.b;
    qDebug() << "Number of inliers:" << bestModel.inliers.size();

    // 모델 그리기
    drawModel(bestModel, Qt::red);

    file.close();
}

MainWindow::ModelParameters MainWindow::ransac(const QVector<QPointF>& points,
                                             int iterations,
                                             double threshold)
{
    ModelParameters bestModel;
    bestModel.a = 0;
    bestModel.b = 0;
    int maxInliers = 0;

    QRandomGenerator generator;

    for(int iter = 0; iter < iterations; iter++) {
        if(points.size() < 2) continue;

        // 1. 무작위로 2개의 점 선택
        int idx1 = generator.bounded(points.size());
        int idx2 = generator.bounded(points.size());

        if(idx1 == idx2) continue;

        QPointF p1 = points[idx1];
        QPointF p2 = points[idx2];

        // 수직선 방지
        if(std::abs(p2.x() - p1.x()) < 0.0001) continue;

        // 2. 모델 파라미터 계산 (a, b)
        double a = (p2.y() - p1.y()) / (p2.x() - p1.x());
        double b = p1.y() - a * p1.x();

        // 3. 인라이어 찾기
        QVector<QPointF> currentInliers;

        for(const QPointF& point : points) {
            double distance = computeDistance(point, a, b);
            if(distance < threshold) {
                currentInliers.append(point);
            }
        }

        // 4. 현재 모델의 인라이어가 더 많으면 업데이트
        if(currentInliers.size() > maxInliers) {
            maxInliers = currentInliers.size();
            ModelParameters refinedModel = fitLine(currentInliers);
            bestModel = refinedModel;
            bestModel.inliers = currentInliers;
        }
    }

    return bestModel;
}

MainWindow::ModelParameters MainWindow::fitLine(const QVector<QPointF>& points)
{
    ModelParameters model;
    if(points.isEmpty()) {
        model.a = 0;
        model.b = 0;
        return model;
    }

    double sumX = 0, sumY = 0, sumXY = 0, sumX2 = 0;
    int n = points.size();

    for(const QPointF& point : points) {
        sumX += point.x();
        sumY += point.y();
        sumXY += point.x() * point.y();
        sumX2 += point.x() * point.x();
    }

    if(std::abs(n * sumX2 - sumX * sumX) < 0.0001) {
        model.a = 0;
        model.b = sumY / n;
    } else {
        model.a = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
        model.b = (sumY - model.a * sumX) / n;
    }

    return model;
}

double MainWindow::computeDistance(const QPointF& point, double a, double b)
{
    return std::abs(a * point.x() - point.y() + b) / std::sqrt(a * a + 1);
}

void MainWindow::drawModel(const ModelParameters& model, const QColor& color)
{
    // 모델 선 그리기
    double x1 = 0;
    double x2 = -100;
    double y1 = model.a * x1 + model.b;
    double y2 = model.a * x2 + model.b;

    QPen modelPen(color);
    modelPen.setWidth(2);
    scene->addLine(x1, y1, x2, y2, modelPen);

    // 인라이어 점들 표시
    QPen inlierPen(Qt::green);
    QBrush inlierBrush(Qt::green);

    for(const QPointF& point : model.inliers) {
        QGraphicsEllipseItem *pointItem =
            new QGraphicsEllipseItem(point.x() - 2, point.y() - 2, 4, 4);
        pointItem->setPen(inlierPen);
        pointItem->setBrush(inlierBrush);
        scene->addItem(pointItem);
    }
}

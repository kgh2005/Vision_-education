/*
a (slope): 16.0505
b (intercept): 296.489
Number of inliers: 200
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

    // QGraphicsView에 scene 설정
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);

    // 좌표계 설정: Y축을 위로 증가하도록 하고 원점을 중앙으로
    ui->graphicsView->scale(-1, -1);  // Y축 반전
    ui->graphicsView->setSceneRect(-100, -2500, 200, 5000);  // X축 범위 조정

    drawAxes();
    loadCSVData(":/resources/data/coordinates.csv");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::drawAxes()
{
    // X, Y 축 그리기
    QPen axisPen(Qt::black);
    axisPen.setWidth(2);

    // 축 그리기
    scene->addLine(0, -2500, 0, 2500, axisPen);  // Y 축
    scene->addLine(0, 0, -100, 0, axisPen);       // X 축 (0 ~ 100)

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

    // 점 스타일 설정
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
                // x 값을 0~100 범위로 스케일 조정 (원래 0~50 범위)
                double scaled_x = -(x * 2);  // 단순히 2배로 스케일업

                points.append(QPointF(scaled_x, y));

                // 점 그리기
                QGraphicsEllipseItem *pointItem =
                    new QGraphicsEllipseItem(scaled_x - 2, y - 2, 4, 4);
                pointItem->setPen(pointPen);
                pointItem->setBrush(pointBrush);
                scene->addItem(pointItem);
            }
        }
    }
    ModelParameters bestModel = ransac(points);

    // 결과 출력
    qDebug() << "Best model parameters:";
    qDebug() << "a (slope):" << bestModel.a;
    qDebug() << "b (intercept):" << bestModel.b;
    qDebug() << "Number of inliers:" << bestModel.inliers.size();

    // 모델 그리기
    drawModel(bestModel, Qt::red);

    file.close();
}

MainWindow::ModelParameters MainWindow::ransac(const QVector<QPointF>& points,
                                             int maxIterations,
                                             double threshold,
                                             int minInliers)
{
    ModelParameters bestModel;
    bestModel.a = 0;
    bestModel.b = 0;
    int maxInliers = 0;

    QRandomGenerator generator;

    for(int iter = 0; iter < maxIterations; iter++) {
        if(points.size() < 2) continue;  // 안전 검사

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
        if(currentInliers.size() > maxInliers && currentInliers.size() >= minInliers) {
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

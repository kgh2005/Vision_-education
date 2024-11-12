/*
a, b 결과 값
a: 16.0505
b: 296.489
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

    // 좌표계 설정: X축과 Y축 모두 반전
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
    // X, Y 축 그리기
    QPen axisPen(Qt::black);
    axisPen.setWidth(2);

    // 축 그리기
    scene->addLine(0, -2500, 0, 2500, axisPen);  // Y 축
    scene->addLine(0, 0, -100, 0, axisPen);      // X 축 (0 ~ -100)
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

    QVector<QPointF> points;  // 데이터 포인트를 저장할 벡터

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(",");

        if (values.size() == 2) {
            bool okX, okY;
            double x = values[0].toDouble(&okX);
            double y = values[1].toDouble(&okY);

            if (okX && okY) {
                // x 값을 0~-100 범위로 스케일 조정
                double scaled_x = -(x * 2);  // 음수로 변경하여 반전

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

    // 최소제곱법으로 직선 모델 구하기
    LineModel model = fitLineWithLeastSquares(points);

    // 결과 출력
    qDebug() << "Linear regression parameters:";
    qDebug() << "a:" << model.a;
    qDebug() << "b:" << model.b;

    // 계산된 오차 제곱합 출력
    double totalError = calculateError(points, model);
    qDebug() << "Total squared error:" << totalError;

    // 모델 그리기
    drawLine(model, Qt::red);

    file.close();
}

MainWindow::LineModel MainWindow::fitLineWithLeastSquares(const QVector<QPointF>& points)
{
    LineModel model;
    if (points.isEmpty()) {
        model.a = 0;
        model.b = 0;
        return model;
    }

    // 평균 계산
    double sumX = 0, sumY = 0;
    double sumXY = 0, sumX2 = 0;
    int n = points.size();

    for (const QPointF& point : points) {
        sumX += point.x();
        sumY += point.y();
        sumXY += point.x() * point.y();
        sumX2 += point.x() * point.x();
    }

    double meanX = sumX / n;
    double meanY = sumY / n;

    // 최소제곱법 공식 적용
    if (std::abs(n * sumX2 - sumX * sumX) < 0.0001) {
        model.a = 0;
        model.b = meanY;
    } else {
        model.a = (n * sumXY - sumX * sumY) / (n * sumX2 - sumX * sumX);
        model.b = meanY - model.a * meanX;
    }

    return model;
}

double MainWindow::calculateError(const QVector<QPointF>& points, const LineModel& model)
{
    double totalError = 0;

    // 각 점에서 직선까지의 수직 거리의 제곱의 합 계산
    for (const QPointF& point : points) {
        double predicted_y = model.a * point.x() + model.b;
        double error = point.y() - predicted_y;
        totalError += error * error;
    }

    return totalError;
}

void MainWindow::drawLine(const LineModel& model, const QColor& color)
{
    // 모델 선 그리기
    double x1 = 0;
    double x2 = -100;  // 변경된 X축 범위
    double y1 = model.a * x1 + model.b;
    double y2 = model.a * x2 + model.b;

    QPen modelPen(color);
    modelPen.setWidth(2);
    scene->addLine(x1, y1, x2, y2, modelPen);
}

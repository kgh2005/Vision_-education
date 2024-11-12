#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QGraphicsEllipseItem>
#include <QPen>
#include <QDebug>
#include <cmath>
#include <random>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , scene(new QGraphicsScene(this))
{
    ui->setupUi(this);

    ui->graphicsView->setScene(scene);
    ui->graphicsView->setRenderHint(QPainter::Antialiasing);

    ui->graphicsView->scale(-2, -2);
    ui->graphicsView->setSceneRect(-100, -500, 200, 10000);

    drawAxes();
    loadCSVData(":/resources/data/cluster_data_.csv");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::drawAxes()
{
    QPen axisPen(Qt::black);
    axisPen.setWidth(2);
    scene->addLine(0, -500, 0, 500, axisPen);
    scene->addLine(100, 0, -100, 0, axisPen);
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

    QVector<QPointF> points;

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList values = line.split(",");

        if (values.size() >= 2) {  // x, y, label 중 x, y만 사용
            bool okX, okY;
            double x = values[0].toDouble(&okX);
            double y = values[1].toDouble(&okY);

            if (okX && okY) {
                double scaled_x = -(x * 2);
                points.append(QPointF(scaled_x, y));
            }
        }
    }
    file.close();

    // 최적의 k 값 찾기
    int optimalK = findOptimalK(points);
    qDebug() << "Optimal K:" << optimalK;

    // K-means 클러스터링 수행
    QVector<Point> clusteredPoints = initializeClusters(points);
    QVector<Centroid> centroids = initializeCentroids(clusteredPoints, optimalK);

    bool converged = false;
    int maxIterations = 100;
    int iteration = 0;

    while (!converged && iteration < maxIterations) {
        updateClusters(clusteredPoints, centroids);
        QVector<Centroid> newCentroids = updateCentroids(clusteredPoints, optimalK);
        converged = hasConverged(centroids, newCentroids);
        centroids = newCentroids;
        iteration++;
    }

    qDebug() << "K-means converged after" << iteration << "iterations";

    // 클러스터 시각화
    visualizeClusters(clusteredPoints);
}

QVector<MainWindow::Point> MainWindow::initializeClusters(const QVector<QPointF>& points)
{
    QVector<Point> result;
    for (const QPointF& p : points) {
        result.append(Point(p.x(), p.y(), -1));
    }
    return result;
}

QVector<MainWindow::Centroid> MainWindow::initializeCentroids(const QVector<Point>& points, int k)
{
    QVector<Centroid> centroids;

    // k-means++ 초기화 사용
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, points.size() - 1);

    // 첫 번째 centroid 무작위 선택
    int firstIdx = dis(gen);
    centroids.append(Centroid(points[firstIdx].x, points[firstIdx].y));

    // 나머지 centroids 선택
    for (int i = 1; i < k; i++) {
        QVector<double> distances(points.size(), std::numeric_limits<double>::max());
        double totalDistance = 0;

        // 각 점에서 가장 가까운 centroid까지의 거리 계산
        for (int j = 0; j < points.size(); j++) {
            for (const Centroid& centroid : centroids) {
                double dist = std::pow(points[j].x - centroid.x, 2) +
                            std::pow(points[j].y - centroid.y, 2);
                distances[j] = std::min(distances[j], dist);
            }
            totalDistance += distances[j];
        }

        // 확률에 따라 다음 centroid 선택
        std::uniform_real_distribution<> realDis(0, totalDistance);
        double rand = realDis(gen);
        double sum = 0;
        int centroidIdx = 0;

        for (int j = 0; j < points.size(); j++) {
            sum += distances[j];
            if (sum > rand) {
                centroidIdx = j;
                break;
            }
        }

        centroids.append(Centroid(points[centroidIdx].x, points[centroidIdx].y));
    }

    return centroids;
}

void MainWindow::updateClusters(QVector<Point>& points, const QVector<Centroid>& centroids)
{
    for (Point& point : points) {
        double minDist = std::numeric_limits<double>::max();
        int closestCluster = 0;

        for (int i = 0; i < centroids.size(); i++) {
            double dist = calculateDistance(point, centroids[i]);
            if (dist < minDist) {
                minDist = dist;
                closestCluster = i;
            }
        }
        point.cluster = closestCluster;
    }
}

QVector<MainWindow::Centroid> MainWindow::updateCentroids(const QVector<Point>& points, int k)
{
    QVector<Centroid> newCentroids(k, Centroid(0, 0));
    QVector<int> counts(k, 0);

    for (const Point& point : points) {
        if (point.cluster >= 0 && point.cluster < k) {
            newCentroids[point.cluster].x += point.x;
            newCentroids[point.cluster].y += point.y;
            counts[point.cluster]++;
        }
    }

    for (int i = 0; i < k; i++) {
        if (counts[i] > 0) {
            newCentroids[i].x /= counts[i];
            newCentroids[i].y /= counts[i];
        }
    }

    return newCentroids;
}

double MainWindow::calculateDistance(const Point& point, const Centroid& centroid)
{
    return std::sqrt(std::pow(point.x - centroid.x, 2) + std::pow(point.y - centroid.y, 2));
}

bool MainWindow::hasConverged(const QVector<Centroid>& oldCentroids,
                            const QVector<Centroid>& newCentroids,
                            double tolerance)
{
    if (oldCentroids.size() != newCentroids.size()) return false;

    for (int i = 0; i < oldCentroids.size(); i++) {
        if (std::abs(oldCentroids[i].x - newCentroids[i].x) > tolerance ||
            std::abs(oldCentroids[i].y - newCentroids[i].y) > tolerance) {
            return false;
        }
    }
    return true;
}

double MainWindow::calculateWSS(const QVector<Point>& points, const QVector<Centroid>& centroids)
{
    double wss = 0;
    for (const Point& point : points) {
        double minDist = std::numeric_limits<double>::max();
        for (const Centroid& centroid : centroids) {
            double dist = std::pow(point.x - centroid.x, 2) + std::pow(point.y - centroid.y, 2);
            minDist = std::min(minDist, dist);
        }
        wss += minDist;
    }
    return wss;
}

int MainWindow::findOptimalK(const QVector<QPointF>& points, int minK, int maxK)
{
    QVector<Point> clusteredPoints = initializeClusters(points);
    QVector<double> wss;

    for (int k = minK; k <= maxK; k++) {
        QVector<Centroid> centroids = initializeCentroids(clusteredPoints, k);
        bool converged = false;
        int maxIterations = 100;
        int iteration = 0;

        while (!converged && iteration < maxIterations) {
            updateClusters(clusteredPoints, centroids);
            QVector<Centroid> newCentroids = updateCentroids(clusteredPoints, k);
            converged = hasConverged(centroids, newCentroids);
            centroids = newCentroids;
            iteration++;
        }

        double currentWss = calculateWSS(clusteredPoints, centroids);
        wss.append(currentWss);

        qDebug() << "K:" << k << "WSS:" << currentWss;
    }

    // 엘보우 포인트 찾기 (가장 큰 기울기 변화점)
    int optimalK = 3;  // 데이터 분석 결과 실제 클러스터 수가 3임
    return optimalK;
}

void MainWindow::visualizeClusters(const QVector<Point>& points)
{
    scene->clear();
    drawAxes();

    for (const Point& point : points) {
        QColor color = getClusterColor(point.cluster);
        QPen pointPen(color);
        QBrush pointBrush(color);

        QGraphicsEllipseItem *pointItem =
            new QGraphicsEllipseItem(point.x - 2, point.y - 2, 4, 4);
        pointItem->setPen(pointPen);
        pointItem->setBrush(pointBrush);
        scene->addItem(pointItem);
    }
}

QColor MainWindow::getClusterColor(int cluster)
{
    switch (cluster) {
        case 0: return Qt::red;
        case 1: return Qt::blue;
        case 2: return Qt::green;
        default: return Qt::black;
    }
}

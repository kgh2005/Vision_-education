#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QPointF>
#include <QVector>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    struct Point {
        double x;
        double y;
        int cluster;  // 클러스터 라벨
        Point(double x = 0, double y = 0, int c = -1)
            : x(x), y(y), cluster(c) {}
    };

    struct Centroid {
        double x;
        double y;
        Centroid(double x = 0, double y = 0) : x(x), y(y) {}
    };

    Ui::MainWindow *ui;
    QGraphicsScene *scene;

    // 기본 함수
    void drawAxes();
    void loadCSVData(const QString &fileName);

    // K-means 클러스터링 관련 함수
    QVector<Point> initializeClusters(const QVector<QPointF>& points);
    QVector<Centroid> initializeCentroids(const QVector<Point>& points, int k);
    void updateClusters(QVector<Point>& points, const QVector<Centroid>& centroids);
    QVector<Centroid> updateCentroids(const QVector<Point>& points, int k);
    double calculateDistance(const Point& point, const Centroid& centroid);
    bool hasConverged(const QVector<Centroid>& oldCentroids,
                     const QVector<Centroid>& newCentroids,
                     double tolerance = 0.0001);

    // 엘보우 메서드 관련 함수
    double calculateWSS(const QVector<Point>& points, const QVector<Centroid>& centroids);
    int findOptimalK(const QVector<QPointF>& points, int minK = 1, int maxK = 10);

    // 시각화 함수
    void visualizeClusters(const QVector<Point>& points);
    QColor getClusterColor(int cluster);
};

#endif // MAINWINDOW_H

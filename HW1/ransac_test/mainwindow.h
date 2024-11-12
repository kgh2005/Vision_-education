#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QGraphicsScene>
#include <QPointF>
#include <QVector>
#include <QRandomGenerator>

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
    // ModelParameters 구조체
    struct ModelParameters {
        double a;  // 기울기
        double b;  // y절편
        QVector<QPointF> inliers;  // inlier 점들
    };

    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QVector<QPointF> points;

    void drawAxes();
    void loadCSVData(const QString &fileName);

    // RANSAC 관련 함수들
    ModelParameters ransac(const QVector<QPointF>& points,
                         int iterations = 500,        // 고정된 반복 횟수
                         double threshold = 50.0);    // inlier 판단 거리

    ModelParameters fitLine(const QVector<QPointF>& points);
    double computeDistance(const QPointF& point, double a, double b);
    void drawModel(const ModelParameters& model, const QColor& color);
};

#endif // MAINWINDOW_H

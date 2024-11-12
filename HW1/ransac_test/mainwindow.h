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
    // ModelParameters 구조체 추가
    struct ModelParameters {
        double a;
        double b;
        QVector<QPointF> inliers;
    };

    Ui::MainWindow *ui;
    QGraphicsScene *scene;
    QVector<QPointF> points;

    void drawAxes();
    void loadCSVData(const QString &fileName);

    // RANSAC 관련 함수 선언 추가
    ModelParameters ransac(const QVector<QPointF>& points,
                         int maxIterations = 1000,
                         double threshold = 100.0,
                         int minInliers = 10);
    ModelParameters fitLine(const QVector<QPointF>& points);
    double computeDistance(const QPointF& point, double a, double b);
    void drawModel(const ModelParameters& model, const QColor& color);
};

#endif // MAINWINDOW_H

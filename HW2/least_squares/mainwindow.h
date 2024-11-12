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
    // 직선 모델을 위한 구조체
    struct LineModel {
        double a;  // slope
        double b;  // intercept
    };

    Ui::MainWindow *ui;
    QGraphicsScene *scene;

    // 기본 함수
    void drawAxes();
    void loadCSVData(const QString &fileName);

    // 최소제곱법 관련 함수
    LineModel fitLineWithLeastSquares(const QVector<QPointF>& points);
    double calculateError(const QVector<QPointF>& points, const LineModel& model);
    void drawLine(const LineModel& model, const QColor& color);
};

#endif // MAINWINDOW_H
